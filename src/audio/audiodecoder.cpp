#include "audio/audiodecoder.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

#include <algorithm>
#include <cmath>
#include <memory>

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
struct SwrCtxDeleter {
    void operator()(SwrContext *p) const {
        if (p) swr_free(&p);
    }
};
struct AVFrameDeleter {
    void operator()(AVFrame *p) const {
        if (p) av_frame_free(&p);
    }
};
struct AVPacketDeleter {
    void operator()(AVPacket *p) const {
        if (p) av_packet_free(&p);
    }
};

AudioDecoder::WaveformData
AudioDecoder::decodeForWaveform(const QString &filePath, int targetPeaks)
{
    WaveformData result;

    AVFormatContext *fmtRaw = nullptr;
    if (avformat_open_input(&fmtRaw, filePath.toUtf8().constData(),
                            nullptr, nullptr) < 0)
        return result;
    std::unique_ptr<AVFormatContext, AVFormatCtxDeleter> fmt(fmtRaw);

    if (avformat_find_stream_info(fmt.get(), nullptr) < 0)
        return result;

    int streamIdx = -1;
    for (unsigned i = 0; i < fmt->nb_streams; ++i) {
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            streamIdx = static_cast<int>(i);
            break;
        }
    }
    if (streamIdx < 0) return result;

    AVCodecParameters *par = fmt->streams[streamIdx]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(par->codec_id);
    if (!codec) return result;

    std::unique_ptr<AVCodecContext, AVCodecCtxDeleter>
        dec(avcodec_alloc_context3(codec));
    avcodec_parameters_to_context(dec.get(), par);
    if (avcodec_open2(dec.get(), codec, nullptr) < 0) return result;

    // Resample everything to mono float
    SwrContext *swrRaw = nullptr;
    AVChannelLayout monoLayout = AV_CHANNEL_LAYOUT_MONO;
    swr_alloc_set_opts2(&swrRaw,
                        &monoLayout, AV_SAMPLE_FMT_FLT, dec->sample_rate,
                        &dec->ch_layout, dec->sample_fmt, dec->sample_rate,
                        0, nullptr);
    if (!swrRaw) return result;
    std::unique_ptr<SwrContext, SwrCtxDeleter> swr(swrRaw);
    if (swr_init(swr.get()) < 0) return result;

    result.sampleRate = dec->sample_rate;
    result.channels   = dec->ch_layout.nb_channels;

    // Collect all mono samples
    QVector<float> allSamples;
    allSamples.reserve(result.sampleRate * 300); // ~5 min estimate

    std::unique_ptr<AVPacket, AVPacketDeleter> pkt(av_packet_alloc());
    std::unique_ptr<AVFrame, AVFrameDeleter>   frm(av_frame_alloc());

    while (av_read_frame(fmt.get(), pkt.get()) >= 0) {
        if (pkt->stream_index != streamIdx) {
            av_packet_unref(pkt.get());
            continue;
        }

        if (avcodec_send_packet(dec.get(), pkt.get()) < 0) {
            av_packet_unref(pkt.get());
            continue;
        }

        while (avcodec_receive_frame(dec.get(), frm.get()) == 0) {
            int outSamples = swr_get_out_samples(swr.get(), frm->nb_samples);
            QVector<float> buf(outSamples);
            uint8_t *outBuf = reinterpret_cast<uint8_t *>(buf.data());
            int got = swr_convert(swr.get(), &outBuf, outSamples,
                                  const_cast<const uint8_t **>(frm->extended_data),
                                  frm->nb_samples);
            if (got > 0) {
                buf.resize(got);
                allSamples.append(buf);
            }
            av_frame_unref(frm.get());
        }
        av_packet_unref(pkt.get());
    }

    // Flush decoder
    avcodec_send_packet(dec.get(), nullptr);
    while (avcodec_receive_frame(dec.get(), frm.get()) == 0) {
        int outSamples = swr_get_out_samples(swr.get(), frm->nb_samples);
        QVector<float> buf(outSamples);
        uint8_t *outBuf = reinterpret_cast<uint8_t *>(buf.data());
        int got = swr_convert(swr.get(), &outBuf, outSamples,
                              const_cast<const uint8_t **>(frm->extended_data),
                              frm->nb_samples);
        if (got > 0) {
            buf.resize(got);
            allSamples.append(buf);
        }
        av_frame_unref(frm.get());
    }

    if (allSamples.isEmpty()) return result;

    result.durationSec = static_cast<double>(allSamples.size()) / result.sampleRate;

    // Downsample to peaks (min/max pairs)
    qsizetype total = allSamples.size();
    qsizetype samplesPerPeak = std::max(qsizetype(1), total / targetPeaks);
    result.peaks.reserve(targetPeaks * 2);

    for (qsizetype i = 0; i < total; i += samplesPerPeak) {
        qsizetype end = std::min(i + samplesPerPeak, total);
        float lo =  1.0f;
        float hi = -1.0f;
        for (qsizetype j = i; j < end; ++j) {
            lo = std::min(lo, allSamples[j]);
            hi = std::max(hi, allSamples[j]);
        }
        result.peaks.append(lo);
        result.peaks.append(hi);
    }

    result.valid = true;
    return result;
}
