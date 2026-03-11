#include "video/videoexporter.h"
#include "video/videodecoder.h"
#include "model/subtitlerenderer.h"
#include <QImage>
#include <QThread>
#include <QFile>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>

// Set to true to dump frames at each stage to .cursor/export_debug/ for visual debugging
static const bool kDebugExportFrames = true;
static const int kDebugSaveFirstN = 90;
static const int kDebugSaveEveryNth = 60;

// #region agent log
static void agentLog(const char *location, const char *message, const QJsonObject &data) {
    QFile f("C:/Users/dpvpd/source/reels-forge/SubtitleForge/.cursor/debug.log");
    if (!f.open(QIODevice::Append | QIODevice::WriteOnly | QIODevice::Text)) return;
    QJsonObject o;
    o.insert("location", QString::fromUtf8(location));
    o.insert("message", QString::fromUtf8(message));
    o.insert("data", data);
    o.insert("timestamp", QDateTime::currentMSecsSinceEpoch());
    f.write(QJsonDocument(o).toJson(QJsonDocument::Compact) + "\n");
    f.close();
}
static bool shouldSaveDebugFrame(qint64 outF) {
    if (!kDebugExportFrames) return false;
    if (outF < kDebugSaveFirstN) return true;
    if (outF % kDebugSaveEveryNth == 0) return true;
    return false;
}
static void saveDebugFrame(const QImage &img, qint64 outF, qint64 needIdx, const char *stage) {
    if (!shouldSaveDebugFrame(outF) || img.isNull()) return;
    QDir dir("C:/Users/dpvpd/source/reels-forge/SubtitleForge/.cursor/export_debug");
    if (!dir.exists() && !dir.mkpath(".")) return;
    QString path = dir.absoluteFilePath(
        QString("frame_outF_%1_needIdx_%2_%3.png").arg(outF).arg(needIdx).arg(stage));
    if (img.copy(0, 0, img.width(), img.height()).save(path)) {
        QJsonObject d;
        d.insert("outF", static_cast<double>(outF));
        d.insert("needIdx", static_cast<double>(needIdx));
        d.insert("stage", QString::fromUtf8(stage));
        d.insert("path", path);
        agentLog("videoexporter.cpp:debug_frame_saved", "debug frame saved", d);
    }
}
// #endregion

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

VideoExporter::VideoExporter(QObject *parent) : QObject(parent) {}

