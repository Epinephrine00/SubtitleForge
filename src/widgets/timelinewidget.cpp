#include "widgets/timelinewidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <algorithm>
#include <cmath>

TimelineWidget::TimelineWidget(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMinimumHeight(kRulerHeight + kWaveformHeight + kMarkerHeight + 10);
}

void TimelineWidget::setWaveform(const AudioDecoder::WaveformData &data)
{
    m_waveform = data;
    update();
}

void TimelineWidget::setSubtitles(const QVector<SubtitleEntry> &subs)
{
    m_subs = subs;
    update();
}

void TimelineWidget::setCurrentFrame(qint64 frame)
{
    m_currentFrame = frame;

    // auto-scroll to keep playback cursor visible
    int cx = xAtFrame(frame);
    if (cx < 0 || cx > width()) {
        m_scrollOffset = std::max<qint64>(0,
            frame - static_cast<qint64>(width() * 0.2 / m_pixelsPerFrame));
    }

    update();
}

void TimelineWidget::setTotalFrames(qint64 total)
{
    m_totalFrames = total;
    update();
}

void TimelineWidget::setFps(int fps) { m_fps = fps; }

void TimelineWidget::setTrimRange(qint64 trimStartFrames, qint64 sourceTotalFrames)
{
    m_trimStart   = trimStartFrames;
    m_sourceTotal = sourceTotalFrames;
    update();
}

void TimelineWidget::clearSelection()
{
    m_selectedId = -1;
    emit subtitleDeselected();
    update();
}

// --- coordinate helpers ---

qint64 TimelineWidget::frameAtX(int x) const
{
    return static_cast<qint64>((x / m_pixelsPerFrame) + m_scrollOffset);
}

int TimelineWidget::xAtFrame(qint64 frame) const
{
    return static_cast<int>((frame - m_scrollOffset) * m_pixelsPerFrame);
}

// --- painting ---

void TimelineWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(30, 30, 30));

    const int w = width();
    const int h = height();

    // ---- ruler ----
    p.setPen(QColor(180, 180, 180));
    p.setFont(QFont("monospace", 8));
    {
        qint64 firstFrame = frameAtX(0);
        qint64 lastFrame  = frameAtX(w);
        int stepFrames = std::max(1, static_cast<int>(60.0 / m_pixelsPerFrame));
        for (int nice : {1, 5, 10, 15, 30, 60, 150, 300, 600, 1800, 3600}) {
            int nf = nice * m_fps;
            if (nf * m_pixelsPerFrame >= 60) { stepFrames = nf; break; }
        }

        qint64 start = (firstFrame / stepFrames) * stepFrames;
        for (qint64 f = start; f <= lastFrame; f += stepFrames) {
            int x = xAtFrame(f);
            p.drawLine(x, 0, x, kRulerHeight);
            double sec = static_cast<double>(f) / m_fps;
            int m = static_cast<int>(sec) / 60;
            double s = sec - m * 60;
            p.drawText(x + 3, kRulerHeight - 4,
                       QString("%1:%2").arg(m).arg(s, 0, 'f', 1));
        }
    }

    // Current / Total time (right side of ruler)
    double curSec = static_cast<double>(m_currentFrame) / std::max(1, m_fps);
    double totalSec = static_cast<double>(m_totalFrames) / std::max(1, m_fps);
    int curM = static_cast<int>(curSec) / 60;
    double curS = curSec - curM * 60;
    int totalM = static_cast<int>(totalSec) / 60;
    double totalS = totalSec - totalM * 60;
    QString timeLabel = QString("%1:%2 / %3:%4")
                            .arg(curM).arg(curS, 0, 'f', 2)
                            .arg(totalM).arg(totalS, 0, 'f', 2);
    p.drawText(w - 120, kRulerHeight - 4, timeLabel);

    // ---- waveform ----
    if (m_waveform.valid && m_waveform.peaks.size() >= 2 && m_sourceTotal > 0) {
        int peakCount   = m_waveform.peaks.size() / 2;
        int waveTop     = kRulerHeight;
        int waveMid     = waveTop + kWaveformHeight / 2;

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 180, 100, 180));

        for (int px = 0; px < w; ++px) {
            qint64 f = frameAtX(px);
            if (f < 0 || f >= m_totalFrames) continue;
            qint64 sourceFrame = m_trimStart + f;
            if (sourceFrame >= m_sourceTotal) continue;
            double frac = static_cast<double>(sourceFrame) / m_sourceTotal;
            int idx = static_cast<int>(frac * peakCount);
            idx = std::clamp(idx, 0, peakCount - 1);
            float lo = m_waveform.peaks[idx * 2];
            float hi = m_waveform.peaks[idx * 2 + 1];
            int y1 = waveMid + static_cast<int>(lo * kWaveformHeight / 2);
            int y2 = waveMid + static_cast<int>(hi * kWaveformHeight / 2);
            p.drawRect(px, y1, 1, y2 - y1);
        }
    }

    // ---- subtitle markers ----
    int markerTop = kRulerHeight + kWaveformHeight + 2;
    for (const auto &sub : m_subs) {
        int x = xAtFrame(sub.keyFrame);
        bool selected = (sub.id == m_selectedId);

        QColor c = selected ? QColor(255, 200, 50) : QColor(100, 150, 255);
        p.setPen(QPen(c, selected ? 2 : 1));
        p.drawLine(x, kRulerHeight, x, markerTop + kMarkerHeight);

        p.setBrush(c);
        QRect mr(x - 4, markerTop, 8, kMarkerHeight);
        p.drawRoundedRect(mr, 2, 2);

        // label
        p.setPen(Qt::black);
        p.setFont(QFont("sans-serif", 7));
        QString label = sub.text.left(8);
        p.drawText(mr.adjusted(1, 1, -1, -1), Qt::AlignCenter, label);
    }

    // ---- playback cursor ----
    {
        int cx = xAtFrame(m_currentFrame);
        p.setPen(QPen(Qt::red, 2));
        p.drawLine(cx, 0, cx, h);
    }
}

