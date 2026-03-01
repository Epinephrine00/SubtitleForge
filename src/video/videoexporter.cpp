#include "video/videoexporter.h"
#include "model/subtitlerenderer.h"
#include <QImage>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <vector>
#include <algorithm>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

VideoExporter::VideoExporter(QObject *parent) : QObject(parent) {}

bool VideoExporter::exportVideo(const Project &project,
                                const ExportSettings &cfg)
{
    m_cancel = false;
    const qint64 totalFrames = project.totalFrames();
    if (totalFrames <= 0) {
        emit exportFinished(false, "No frames to export.");
        return false;
    }

    // ---- format context ----
    AVFormatContext *fmtCtx = nullptr;
    if (cfg.alphaChannel) {
        avformat_alloc_output_context2(&fmtCtx, nullptr, "mov", nullptr);
    } else {
        avformat_alloc_output_context2(&fmtCtx, nullptr, nullptr,
                                       cfg.outputPath.toUtf8().constData());
    }
    if (!fmtCtx) {
        emit exportFinished(false, "Could not create output context.");
        return false;
    }

    // ---- find encoder ----
    const AVCodec *codec = nullptr;
    AVPixelFormat destPixFmt;

    if (cfg.alphaChannel) {
        codec = avcodec_find_encoder_by_name("prores_ks");
        if (codec)
            destPixFmt = AV_PIX_FMT_YUVA444P10LE;

        if (!codec) {
            codec = avcodec_find_encoder(AV_CODEC_ID_QTRLE);
            if (codec)
                destPixFmt = AV_PIX_FMT_ARGB;
        }
        if (!codec) {
            avformat_free_context(fmtCtx);
            emit exportFinished(false,
                "No alpha-capable encoder found (prores_ks / qtrle).");
            return false;
        }
    } else {
        codec = avcodec_find_encoder_by_name("libx264");
        if (!codec)
            codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!codec) {
            avformat_free_context(fmtCtx);
            emit exportFinished(false, "H.264 encoder not found.");
            return false;
        }
        bool has444 = false;
        if (codec->pix_fmts) {
            for (const auto *p = codec->pix_fmts; *p != AV_PIX_FMT_NONE; ++p)
                if (*p == AV_PIX_FMT_YUV444P) { has444 = true; break; }
        }
        destPixFmt = has444 ? AV_PIX_FMT_YUV444P : AV_PIX_FMT_YUV420P;
    }

    // ---- stream + encoder ----
    AVStream *stream = avformat_new_stream(fmtCtx, nullptr);
    if (!stream) {
        avformat_free_context(fmtCtx);
        emit exportFinished(false, "Could not create video stream.");
        return false;
    }

    AVCodecContext *enc = avcodec_alloc_context3(codec);
    if (!enc) {
        avformat_free_context(fmtCtx);
        emit exportFinished(false, "Could not allocate encoder context.");
        return false;
    }

    enc->width     = cfg.width;
    enc->height    = cfg.height;
    enc->time_base = {1, cfg.fps};
    enc->framerate = {cfg.fps, 1};
    enc->pix_fmt   = destPixFmt;

    if (!cfg.alphaChannel) {
        enc->gop_size     = 1;
        enc->max_b_frames = 0;
        enc->color_range  = AVCOL_RANGE_JPEG;
        enc->colorspace   = AVCOL_SPC_BT709;
    }

    // Encoder-level multi-threading
    int hwThreads = static_cast<int>(std::thread::hardware_concurrency());
    enc->thread_count = std::max(1, hwThreads / 2);

    if (fmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
        enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    AVDictionary *encOpts = nullptr;
    if (cfg.alphaChannel) {
        if (QString(codec->name) == "prores_ks") {
            av_dict_set(&encOpts, "profile", "4", 0);
            av_dict_set(&encOpts, "vendor",  "apl0", 0);
        }
    } else {
        if (QString(codec->name) == "libx264") {
            av_dict_set(&encOpts, "preset", "medium", 0);
            av_dict_set(&encOpts, "crf",    "0",      0);
        } else {
            enc->bit_rate = 80LL * 1000 * 1000;
        }
    }

    if (avcodec_open2(enc, codec, &encOpts) < 0) {
        av_dict_free(&encOpts);
        avcodec_free_context(&enc);
        avformat_free_context(fmtCtx);
        emit exportFinished(false, "Could not open encoder.");
        return false;
    }
    av_dict_free(&encOpts);

    avcodec_parameters_from_context(stream->codecpar, enc);
    stream->time_base = enc->time_base;

    if (!(fmtCtx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&fmtCtx->pb, cfg.outputPath.toUtf8().constData(),
                      AVIO_FLAG_WRITE) < 0) {
            avcodec_free_context(&enc);
            avformat_free_context(fmtCtx);
            emit exportFinished(false, "Could not open output file.");
            return false;
        }
    }

    if (avformat_write_header(fmtCtx, nullptr) < 0) {
        avcodec_free_context(&enc);
        if (!(fmtCtx->oformat->flags & AVFMT_NOFILE))
            avio_closep(&fmtCtx->pb);
        avformat_free_context(fmtCtx);
        emit exportFinished(false, "Could not write output header.");
        return false;
    }

    // ---- pixel conversion ----
    SwsContext *sws = sws_getContext(
        cfg.width, cfg.height, AV_PIX_FMT_RGBA,
        cfg.width, cfg.height, destPixFmt,
        SWS_LANCZOS, nullptr, nullptr, nullptr);

    AVFrame *avFrame = av_frame_alloc();
    avFrame->format = destPixFmt;
    avFrame->width  = cfg.width;
    avFrame->height = cfg.height;
    if (!cfg.alphaChannel) {
        avFrame->color_range = AVCOL_RANGE_JPEG;
        avFrame->colorspace  = AVCOL_SPC_BT709;
    }
    av_frame_get_buffer(avFrame, 32);

    AVPacket *pkt = av_packet_alloc();
    bool ok = true;

    // ==================================================================
    // Multi-threaded rendering pipeline (producer-consumer)
    // ==================================================================
    const int numWorkers = std::max(1, hwThreads - 1);
    const int maxAhead   = numWorkers * 2;

    std::mutex                     mtx;
    std::condition_variable        cv;
    std::map<qint64, QImage>       buffer;
    qint64                         nextRender = 0;
    qint64                         nextEncode = 0;
    std::atomic<bool>              cancelFlag{false};

    auto renderWorker = [&]() {
        while (true) {
            qint64 f;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [&] {
                    return cancelFlag.load()
                        || nextRender >= totalFrames
                        || (nextRender - nextEncode) < maxAhead;
                });
                if (cancelFlag.load() || nextRender >= totalFrames)
                    return;
                f = nextRender++;
            }
            cv.notify_all();

            QImage img = SubtitleRenderer::renderFrame(
                project, f, cfg.width, cfg.height, cfg.alphaChannel);
            img = img.convertToFormat(QImage::Format_RGBA8888);

            {
                std::lock_guard<std::mutex> lock(mtx);
                buffer[f] = std::move(img);
            }
            cv.notify_all();
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(numWorkers);
    for (int i = 0; i < numWorkers; ++i)
        workers.emplace_back(renderWorker);

    // Encoding loop — consumes rendered frames in order
    for (qint64 f = 0; f < totalFrames && !m_cancel; ++f) {
        QImage img;
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&] {
                return buffer.count(f) > 0 || m_cancel;
            });
            if (m_cancel) break;
            img = std::move(buffer[f]);
            buffer.erase(f);
            nextEncode = f + 1;
        }
        cv.notify_all();

        av_frame_make_writable(avFrame);

        const uint8_t *srcSlice[1] = { img.constBits() };
        int srcStride[1] = { static_cast<int>(img.bytesPerLine()) };
        sws_scale(sws, srcSlice, srcStride, 0, cfg.height,
                  avFrame->data, avFrame->linesize);
        avFrame->pts = f;

        if (avcodec_send_frame(enc, avFrame) < 0) { ok = false; break; }
        while (avcodec_receive_packet(enc, pkt) == 0) {
            av_packet_rescale_ts(pkt, enc->time_base, stream->time_base);
            pkt->stream_index = stream->index;
            av_interleaved_write_frame(fmtCtx, pkt);
            av_packet_unref(pkt);
        }

        int pct = static_cast<int>(f * 100 / totalFrames);
        emit progressChanged(pct);
    }

    // Signal workers to stop and join
    cancelFlag.store(true);
    cv.notify_all();
    for (auto &w : workers)
        w.join();

    // Flush encoder
    avcodec_send_frame(enc, nullptr);
    while (avcodec_receive_packet(enc, pkt) == 0) {
        av_packet_rescale_ts(pkt, enc->time_base, stream->time_base);
        pkt->stream_index = stream->index;
        av_interleaved_write_frame(fmtCtx, pkt);
        av_packet_unref(pkt);
    }

    av_write_trailer(fmtCtx);

    av_packet_free(&pkt);
    av_frame_free(&avFrame);
    sws_freeContext(sws);
    avcodec_free_context(&enc);
    if (!(fmtCtx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&fmtCtx->pb);
    avformat_free_context(fmtCtx);

    if (m_cancel) {
        emit exportFinished(false, "Export cancelled.");
        return false;
    }

    emit progressChanged(100);
    emit exportFinished(true, {});
    return ok;
}
