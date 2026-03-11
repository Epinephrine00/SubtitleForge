#include "video/previewdecoder.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/rational.h>
#include <libswscale/swscale.h>
}

#include <QPair>
#include <cmath>

namespace {

struct DecoderState {
    AVFormatContext *fmt = nullptr;
    AVCodecContext *dec = nullptr;
    int streamIdx = -1;
    AVStream *stream = nullptr;
    AVPacket *pkt = nullptr;
    AVFrame *frame = nullptr;
    SwsContext *sws = nullptr;
    QString path;
    double lastDecodedTimeSec = -1.0;  // for sequential decode

    void close() {
        if (sws) { sws_freeContext(sws); sws = nullptr; }
        if (frame) { av_frame_free(&frame); frame = nullptr; }
        if (pkt) { av_packet_free(&pkt); pkt = nullptr; }
        if (dec) { avcodec_free_context(&dec); dec = nullptr; }
        if (fmt) { avformat_close_input(&fmt); fmt = nullptr; }
        streamIdx = -1;
        stream = nullptr;
        path.clear();
        lastDecodedTimeSec = -1.0;
    }

    bool open(const QString &filePath) {
        close();
        if (filePath.isEmpty()) return false;
        if (avformat_open_input(&fmt, filePath.toUtf8().constData(), nullptr, nullptr) < 0)
            return false;
        if (avformat_find_stream_info(fmt, nullptr) < 0) {
            avformat_close_input(&fmt);
            fmt = nullptr;
            return false;
        }
        for (unsigned i = 0; i < fmt->nb_streams; ++i) {
            if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                streamIdx = static_cast<int>(i);
                stream = fmt->streams[streamIdx];
                break;
            }
        }
        if (streamIdx < 0 || !stream) {
            avformat_close_input(&fmt);
            fmt = nullptr;
            return false;
        }
        const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!codec) {
            avformat_close_input(&fmt);
            fmt = nullptr;
            return false;
        }
        dec = avcodec_alloc_context3(codec);
        if (!dec || avcodec_parameters_to_context(dec, stream->codecpar) < 0 ||
            avcodec_open2(dec, codec, nullptr) < 0) {
            if (dec) avcodec_free_context(&dec);
            avformat_close_input(&fmt);
            fmt = nullptr;
            dec = nullptr;
            return false;
        }
        pkt = av_packet_alloc();
        frame = av_frame_alloc();
        if (!pkt || !frame) {
            close();
            return false;
        }
        path = filePath;
        return true;
    }

    // Decode next frame without seeking (sequential). Returns (timeSec, image). timeSec < 0 if no frame.
    QPair<double, QImage> decodeNextFrame() {
        QPair<double, QImage> out(-1.0, QImage());
        if (!fmt || !dec || streamIdx < 0 || !pkt || !frame) return out;
        while (av_read_frame(fmt, pkt) >= 0) {
            if (pkt->stream_index != streamIdx) {
                av_packet_unref(pkt);
                continue;
            }
            if (avcodec_send_packet(dec, pkt) < 0) {
                av_packet_unref(pkt);
                continue;
            }
            av_packet_unref(pkt);
            if (avcodec_receive_frame(dec, frame) == 0) {
                double sec = (frame->pts == AV_NOPTS_VALUE) ? lastDecodedTimeSec + 0.033
                    : frame->pts * av_q2d(stream->time_base);
                lastDecodedTimeSec = sec;
                if (!sws)
                    sws = sws_getContext(
                        frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
                        frame->width, frame->height, AV_PIX_FMT_BGRA,
                        SWS_BILINEAR, nullptr, nullptr, nullptr);
                if (!sws) return out;
                QImage img(frame->width, frame->height, QImage::Format_ARGB32);
                int dstStride = img.bytesPerLine();
                uint8_t *dstData = img.bits();
                sws_scale(sws, frame->data, frame->linesize, 0, frame->height, &dstData, &dstStride);
                return qMakePair(sec, img);
            }
        }
        return out;
    }

    // Seek to time and decode one frame (used when position jumps).
    QImage decodeAt(double timeSec) {
        if (!fmt || !dec || streamIdx < 0 || !pkt || !frame) return QImage();
        int64_t seekTarget = static_cast<int64_t>(timeSec * stream->time_base.den / stream->time_base.num);
        seekTarget = std::max<int64_t>(0, seekTarget);
        if (av_seek_frame(fmt, streamIdx, seekTarget, AVSEEK_FLAG_BACKWARD) < 0)
            av_seek_frame(fmt, streamIdx, 0, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(dec);
        lastDecodedTimeSec = -1.0;

        auto p = decodeNextFrame();
        lastDecodedTimeSec = p.first;
        return p.second;
    }
};

} // namespace

PreviewDecoder::PreviewDecoder(QObject *parent)
    : QObject(parent)
{
}

PreviewDecoder::~PreviewDecoder()
{
    QMutexLocker lock(&m_mutex);
    m_quit = true;
    m_cond.wakeAll();
}

void PreviewDecoder::setFile(const QString &path)
{
    QMutexLocker lock(&m_mutex);
    m_path = path;
    m_pathChanged = true;
    m_hasRequest = false;
    m_cond.wakeAll();
}

void PreviewDecoder::requestFrame(qint64 frameIndex, double timeSec)
{
    QMutexLocker lock(&m_mutex);
    m_requestedFrame = frameIndex;
    m_requestedTimeSec = timeSec;
    m_hasRequest = true;
    m_cond.wakeAll();
}

void PreviewDecoder::requestQuit()
{
    QMutexLocker lock(&m_mutex);
    m_quit = true;
    m_cond.wakeAll();
}

void PreviewDecoder::runDecoder()
{
    DecoderState state;
    QString currentPath;
    for (;;) {
        qint64 reqFrame = -1;
        double reqTime = -1.0;
        {
            QMutexLocker lock(&m_mutex);
            while (!m_quit && !m_pathChanged && !m_hasRequest)
                m_cond.wait(&m_mutex);
            if (m_quit) break;
            if (m_pathChanged) {
                currentPath = m_path;
                m_pathChanged = false;
                m_hasRequest = false;
            }
            if (m_hasRequest) {
                reqFrame = m_requestedFrame;
                reqTime = m_requestedTimeSec;
                m_hasRequest = false;
            }
        }
        if (currentPath.isEmpty()) {
            state.close();
        } else if (state.path != currentPath) {
            state.open(currentPath);
        }
        if (reqFrame >= 0 && reqTime >= 0 && !currentPath.isEmpty() && state.path == currentPath) {
            QImage img;
            const double tol = 0.05;
            bool needSeek = state.lastDecodedTimeSec < 0
                || reqTime < state.lastDecodedTimeSec - tol
                || reqTime > state.lastDecodedTimeSec + 0.5;
            if (needSeek)
                img = state.decodeAt(reqTime);
            else {
                while (state.lastDecodedTimeSec < reqTime - 0.001) {
                    auto p = state.decodeNextFrame();
                    if (p.first < 0) break;
                    if (p.first >= reqTime - 0.001) { img = p.second; break; }
                }
                if (img.isNull()) {
                    auto p = state.decodeNextFrame();
                    img = p.second;
                }
            }
            emit frameDecoded(reqFrame, img);
        }
    }
    state.close();
}
