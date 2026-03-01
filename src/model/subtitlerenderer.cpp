#include "model/subtitlerenderer.h"

#include <QPainter>
#include <QPainterPath>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsBlurEffect>
#include <cmath>

// ---------------------------------------------------------------------------
// Gaussian blur via QGraphicsBlurEffect
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

    // Pin the scene rect to the source image bounds so that
    // scene.render() does not scale down due to the blur effect
    // expanding the item's bounding rect.
    QRectF bounds(0, 0, src.width(), src.height());
    scene.setSceneRect(bounds);

    QImage out(src.size(), QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);
    QPainter p(&out);
    scene.render(&p, QRectF(out.rect()), bounds);
    return out;
}

// ---------------------------------------------------------------------------
// Render one subtitle's text to an ARGB image
// ---------------------------------------------------------------------------
QImage SubtitleRenderer::renderText(const SubtitleEntry &entry,
                                    const InterpolatedParams &ip,
                                    int viewW, int viewH)
{
    QFont font = entry.font;
    font.setPointSizeF(ip.fontSize);

    QFontMetricsF fm(font);
    const QStringList lines = entry.text.split('\n');

    qreal textW = 0;
    for (const auto &line : lines)
        textW = std::max(textW, fm.horizontalAdvance(line));

    qreal lineH   = fm.height();
    qreal textH   = lineH * lines.size();
    qreal outline  = std::max(1.0, ip.fontSize * 0.04);
    int   pad      = static_cast<int>(std::ceil(outline * 2 + ip.blur + 4));

    int imgW = static_cast<int>(std::ceil(textW))  + pad * 2;
    int imgH = static_cast<int>(std::ceil(textH))  + pad * 2;

    QImage img(imgW, imgH, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath path;
    qreal y = pad + fm.ascent();
    for (const auto &line : lines) {
        qreal x = pad;
        if (entry.alignment & Qt::AlignHCenter)
            x = pad + (textW - fm.horizontalAdvance(line)) / 2.0;
        else if (entry.alignment & Qt::AlignRight)
            x = pad + textW - fm.horizontalAdvance(line);
        path.addText(x, y, font, line);
        y += lineH;
    }

    // black outline
    painter.setPen(QPen(Qt::black, outline * 2,
                        Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path);

    // fill
    painter.fillPath(path, entry.textColor);
    painter.end();

    return img;
}

// ---------------------------------------------------------------------------
// Compose a full frame
// ---------------------------------------------------------------------------
QImage SubtitleRenderer::renderFrame(const Project &project,
                                     qint64 frame,
                                     int width, int height,
                                     bool transparentBg)
{
    QImage canvas(width, height, QImage::Format_ARGB32_Premultiplied);
    canvas.fill(transparentBg ? Qt::transparent : QColor(0x00, 0xFF, 0x00));

    auto visible = AnimationEngine::calculateAll(project, frame);
    if (visible.isEmpty()) return canvas;

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    for (const auto &[id, ip] : visible) {
        const SubtitleEntry *sub = project.subtitleById(id);
        if (!sub) continue;

        QImage txt = renderText(*sub, ip, width, height);

        if (ip.blur > 0.5f)
            txt = applyBlur(txt, ip.blur);

        qreal scaleF  = ip.scale / 100.0;
        qreal drawW   = txt.width()  * scaleF;
        qreal drawH   = txt.height() * scaleF;
        qreal cx      = width  / 2.0 + ip.posX - drawW / 2.0;
        qreal cy      = height / 2.0 + ip.posY - drawH / 2.0;

        painter.setOpacity(std::clamp(ip.opacity / 100.0f, 0.0f, 1.0f));
        painter.drawImage(QRectF(cx, cy, drawW, drawH), txt);
    }

    painter.setOpacity(1.0);
    return canvas;
}
