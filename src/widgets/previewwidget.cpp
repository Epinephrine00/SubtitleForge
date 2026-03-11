#include "widgets/previewwidget.h"
#include "model/subtitlerenderer.h"
#include <QPainter>

PreviewWidget::PreviewWidget(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(270, 480);
}

void PreviewWidget::setProject(const Project *project)
{
    m_project = project;
    update();
}

void PreviewWidget::setCurrentFrame(qint64 frame)
{
    m_frame = frame;
    update();
}

void PreviewWidget::setVideoFrame(const QImage &frame)
{
    m_videoFrame = frame;
    update();
}

void PreviewWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(40, 40, 40));

    if (!m_project) return;

    const int pw = m_project->videoWidth();   // 1080
    const int ph = m_project->videoHeight();  // 1920

    QImage frame = SubtitleRenderer::renderFrame(
        *m_project, m_frame, m_videoFrame, pw, ph);

    // Scale to fit widget keeping 9:16
    double aspect = static_cast<double>(pw) / ph;
    int previewW = width();
    int previewH = static_cast<int>(previewW / aspect);
    if (previewH > height()) {
        previewH = height();
        previewW = static_cast<int>(previewH * aspect);
    }

    int x = (width()  - previewW) / 2;
    int y = (height() - previewH) / 2;
    painter.drawImage(QRect(x, y, previewW, previewH), frame);

    int fps = m_project->fps();
    double sec = static_cast<double>(m_frame) / std::max(1, fps);
    int mins = static_cast<int>(sec) / 60;
    double s = sec - mins * 60;
    painter.setPen(Qt::white);
    painter.setFont(QFont("monospace", 9));
    painter.drawText(x + 6, y + 16,
                     QString("Frame %1  |  %2:%3")
                         .arg(m_frame)
                         .arg(mins)
                         .arg(s, 0, 'f', 2));
}
