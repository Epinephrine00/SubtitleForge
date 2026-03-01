#pragma once

#include <QVector>
#include <QString>

class AudioDecoder {
public:
    struct WaveformData {
        QVector<float> peaks;   // min/max interleaved pairs
        double durationSec = 0;
        int    sampleRate  = 0;
        int    channels    = 0;
        bool   valid       = false;
    };

    // Decode audio (or extract audio from video) and produce down-sampled
    // waveform peaks.  `targetPeaks` controls the resolution.
    static WaveformData decodeForWaveform(const QString &filePath,
                                          int targetPeaks = 8192);
};
