#pragma once

#include <QImage>
#include <QPainter>
#include "model/project.h"
#include "model/animationengine.h"

class SubtitleRenderer {
public:
    // Render a complete frame: black bg + blurred video fill + center video
    // + title (if any) + subtitles. videoFrame can be null (black + text only).
    static QImage renderFrame(const Project &project,
                              qint64 frame,
                              const QImage &videoFrame,
                              int width, int height);

private:
    static QImage renderSubtitleText(const QString &text,
                                     const QFont &globalFont,
                                     float fontSize,
                                     const QColor &color,
                                     bool outlineEnabled,
                                     const QColor &outlineColor,
                                     float outlineWidthPx,
                                     const InterpolatedParams &ip,
                                     int viewW, int viewH);

    static void drawTitle(QPainter &painter,
                          const VideoTitle &title,
                          int viewW, int viewH);

    static QImage applyBlur(const QImage &src, qreal radius);
};