// --- mouse ---

void TimelineWidget::mousePressEvent(QMouseEvent *e)
{
    setFocus();
    if (e->button() != Qt::LeftButton) return;

    int mx = static_cast<int>(e->position().x());

    // check if clicking on a subtitle marker
    int markerTop = kRulerHeight + kWaveformHeight + 2;
    if (e->position().y() >= markerTop) {
        for (const auto &sub : m_subs) {
            int sx = xAtFrame(sub.keyFrame);
            if (std::abs(mx - sx) < 8) {
                m_selectedId    = sub.id;
                m_draggingId    = sub.id;
                m_dragStartX    = mx;
                m_dragOrigFrame = sub.keyFrame;
                emit subtitleSelected(sub.id);
                update();
                return;
            }
        }
    }

    // clicking elsewhere: seek + start cursor drag
    qint64 frame = std::max<qint64>(0, frameAtX(mx));
    m_currentFrame = frame;
    m_draggingCursor = true;
    emit frameChanged(frame);
    m_selectedId = -1;
    emit subtitleDeselected();
    update();
}

void TimelineWidget::mouseMoveEvent(QMouseEvent *e)
{
    int mx = static_cast<int>(e->position().x());

    if (m_draggingCursor) {
        qint64 frame = std::clamp<qint64>(frameAtX(mx), 0, std::max<qint64>(1, m_totalFrames - 1));
        m_currentFrame = frame;
        emit frameChanged(frame);
        update();
        return;
    }

    if (m_draggingId < 0) return;
    int dx   = mx - m_dragStartX;
    qint64 dFrames = static_cast<qint64>(dx / m_pixelsPerFrame);
    qint64 newFrame = std::max<qint64>(0, m_dragOrigFrame + dFrames);
    for (auto &sub : m_subs) {
        if (sub.id == m_draggingId) {
            sub.keyFrame = newFrame;
            break;
        }
    }
    update();
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent *)
{
    m_draggingCursor = false;

    if (m_draggingId >= 0) {
        for (const auto &sub : m_subs) {
            if (sub.id == m_draggingId) {
                emit subtitleMoved(m_draggingId, sub.keyFrame);
                break;
            }
        }
        m_draggingId = -1;
    }
}

void TimelineWidget::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::ShiftModifier) {
        // horizontal scroll
        double delta = e->angleDelta().y() / 120.0;
        qint64 scrollStep = static_cast<qint64>(50.0 / m_pixelsPerFrame);
        m_scrollOffset = std::max<qint64>(0,
            m_scrollOffset - static_cast<qint64>(delta * scrollStep));
    } else {
        // zoom around mouse position
        double delta = e->angleDelta().y() / 120.0;
        double factor = std::pow(1.15, delta);
        double mouseFrame = frameAtX(static_cast<int>(e->position().x()));
        m_pixelsPerFrame = std::clamp(m_pixelsPerFrame * factor, 0.05, 50.0);
        m_scrollOffset = static_cast<qint64>(
            mouseFrame - e->position().x() / m_pixelsPerFrame);
        m_scrollOffset = std::max<qint64>(0, m_scrollOffset);
    }
    update();
}

void TimelineWidget::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Space:
        emit playPauseRequested();
        break;
    case Qt::Key_Right:
        if (e->modifiers() & Qt::ControlModifier)
            emit stepForwardLarge();
        else
            emit stepForward();
        break;
    case Qt::Key_Left:
        if (e->modifiers() & Qt::ControlModifier)
            emit stepBackwardLarge();
        else
            emit stepBackward();
        break;
    case Qt::Key_Delete:
        if (m_selectedId >= 0)
            emit deleteSubtitleRequested(m_selectedId);
        break;
    default:
        QWidget::keyPressEvent(e);
    }
}
