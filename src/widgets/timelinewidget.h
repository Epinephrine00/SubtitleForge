#pragma once

#include <QWidget>
#include <QVector>
#include "audio/audiodecoder.h"
#include "model/subtitle.h"

class TimelineWidget : public QWidget {
    Q_OBJECT
public:
    explicit TimelineWidget(QWidget *parent = nullptr);

    void setWaveform(const AudioDecoder::WaveformData &data);
    void setSubtitles(const QVector<SubtitleEntry> &subs);
    void setCurrentFrame(qint64 frame);
    void setTotalFrames(qint64 total);
    void setFps(int fps);
    void setTrimRange(qint64 trimStartFrames, qint64 sourceTotalFrames);
    qint64 currentFrame() const { return m_currentFrame; }
    int  selectedSubtitleId() const { return m_selectedId; }
    void clearSelection();

signals:
    void frameChanged(qint64 frame);
    void subtitleSelected(int id);
    void subtitleDeselected();
    void subtitleMoved(int id, qint64 newKeyFrame);
    void playPauseRequested();
    void stepForward();
    void stepBackward();
    void stepForwardLarge();
    void stepBackwardLarge();
    void deleteSubtitleRequested(int id);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    QSize sizeHint() const override { return {800, 120}; }

private:
    qint64 frameAtX(int x) const;
    int    xAtFrame(qint64 frame) const;

    AudioDecoder::WaveformData m_waveform;
    QVector<SubtitleEntry> m_subs;

    qint64 m_currentFrame = 0;
    qint64 m_totalFrames  = 0;
    qint64 m_trimStart    = 0;
    qint64 m_sourceTotal  = 0;
    int    m_fps          = 30;

    // view
    double m_pixelsPerFrame = 4.0;
    qint64 m_scrollOffset   = 0;   // in frames

    // interaction
    int    m_selectedId    = -1;
    int    m_draggingId    = -1;
    int    m_dragStartX    = 0;
    qint64 m_dragOrigFrame = 0;
    bool   m_draggingCursor = false;

    static constexpr int kWaveformHeight = 60;
    static constexpr int kMarkerHeight   = 30;
    static constexpr int kRulerHeight    = 20;
};
