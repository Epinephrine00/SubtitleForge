#pragma once

#include <QString>
#include <QImage>
#include <QSize>

class VideoDecoder {
public:
    // Decode a single video frame at the given time (seconds).
    // Returns null image on failure.
    static QImage frameAt(const QString &filePath, double timeSec);

    // Get video duration in seconds. Returns < 0 on failure.
    static double durationSec(const QString &filePath);

    // Get video size (width, height). Returns (0,0) on failure.
    static QSize videoSize(const QString &filePath);

    // Get video frame rate (e.g. 30.0, 60.0). Returns 0 on failure; use 60 as fallback.
    static double frameRate(const QString &filePath);
};
