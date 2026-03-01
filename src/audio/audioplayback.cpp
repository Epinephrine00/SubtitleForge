#include "audio/audioplayback.h"
#include <QUrl>

AudioPlayback::AudioPlayback(QObject *parent)
    : QObject(parent)
    , m_player(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
{
    m_player->setAudioOutput(m_audioOutput);

    connect(m_player, &QMediaPlayer::positionChanged,
            this, &AudioPlayback::onMediaPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged,
            this, &AudioPlayback::onMediaDurationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged,
            this, &AudioPlayback::onPlaybackStateChanged);
}

void AudioPlayback::loadFile(const QString &path)
{
    m_player->setSource(QUrl::fromLocalFile(path));
}

void AudioPlayback::play()
{
    m_player->play();
}

void AudioPlayback::pause()
{
    m_player->pause();
}

void AudioPlayback::togglePlayPause()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState)
        pause();
    else
        play();
}

void AudioPlayback::seekToFrame(qint64 frame, int fps)
{
    qint64 ms = frame * 1000 / fps;
    m_player->setPosition(ms);
}

qint64 AudioPlayback::currentFrame(int fps) const
{
    return m_player->position() * fps / 1000;
}

qint64 AudioPlayback::durationMs() const
{
    return m_player->duration();
}

bool AudioPlayback::isPlaying() const
{
    return m_player->playbackState() == QMediaPlayer::PlayingState;
}

void AudioPlayback::onMediaPositionChanged(qint64 ms)
{
    qint64 frame = ms * m_fps / 1000;
    emit positionChanged(frame);
}

void AudioPlayback::onMediaDurationChanged(qint64 ms)
{
    emit durationChanged(ms);
}

void AudioPlayback::onPlaybackStateChanged(QMediaPlayer::PlaybackState st)
{
    emit playbackStateChanged(st == QMediaPlayer::PlayingState);
}
