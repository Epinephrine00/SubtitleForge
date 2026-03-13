#include "model/subtitlerenderer.h"

#include <QPainter>
#include <QPainterPath>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsBlurEffect>
#include <cmath>

// ---------------------------------------------------------------------------
// Gaussian blur
// ---------------------------------------------------------------------------
QImage SubtitleRenderer::applyBlur(const QImage &src, qreal radius)
{
    if (radius < 0.5) return src;

    QGraphicsScene scene;
    auto *item = new QGraphicsPixmapItem(QPixmap::fromImage(src));
    auto *fx   = new QGraphicsBlurEffect;
    fx->setBlurRadius(radius);
    fx->setBlurHints(QGraphicsBlurEffect::QualityHint);
    item->setGraphicsEffect(fx);
    scene.addItem(item);

    QRectF bounds(0, 0, src.width(), src.height());
    scene.setSceneRect(bounds);

    QImage out(src.size(), QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);
    QPainter p(&out);
    scene.render(&p, QRectF(out.rect()), bounds);
    return out;
}

// ---------------------------------------------------------------------------
// Render one subtitle's text (global font/size/color)
// ---------------------------------------------------------------------------
QImage SubtitleRenderer::renderSubtitleText(const QString &text,
                                            const QFont &globalFont,
                                            float fontSize,
                                            const QColor &color,
                                            bool outlineEnabled,
                                            const QColor &outlineColor,
                                            float outlineWidthPx,
                                            const InterpolatedParams &ip,
                                            int /*viewW*/, int /*viewH*/)
{
    QFont font = globalFont;
    font.setPointSizeF(ip.fontSize > 0 ? ip.fontSize : fontSize);

    QFontMetricsF fm(font);
    const QStringList lines = text.split('\n');

    qreal textW = 0;
    for (const auto &line : lines)
        textW = std::max(textW, fm.horizontalAdvance(line));

    qreal lineH = fm.height();
    qreal textH = lineH * lines.size();
    qreal stroke = outlineEnabled && outlineWidthPx > 0 ? outlineWidthPx : 0;
    int   pad   = static_cast<int>(std::ceil(stroke * 2 + ip.blur + 4));

    int imgW = static_cast<int>(std::ceil(textW)) + pad * 2;
    int imgH = static_cast<int>(std::ceil(textH)) + pad * 2;

    QImage img(imgW, imgH, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath path;
    qreal y = pad + fm.ascent();
    for (const auto &line : lines) {
        qreal x = pad + (textW - fm.horizontalAdvance(line)) / 2.0;  // center
        path.addText(x, y, font, line);
        y += lineH;
    }

    if (outlineEnabled && outlineWidthPx > 0) {
        painter.setPen(QPen(outlineColor, outlineWidthPx * 2,
                            Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.fillPath(path, color);
    painter.end();

    return img;
}

// ---------------------------------------------------------------------------
// Draw video title (multiline, centered)
// ---------------------------------------------------------------------------
void SubtitleRenderer::drawTitle(QPainter &painter,
                                  const VideoTitle &title,
                                  int viewW, int viewH)
{
    if (title.text.trimmed().isEmpty()) return;

    QFont font = title.font;
    font.setPointSizeF(title.fontSize);
    painter.setFont(font);

    QFontMetricsF fm(font);
    const QStringList lines = title.text.trimmed().split('\n');
    qreal lineH = fm.height();
    qreal totalH = lineH * lines.size();
    qreal y = (viewH - totalH) / 2.0 + title.posY + fm.ascent();

    QPainterPath path;
    for (const auto &line : lines) {
        qreal x = (viewW - fm.horizontalAdvance(line)) / 2.0;
        path.addText(x, y, font, line);
        y += lineH;
    }

    if (title.outlineEnabled && title.outlineWidthPx > 0) {
        painter.setPen(QPen(title.outlineColor, title.outlineWidthPx * 2,
                            Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(title.color);
    painter.drawPath(path);
}

// ---------------------------------------------------------------------------
// Compose full frame: black + blurred video fill + center video + title + subtitles
// ---------------------------------------------------------------------------
QImage SubtitleRenderer::renderFrame(const Project &project,
                                     qint64 frame,
                                     const QImage &videoFrame,
                                     int width, int height)
{
    QImage canvas(width, height, QImage::Format_ARGB32_Premultiplied);
    canvas.fill(Qt::black);

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (!videoFrame.isNull()) {
        int vw = videoFrame.width();
        int vh = videoFrame.height();
        if (vw > 0 && vh > 0) {
            // 1) Blurred fill: scale video to height = height, crop center width
            double fillScale = static_cast<double>(height) / vh;
            int fillW = static_cast<int>(std::ceil(vw * fillScale));
            int fillH = height;
            QImage fillImg = videoFrame.scaled(fillW, fillH, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            int cropX = (fillImg.width() - width) / 2;
            if (cropX > 0)
                fillImg = fillImg.copy(cropX, 0, width, fillH);
            else if (fillImg.width() > width)
                fillImg = fillImg.copy(0, 0, width, fillH);

            fillImg = applyBlur(fillImg, 80);
            painter.setOpacity(0.35);
            painter.drawImage(0, 0, fillImg);
            painter.setOpacity(1.0);

            // 2) Center video: scale to fit width
            double centerScale = static_cast<double>(width) / vw;
            int cw = width;
            int ch = static_cast<int>(std::ceil(vh * centerScale));
            int cy = (height - ch) / 2;
            QImage centerImg = videoFrame.scaled(cw, ch, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            painter.drawImage(0, cy, centerImg);
        }
    }

    // 3) Title (full duration)
    drawTitle(painter, project.videoTitle(), width, height);

    // 4) Subtitles (focus-only, global style)
    auto visible = AnimationEngine::calculateAll(project, frame);
    const QFont &globalFont = project.globalSubtitleFont();
    float globalFontSize = project.globalSubtitleFontSize();
    const QColor &globalColor = project.globalSubtitleColor();
    bool subOutline = project.globalSubtitleOutlineEnabled();
    const QColor &subOutlineColor = project.globalSubtitleOutlineColor();
    float subOutlinePx = project.globalSubtitleOutlineWidthPx();

    for (const auto &[id, ip] : visible) {
        const SubtitleEntry *sub = project.subtitleById(id);
        if (!sub) continue;

        QImage txt = renderSubtitleText(sub->text, globalFont, globalFontSize, globalColor,
                                        subOutline, subOutlineColor, subOutlinePx, ip, width, height);

        if (ip.blur > 0.5f)
            txt = applyBlur(txt, ip.blur);

        qreal scaleF = ip.scale / 100.0;
        qreal drawW  = txt.width() * scaleF;
        qreal drawH  = txt.height() * scaleF;
        qreal cx     = width  / 2.0 + ip.posX - drawW / 2.0;
        qreal cy     = height / 2.0 + ip.posY - drawH / 2.0;

        painter.save();
        painter.setOpacity(std::clamp(ip.opacity / 100.0f, 0.0f, 1.0f));
        painter.drawImage(QRectF(cx, cy, drawW, drawH), txt);
        painter.restore();
    }

    painter.end();
    return canvas;
}