bool VideoExporter::exportVideo(const Project &project,
                                const ExportSettings &cfg)
{
    m_cancel = false;
    m_lastError.clear();
    const qint64 totalFrames = project.totalFrames();
    if (totalFrames <= 0) {
        m_lastError = "No frames to export.";
        emit exportFinished(false, m_lastError);
        return false;
    }

    // ---- format context (MP4 only) ----
    AVFormatContext *fmtCtx = nullptr;
    avformat_alloc_output_context2(&fmtCtx, nullptr, nullptr,
                                    cfg.outputPath.toUtf8().constData());
    if (!fmtCtx) {
        m_lastError = "Could not create output context.";
        emit exportFinished(false, m_lastError);
        return false;
    }

    const AVPixelFormat destPixFmt = AV_PIX_FMT_YUV420P;
    const int encW = std::max(16, (cfg.width / 16) * 16);
    const int encH = std::max(16, (cfg.height / 16) * 16);

    AVCodecContext *enc = nullptr;
    AVStream *stream = nullptr;
    const AVCodec *codec = nullptr;

    auto tryOpenEncoder = [&](const AVCodec *c) -> bool {
        if (!c) return false;
        stream = avformat_new_stream(fmtCtx, nullptr);
        if (!stream) return false;
        enc = avcodec_alloc_context3(c);
        if (!enc) { stream = nullptr; return false; }
        enc->width     = encW;
        enc->height    = encH;
        enc->time_base = {1, cfg.fps};
        enc->framerate = {cfg.fps, 1};
        enc->pix_fmt   = destPixFmt;
        enc->gop_size  = 12;
        enc->max_b_frames = 0;
        enc->thread_count = 0;
        if (QString(c->name) != "libx264")
            enc->bit_rate = 80LL * 1000 * 1000;
        if (fmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
            enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        AVDictionary *opts = nullptr;
        int err = avcodec_open2(enc, c, &opts);
        av_dict_free(&opts);
        if (err < 0) {
            avcodec_free_context(&enc);
            enc = nullptr;
            stream = nullptr;
            return false;
        }
        codec = c;
        avcodec_parameters_from_context(stream->codecpar, enc);
        stream->time_base = enc->time_base;
        return true;
    };

    codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!tryOpenEncoder(codec)) {
        // #region agent log
        agentLog("videoexporter.cpp:fallback", "H.264 failed, trying MPEG-4", QJsonObject());
        // #endregion
        avformat_free_context(fmtCtx);
        fmtCtx = nullptr;
        avformat_alloc_output_context2(&fmtCtx, nullptr, nullptr, cfg.outputPath.toUtf8().constData());
        if (!fmtCtx) {
            m_lastError = "Could not create output context.";
            emit exportFinished(false, m_lastError);
            return false;
        }
        codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
        if (!codec || !tryOpenEncoder(codec)) {
            // #region agent log
            agentLog("videoexporter.cpp:encoder_fail", "both H.264 and MPEG-4 failed", QJsonObject());
            // #endregion
            if (fmtCtx) avformat_free_context(fmtCtx);
            m_lastError = "Could not open any video encoder (tried H.264 and MPEG-4).";
            emit exportFinished(false, m_lastError);
            return false;
        }
    }
    // #region agent log
    { QJsonObject d; d.insert("codec", QString::fromUtf8(codec->name)); agentLog("videoexporter.cpp:encoder_ok", "encoder opened", d); }
    // #endregion

    enc->color_range = AVCOL_RANGE_JPEG;
    enc->colorspace  = AVCOL_SPC_BT709;

    const QString srcPath = project.mediaFilePath();

    int inAudioStreamIdx = -1;
    AVStream *outAudioStream = nullptr;
    if (!srcPath.isEmpty()) {
        AVFormatContext *probe = nullptr;
        if (avformat_open_input(&probe, srcPath.toUtf8().constData(), nullptr, nullptr) == 0
            && avformat_find_stream_info(probe, nullptr) == 0) {
            for (unsigned i = 0; i < probe->nb_streams; ++i) {
                if (probe->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                    inAudioStreamIdx = static_cast<int>(i);
                    outAudioStream = avformat_new_stream(fmtCtx, nullptr);
                    if (outAudioStream) {
                        if (avcodec_parameters_copy(outAudioStream->codecpar, probe->streams[i]->codecpar) >= 0)
                            outAudioStream->time_base = probe->streams[i]->time_base;
                    }
                    break;
                }
            }
            avformat_close_input(&probe);
        }
    }

    // #region agent log
    agentLog("videoexporter.cpp:before_avio", "before avio_open", QJsonObject());
    // #endregion
    if (!(fmtCtx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&fmtCtx->pb, cfg.outputPath.toUtf8().constData(),
                      AVIO_FLAG_WRITE) < 0) {
            // #region agent log
            agentLog("videoexporter.cpp:avio_open", "Could not open output file", QJsonObject());
            // #endregion
            avcodec_free_context(&enc);
            avformat_free_context(fmtCtx);
            m_lastError = "Could not open output file.";
            emit exportFinished(false, m_lastError);
            return false;
        }
    }
    // #region agent log
    agentLog("videoexporter.cpp:avio_ok", "avio_open done", QJsonObject());
    // #endregion

    if (avformat_write_header(fmtCtx, nullptr) < 0) {
        // #region agent log
        agentLog("videoexporter.cpp:write_header", "Could not write output header", QJsonObject());
        // #endregion
        avcodec_free_context(&enc);
        if (!(fmtCtx->oformat->flags & AVFMT_NOFILE))
            avio_closep(&fmtCtx->pb);
        avformat_free_context(fmtCtx);
        m_lastError = "Could not write output header.";
        emit exportFinished(false, m_lastError);
        return false;
    }
    // #region agent log
    agentLog("videoexporter.cpp:header_ok", "write_header done", QJsonObject());
    // #endregion

    // ---- packet buffer for interleaved write (video + optional audio) ----
    struct PacketEntry {
        int64_t sortPts;
        AVPacket *pkt;
    };
    std::vector<PacketEntry> packetBuffer;
    auto pushPacket = [&packetBuffer](AVStream *s, AVPacket *pkt) {
        AVPacket *copy = av_packet_alloc();
        if (!copy) return;
        av_packet_ref(copy, pkt);
        copy->stream_index = s->index;
        int64_t sortPts = av_rescale_q(copy->pts, s->time_base, AVRational{1, AV_TIME_BASE});
        packetBuffer.push_back({sortPts, copy});
    };

    // ---- pixel conversion ----
    SwsContext *sws = sws_getContext(
        encW, encH, AV_PIX_FMT_RGBA,
        encW, encH, destPixFmt,
        SWS_LANCZOS, nullptr, nullptr, nullptr);

    AVFrame *avFrame = av_frame_alloc();
    avFrame->format = destPixFmt;
    avFrame->width  = encW;
    avFrame->height = encH;
    avFrame->color_range = AVCOL_RANGE_JPEG;
    avFrame->colorspace  = AVCOL_SPC_BT709;
    av_frame_get_buffer(avFrame, 32);

    AVPacket *pkt = av_packet_alloc();
    bool ok = true;
    const double sourceFps = project.sourceFrameRate();
    const int outputFps = cfg.fps;  // 60
    const qint64 outputFrameCount = totalFrames * outputFps / static_cast<qint64>(sourceFps);
    const qint64 trimStartF = project.trimStartFrame();
    // #region agent log
    { QJsonObject d; d.insert("outputFrameCount", static_cast<double>(outputFrameCount)); d.insert("sourceFps", sourceFps); agentLog("videoexporter.cpp:loop_start", "frame loop starting", d); }
    // #endregion

    // Sequential decode on main thread: one open, decode in order → smooth output
    AVFormatContext *videoFmt = nullptr;
    AVCodecContext *videoDec = nullptr;
    int videoStreamIdx = -1;
    SwsContext *videoSws = nullptr;
    AVFrame *decFrame = nullptr;
    AVPacket *readPkt = nullptr;
    double trimStartSec = 0.0;
    double decodeFps = sourceFps;
    std::map<qint64, QImage> frameByPtsIndex;
    bool useSeq = false;
    if (!srcPath.isEmpty() && sourceFps > 0) {
        if (avformat_open_input(&videoFmt, srcPath.toUtf8().constData(), nullptr, nullptr) == 0
            && avformat_find_stream_info(videoFmt, nullptr) == 0) {
            for (unsigned i = 0; i < videoFmt->nb_streams; ++i) {
                if (videoFmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    videoStreamIdx = static_cast<int>(i);
                    break;
                }
            }
            if (videoStreamIdx >= 0) {
                AVStream *vSt0 = videoFmt->streams[videoStreamIdx];
                double streamFps = av_q2d(vSt0->avg_frame_rate);
                if (streamFps <= 0.0) streamFps = av_q2d(vSt0->r_frame_rate);
                if (streamFps > 0.0) decodeFps = streamFps;
                const AVCodec *decCodec = avcodec_find_decoder(
                    videoFmt->streams[videoStreamIdx]->codecpar->codec_id);
                if (decCodec) {
                    videoDec = avcodec_alloc_context3(decCodec);
                    if (videoDec
                        && avcodec_parameters_to_context(videoDec,
                                videoFmt->streams[videoStreamIdx]->codecpar) == 0
                        && avcodec_open2(videoDec, decCodec, nullptr) == 0
                        && videoDec->width > 0 && videoDec->height > 0) {
                        decFrame = av_frame_alloc();
                        readPkt = av_packet_alloc();
                        if (decFrame && readPkt) {
                            videoSws = sws_getContext(
                                videoDec->width, videoDec->height, static_cast<AVPixelFormat>(videoDec->pix_fmt),
                                encW, encH, AV_PIX_FMT_RGBA,
                                SWS_BILINEAR, nullptr, nullptr, nullptr);
                            if (videoSws) {
                                int64_t seekTs = static_cast<int64_t>((trimStartF / decodeFps) * AV_TIME_BASE);
                                if (av_seek_frame(videoFmt, -1, seekTs, AVSEEK_FLAG_BACKWARD) == 0) {
                                    useSeq = true;
                                    trimStartSec = trimStartF / decodeFps;
                                    // #region agent log
                                    { QJsonObject d; d.insert("decodeFps", decodeFps); d.insert("trimStartSec", trimStartSec); agentLog("videoexporter.cpp:decode_init", "decode FPS and trim", d); }
                                    // #endregion
                                    avcodec_send_packet(videoDec, nullptr);
                                    while (avcodec_receive_frame(videoDec, decFrame) == 0) { }
                                } else {
                                    useSeq = true;
                                    trimStartSec = 0.0;
                                    // #region agent log
                                    { QJsonObject d; d.insert("decodeFps", decodeFps); d.insert("trimStartSec", trimStartSec); agentLog("videoexporter.cpp:decode_init", "decode FPS (seek failed)", d); }
                                    // #endregion
                                }
                            }
                        }
                        if (!useSeq) {
                            if (readPkt) av_packet_free(&readPkt);
                            readPkt = nullptr;
                            if (decFrame) av_frame_free(&decFrame);
                            decFrame = nullptr;
                            if (videoSws) { sws_freeContext(videoSws); videoSws = nullptr; }
                        }
                    }
                    if (!useSeq && videoDec) {
                        avcodec_free_context(&videoDec);
                        videoDec = nullptr;
                    }
                }
            }
            if (!useSeq && videoFmt) {
                avformat_close_input(&videoFmt);
                videoFmt = nullptr;
            }
        }
    }
    // So fallback frameAt() has correct file time when sequential decode didn't run
    trimStartSec = trimStartF / decodeFps;

    int decodeStoreLogCount = 0;
    double segmentStartPtsSec = 0.0;
    bool segmentStartSet = false;
    for (qint64 outF = 0; outF < outputFrameCount && !m_cancel; ++outF) {
        // #region agent log
        if (outF == 0) { agentLog("videoexporter.cpp:frame_0_start", "frame 0 start", QJsonObject()); }
        // #endregion
        qint64 srcFrame = static_cast<qint64>((outF * decodeFps) / outputFps);
        if (srcFrame >= totalFrames) srcFrame = totalFrames - 1;
        const qint64 needIdx = srcFrame;
        // Subtitle frame must match the same moment as the video (needIdx) to avoid drift when decodeFps != sourceFps
        qint64 srcFrameProject = static_cast<qint64>(std::round(static_cast<double>(needIdx) * sourceFps / decodeFps));
        if (srcFrameProject < 0) srcFrameProject = 0;
        if (srcFrameProject >= totalFrames) srcFrameProject = totalFrames - 1;

        QImage videoFrame;
        if (useSeq && videoFmt && videoDec && videoSws && decFrame && readPkt) {
            AVStream *vSt = videoFmt->streams[videoStreamIdx];
            const double tb = av_q2d(vSt->time_base);
            bool pendingPkt = false;
            while (frameByPtsIndex.find(needIdx) == frameByPtsIndex.end()) {
                if (outF == 0 && !pendingPkt) {
                    QJsonObject d; d.insert("needIdx", static_cast<double>(needIdx)); agentLog("videoexporter.cpp:decode_while_enter", "decode while enter", d);
                }
                if (!pendingPkt) {
                    int readRet = av_read_frame(videoFmt, readPkt);
                    if (readRet < 0) {
                        if (outF == 0) { QJsonObject d; d.insert("ret", readRet); agentLog("videoexporter.cpp:av_read_frame_fail", "av_read_frame failed", d); }
                        break;
                    }
                    if (readPkt->stream_index != videoStreamIdx) {
                        av_packet_unref(readPkt);
                        continue;
                    }
                    // After seek, decoder is empty; only keyframe can start decode
                    if (frameByPtsIndex.empty() && !(readPkt->flags & AV_PKT_FLAG_KEY)) {
                        av_packet_unref(readPkt);
                        continue;
                    }
                    pendingPkt = true;
                }
                int sendRet = avcodec_send_packet(videoDec, readPkt);
                if (outF == 0) {
                    static int sendLogN = 0;
                    if (sendLogN < 10) {
                        QJsonObject d; d.insert("sendRet", sendRet); d.insert("EAGAIN", AVERROR(EAGAIN)); d.insert("n", sendLogN); d.insert("isKey", static_cast<bool>(readPkt->flags & AV_PKT_FLAG_KEY));
                        agentLog("videoexporter.cpp:send_packet_ret", "send_packet return", d);
                        sendLogN++;
                    }
                }
                if (sendRet == 0) {
                    pendingPkt = false;
                    av_packet_unref(readPkt);
                } else if (sendRet == AVERROR(EAGAIN)) {
                    // Decoder full; drain with receive_frame below, keep pendingPkt
                } else {
                    // e.g. invalid packet after seek; skip this packet and try next
                    pendingPkt = false;
                    av_packet_unref(readPkt);
                }
                int recvCount = 0;
                while (true) {
                    int recvRet = avcodec_receive_frame(videoDec, decFrame);
                    if (outF == 0 && recvCount < 10) {
                        QJsonObject d; d.insert("recvRet", recvRet); d.insert("n", recvCount);
                        agentLog("videoexporter.cpp:receive_frame_ret", "receive_frame return", d);
                        recvCount++;
                    }
                    if (recvRet != 0) break;
                    if (decFrame->width <= 0 || decFrame->height <= 0) {
                        if (outF == 0 && decodeStoreLogCount < 3) { QJsonObject d; d.insert("w", decFrame->width); d.insert("h", decFrame->height); agentLog("videoexporter.cpp:receive_skip_size", "receive_frame skip width/height", d); }
                        continue;
                    }
                    double ptsSec = (decFrame->pts == AV_NOPTS_VALUE) ? 0.0 : (decFrame->pts * tb);
                    if (!segmentStartSet) {
                        segmentStartPtsSec = ptsSec;
                        segmentStartSet = true;
                    }
                    qint64 presentationIdx = static_cast<qint64>((ptsSec - segmentStartPtsSec) * decodeFps + 0.5);
                    if (outF == 0 && decodeStoreLogCount < 5) {
                        QJsonObject d; d.insert("ptsSec", ptsSec); d.insert("segmentStartPtsSec", segmentStartPtsSec); d.insert("presentationIdx", static_cast<double>(presentationIdx)); d.insert("w", decFrame->width); d.insert("h", decFrame->height);
                        agentLog("videoexporter.cpp:receive_frame_ok", "receive_frame got frame", d);
                    }
                    if (presentationIdx < 0) continue;
                    int w = decFrame->width, h = decFrame->height;
                    if (w > 0 && h > 0) {
                        QImage im(encW, encH, QImage::Format_ARGB32);
                        if (im.bits()) {
                            int stride = im.bytesPerLine();
                            uint8_t *dst = im.bits();
                            SwsContext *scaleCtx = (w == videoDec->width && h == videoDec->height)
                                ? videoSws
                                : sws_getContext(
                                    w, h, static_cast<AVPixelFormat>(decFrame->format),
                                    encW, encH, AV_PIX_FMT_RGBA,
                                    SWS_BILINEAR, nullptr, nullptr, nullptr);
                            if (scaleCtx) {
                                sws_scale(scaleCtx, decFrame->data, decFrame->linesize, 0, h, &dst, &stride);
                                frameByPtsIndex[presentationIdx] = im.copy();
                                // #region agent log
                                if (decodeStoreLogCount < 25) {
                                    QJsonObject d; d.insert("presentationIdx", static_cast<double>(presentationIdx)); d.insert("ptsSec", ptsSec); d.insert("trimStartSec", trimStartSec); d.insert("n", decodeStoreLogCount);
                                    agentLog("videoexporter.cpp:decode_store", "frame stored by presentationIdx", d);
                                    decodeStoreLogCount++;
                                }
                                // #endregion
                                if (scaleCtx != videoSws) sws_freeContext(scaleCtx);
                            } else if (outF == 0 && decodeStoreLogCount < 2) {
                                agentLog("videoexporter.cpp:scaleCtx_null", "scaleCtx is null", QJsonObject());
                            }
                        } else if (outF == 0 && decodeStoreLogCount < 2) {
                            agentLog("videoexporter.cpp:im_bits_null", "QImage bits() null", QJsonObject());
                        }
                    }
                    if (frameByPtsIndex.find(needIdx) != frameByPtsIndex.end()) break;
                }
                if (frameByPtsIndex.find(needIdx) != frameByPtsIndex.end()) break;
            }
            qint64 keyUsed = -1;
            auto it = frameByPtsIndex.find(needIdx);
            if (it != frameByPtsIndex.end() && !it->second.isNull()) {
                videoFrame = it->second;
                keyUsed = needIdx;
            } else if (!frameByPtsIndex.empty()) {
                auto last = frameByPtsIndex.upper_bound(needIdx);
                if (last != frameByPtsIndex.begin()) {
                    --last;
                    if (!last->second.isNull()) { videoFrame = last->second; keyUsed = last->first; }
                }
            }
            // #region agent log
            if (outF <= 5 || outF == 10 || outF == 20 || outF == 30 || outF == 60 || outF == 90) {
                QJsonObject d; d.insert("outF", static_cast<double>(outF)); d.insert("needIdx", static_cast<double>(needIdx)); d.insert("mapSize", static_cast<double>(frameByPtsIndex.size())); d.insert("foundExact", frameByPtsIndex.count(needIdx) != 0); d.insert("keyUsed", static_cast<double>(keyUsed));
                agentLog("videoexporter.cpp:lookup", "frame lookup result", d);
            }
            // #endregion
            qint64 nextNeedIdx = static_cast<qint64>((outF + 1) * decodeFps / outputFps);
            while (!frameByPtsIndex.empty() && frameByPtsIndex.begin()->first < nextNeedIdx)
                frameByPtsIndex.erase(frameByPtsIndex.begin());
        }
        if (videoFrame.isNull() && !srcPath.isEmpty()) {
            // File time for this frame: trim start (sec) + needIdx in decode FPS
            double timeSec = trimStartSec + (needIdx / decodeFps);
            videoFrame = VideoDecoder::frameAt(srcPath, timeSec);
        }
        saveDebugFrame(videoFrame, outF, needIdx, "decoded");
        if (outF == 0) { agentLog("videoexporter.cpp:frame_0_before_render", "frame 0 before renderFrame", QJsonObject()); }
        QImage img = SubtitleRenderer::renderFrame(
            project, srcFrameProject, videoFrame, encW, encH);
        if (outF == 0) { agentLog("videoexporter.cpp:frame_0_after_render", "frame 0 after renderFrame", QJsonObject()); }
        img = img.convertToFormat(QImage::Format_RGBA8888);
        saveDebugFrame(img, outF, needIdx, "rendered");
        if (img.isNull() || !img.constBits()) {
            // #region agent log
            QJsonObject d; d.insert("outF", static_cast<double>(outF)); agentLog("videoexporter.cpp:render_null", "frame render null", d);
            // #endregion
            ok = false;
            break;
        }

        if (outF == 0) { agentLog("videoexporter.cpp:frame_0_before_sws", "frame 0 before sws_scale", QJsonObject()); }
        av_frame_make_writable(avFrame);

        const uint8_t *srcSlice[1] = { img.constBits() };
        int srcStride[1] = { static_cast<int>(img.bytesPerLine()) };
        sws_scale(sws, srcSlice, srcStride, 0, encH,
                  avFrame->data, avFrame->linesize);
        avFrame->pts = outF;
        if (outF == 0) { agentLog("videoexporter.cpp:frame_0_before_send", "frame 0 before avcodec_send_frame", QJsonObject()); }

        if (avcodec_send_frame(enc, avFrame) < 0) {
            // #region agent log
            QJsonObject d; d.insert("outF", static_cast<double>(outF)); agentLog("videoexporter.cpp:send_frame_fail", "avcodec_send_frame failed", d);
            // #endregion
            ok = false;
            break;
        }
        while (avcodec_receive_packet(enc, pkt) == 0) {
            av_packet_rescale_ts(pkt, enc->time_base, stream->time_base);
            pushPacket(stream, pkt);
            av_packet_unref(pkt);
        }

        int pct = outputFrameCount > 0 ? static_cast<int>(outF * 100 / outputFrameCount) : 0;
        emit progressChanged(pct);
        // #region agent log
        if (outF == 0) { agentLog("videoexporter.cpp:frame_0_done", "first frame encoded", QJsonObject()); }
        else if (outF > 0 && outF % 500 == 0) { QJsonObject d; d.insert("outF", static_cast<double>(outF)); agentLog("videoexporter.cpp:frame_progress", "frame progress", d); }
        // #endregion
    }

    // Flush video encoder
    avcodec_send_frame(enc, nullptr);
    while (avcodec_receive_packet(enc, pkt) == 0) {
        av_packet_rescale_ts(pkt, enc->time_base, stream->time_base);
        pushPacket(stream, pkt);
        av_packet_unref(pkt);
    }

    if (videoSws) sws_freeContext(videoSws);
    if (readPkt) av_packet_free(&readPkt);
    if (decFrame) av_frame_free(&decFrame);
    if (videoDec) avcodec_free_context(&videoDec);
    if (videoFmt) avformat_close_input(&videoFmt);

    if (outAudioStream && inAudioStreamIdx >= 0 && !srcPath.isEmpty() && !m_cancel) {
        AVFormatContext *audioFmt = nullptr;
        if (avformat_open_input(&audioFmt, srcPath.toUtf8().constData(), nullptr, nullptr) == 0
            && avformat_find_stream_info(audioFmt, nullptr) == 0) {
            AVStream *inSt = audioFmt->streams[inAudioStreamIdx];
            const double trimEndSec = trimStartSec + (totalFrames / decodeFps);
            const int64_t trimStartTs = static_cast<int64_t>(trimStartSec * AV_TIME_BASE);
            if (av_seek_frame(audioFmt, -1, trimStartTs, AVSEEK_FLAG_BACKWARD) == 0) {
                AVPacket *apkt = av_packet_alloc();
                while (av_read_frame(audioFmt, apkt) >= 0) {
                    if (apkt->stream_index != inAudioStreamIdx) {
                        av_packet_unref(apkt);
                        continue;
                    }
                    double pktSec = apkt->pts == AV_NOPTS_VALUE ? 0.0 : (apkt->pts * av_q2d(inSt->time_base));
                    if (pktSec >= trimEndSec) break;
                    if (pktSec < trimStartSec) {
                        av_packet_unref(apkt);
                        continue;
                    }
                    AVPacket *copy = av_packet_alloc();
                    if (copy) {
                        av_packet_ref(copy, apkt);
                        copy->stream_index = outAudioStream->index;
                        const int64_t trimStartTicks = av_rescale_q(static_cast<int64_t>(trimStartSec * AV_TIME_BASE), AVRational{1, AV_TIME_BASE}, inSt->time_base);
                        copy->pts = av_rescale_q(apkt->pts - trimStartTicks, inSt->time_base, outAudioStream->time_base);
                        copy->dts = apkt->dts == AV_NOPTS_VALUE ? AV_NOPTS_VALUE : av_rescale_q(apkt->dts - trimStartTicks, inSt->time_base, outAudioStream->time_base);
                        int64_t sortPts = av_rescale_q(copy->pts, outAudioStream->time_base, AVRational{1, AV_TIME_BASE});
                        packetBuffer.push_back({sortPts, copy});
                    }
                    av_packet_unref(apkt);
                }
                av_packet_free(&apkt);
            }
            avformat_close_input(&audioFmt);
        }
    }

    // Sort by PTS and write interleaved (video + audio)
    std::sort(packetBuffer.begin(), packetBuffer.end(),
              [](const PacketEntry &a, const PacketEntry &b) { return a.sortPts < b.sortPts; });
    for (auto &e : packetBuffer) {
        av_interleaved_write_frame(fmtCtx, e.pkt);
        av_packet_free(&e.pkt);
    }

    av_write_trailer(fmtCtx);

    av_packet_free(&pkt);
    av_frame_free(&avFrame);
    sws_freeContext(sws);
    avcodec_free_context(&enc);
    if (!(fmtCtx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&fmtCtx->pb);
    avformat_free_context(fmtCtx);

    // #region agent log
    { QJsonObject d; d.insert("ok", ok); d.insert("cancelled", m_cancel); agentLog("videoexporter.cpp:export_done", "export finished", d); }
    // #endregion

    if (m_cancel) {
        m_lastError = "Export cancelled.";
        emit exportFinished(false, m_lastError);
        return false;
    }

    emit progressChanged(100);
    emit exportFinished(true, {});
    return ok;
}
