#pragma once

#include <QImage>
#include "model/project.h"
#include "model/animationengine.h"

class SubtitleRenderer {
public:
    // Render a complete frame (green bg + all visible subtitles) at the
    // given resolution.  Used by both the preview widget and the video
    // exporter.
    static QImage renderFrame(const Project &project,
                              qint64 frame,
                              int width, int height,
                              bool transparentBg = false);

private:
    static QImage renderText(const SubtitleEntry &entry,
                             const InterpolatedParams &ip,
                             int viewW, int viewH);

    static QImage applyBlur(const QImage &src, qreal radius);
};
