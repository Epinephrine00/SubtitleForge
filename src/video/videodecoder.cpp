#include "video/videodecoder.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

#include <memory>

namespace {

struct AVFormatCtxDeleter {
    void operator()(AVFormatContext *p) const {
        if (p) avformat_close_input(&p);
    }
};
struct AVCodecCtxDeleter {
    void operator()(AVCodecContext *p) const {
        if (p) avcodec_free_context(&p);
    }
};
struct SwsCtxDeleter {
    void operator()(SwsContext *p) const {
        if (p) sws_freeContext(p);
    }
};

} // namespace

QImage VideoDecoder::frameAt(const QString &filePath, double timeSec)
{
    AVFormatContext *fmtRaw = nullptr;
    if (avformat_open_input(&fmtRaw, filePath.toUtf8().constData(), nullptr, nullptr) < 0)
        return QImage();
    std::unique_ptr<AVFormatContext, AVFormatCtxDeleter> fmt(fmtRaw);

    if (avformat_find_stream_info(fmt.get(), nullptr) < 0)
        return QImage();

    int streamIdx = -1;
    for (unsigned i = 0; i < fmt->nb_streams; ++i) {
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            streamIdx = static_cast<int>(i);
            break;
        }
    }
    if (streamIdx < 0) return QImage();

    AVStream *stream = fmt->streams[streamIdx];
    const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) return QImage();

    std::unique_ptr<AVCodecContext, AVCodecCtxDeleter> dec(avcodec_alloc_context3(codec));
    if (avcodec_parameters_to_context(dec.get(), stream->codecpar) < 0)
        return QImage();
    if (avcodec_open2(dec.get(), codec, nullptr) < 0)
        return QImage();

    // Seek to timestamp (stream time base)
    int64_t seekTarget = static_cast<int64_t>(timeSec * stream->time_base.den / stream->time_base.num);
    seekTarget = std::max<int64_t>(0, seekTarget);
    if (av_seek_frame(fmt.get(), streamIdx, seekTarget, AVSEEK_FLAG_BACKWARD) < 0) {
        av_seek_frame(fmt.get(), streamIdx, 0, AVSEEK_FLAG_BACKWARD);
    }

    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    QImage result;

    auto cleanup = [&]() {
        av_frame_free(&frame);
        av_packet_free(&pkt);
    };

    const double tb = av_q2d(stream->time_base);
    while (av_read_frame(fmt.get(), pkt) >= 0) {
        if (pkt->stream_index != streamIdx) {
            av_packet_unref(pkt);
            continue;
        }
        if (avcodec_send_packet(dec.get(), pkt) < 0) {
            av_packet_unref(pkt);
            continue;
        }
        av_packet_unref(pkt);

        while (avcodec_receive_frame(dec.get(), frame) == 0) {
            double framePtsSec = (frame->pts == AV_NOPTS_VALUE) ? 0.0 : (frame->pts * tb);
            if (framePtsSec < timeSec) continue;
            SwsContext *swsRaw = sws_getContext(
                frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
                frame->width, frame->height, AV_PIX_FMT_BGRA,
                SWS_BILINEAR, nullptr, nullptr, nullptr);
            if (!swsRaw) { cleanup(); return QImage(); }
            std::unique_ptr<SwsContext, SwsCtxDeleter> sws(swsRaw);

            QImage img(frame->width, frame->height, QImage::Format_ARGB32);
            int dstStride = img.bytesPerLine();
            uint8_t *dstData = img.bits();
            sws_scale(sws.get(), frame->data, frame->linesize, 0, frame->height,
                      &dstData, &dstStride);
            result = img;
            cleanup();
            return result;
        }
    }

    cleanup();
    return result;
}

double VideoDecoder::durationSec(const QString &filePath)
{
    AVFormatContext *fmtRaw = nullptr;
    if (avformat_open_input(&fmtRaw, filePath.toUtf8().constData(), nullptr, nullptr) < 0)
        return -1;
    std::unique_ptr<AVFormatContext, AVFormatCtxDeleter> fmt(fmtRaw);

    if (avformat_find_stream_info(fmt.get(), nullptr) < 0)
        return -1;

    double dur = -1;
    if (fmt->duration != AV_NOPTS_VALUE)
        dur = static_cast<double>(fmt->duration) / AV_TIME_BASE;

    for (unsigned i = 0; i < fmt->nb_streams; ++i) {
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVStream *s = fmt->streams[i];
            if (s->duration != AV_NOPTS_VALUE) {
                double d = s->duration * av_q2d(s->time_base);
                if (d > 0 && (dur < 0 || d < dur)) dur = d;
            }
            break;
        }
    }
    return dur;
}

QSize VideoDecoder::videoSize(const QString &filePath)
{
    AVFormatContext *fmtRaw = nullptr;
    if (avformat_open_input(&fmtRaw, filePath.toUtf8().constData(), nullptr, nullptr) < 0)
        return QSize(0, 0);
    std::unique_ptr<AVFormatContext, AVFormatCtxDeleter> fmt(fmtRaw);

    if (avformat_find_stream_info(fmt.get(), nullptr) < 0)
        return QSize(0, 0);

    for (unsigned i = 0; i < fmt->nb_streams; ++i) {
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            int w = fmt->streams[i]->codecpar->width;
            int h = fmt->streams[i]->codecpar->height;
            return QSize(w, h);
        }
    }
    return QSize(0, 0);
}

double VideoDecoder::frameRate(const QString &filePath)
{
    AVFormatContext *fmtRaw = nullptr;
    if (avformat_open_input(&fmtRaw, filePath.toUtf8().constData(), nullptr, nullptr) < 0)
        return 0;
    std::unique_ptr<AVFormatContext, AVFormatCtxDeleter> fmt(fmtRaw);

    if (avformat_find_stream_info(fmt.get(), nullptr) < 0)
        return 0;

    int streamIdx = -1;
    for (unsigned i = 0; i < fmt->nb_streams; ++i) {
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            streamIdx = static_cast<int>(i);
            break;
        }
    }
    if (streamIdx < 0) return 0;

    AVStream *stream = fmt->streams[streamIdx];
    AVRational r = av_guess_frame_rate(fmt.get(), stream, nullptr);
    if (r.den > 0 && r.num > 0)
        return static_cast<double>(r.num) / r.den;
    r = stream->avg_frame_rate;
    if (r.den > 0 && r.num > 0)
        return av_q2d(r);
    r = stream->r_frame_rate;
    if (r.den > 0 && r.num > 0)
        return av_q2d(r);
    return 0;
}
