#pragma once

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>

class AudioPlayback : public QObject {
    Q_OBJECT
public:
    explicit AudioPlayback(QObject *parent = nullptr);

    void loadFile(const QString &path);
    void play();
    void pause();
    void togglePlayPause();
    void seekToFrame(qint64 frame, int fps);
    qint64 currentFrame(int fps) const;
    qint64 durationMs() const;
    bool isPlaying() const;

signals:
    void positionChanged(qint64 framePos);
    void durationChanged(qint64 totalMs);
    void playbackStateChanged(bool playing);

private slots:
    void onMediaPositionChanged(qint64 ms);
    void onMediaDurationChanged(qint64 ms);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState st);

private:
    QMediaPlayer *m_player      = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    int           m_fps         = 30;

public:
    void setFps(int fps) { m_fps = fps; }
};
