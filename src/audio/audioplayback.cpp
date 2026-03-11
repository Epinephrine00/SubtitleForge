#include "audio/audioplayback.h"
#include <QUrl>
#include <QSettings>
#include <QFileInfo>
#include <QAudioDevice>
#include <QMediaDevices>

static const char kAudioVolumeKey[] = "audio/volume";

AudioPlayback::AudioPlayback(QObject *parent)
    : QObject(parent)
    , m_player(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
{
    m_player->setAudioOutput(m_audioOutput);
    m_audioOutput->setDevice(QMediaDevices::defaultAudioOutput());

    m_volume = static_cast<float>(QSettings().value(kAudioVolumeKey, 1.0).toDouble());
    m_volume = std::clamp(m_volume, 0.0f, 1.0f);
    m_audioOutput->setVolume(m_volume);

    connect(m_player, &QMediaPlayer::positionChanged,
            this, &AudioPlayback::onMediaPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged,
            this, &AudioPlayback::onMediaDurationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged,
            this, &AudioPlayback::onPlaybackStateChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged,
            this, &AudioPlayback::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::errorOccurred,
            this, &AudioPlayback::onError);
}

void AudioPlayback::loadFile(const QString &path)
{
    if (path.isEmpty()) {
        m_player->setSource(QUrl());
        return;
    }
    QString absPath = QFileInfo(path).absoluteFilePath();
    m_player->setSource(QUrl::fromLocalFile(absPath));
    m_audioOutput->setVolume(m_volume);
    m_audioOutput->setMuted(false);
}

void AudioPlayback::setVolume(float linear)
{
    m_volume = std::clamp(linear, 0.0f, 1.0f);
    m_audioOutput->setVolume(m_volume);
    QSettings().setValue(kAudioVolumeKey, static_cast<double>(m_volume));
}

void AudioPlayback::play()
{
    if (m_player->mediaStatus() == QMediaPlayer::NoMedia || m_player->mediaStatus() == QMediaPlayer::LoadingMedia) {
        m_playWhenLoaded = true;
        return;
    }
    m_playWhenLoaded = false;
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

void AudioPlayback::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) {
        m_audioOutput->setVolume(m_volume);
        m_audioOutput->setMuted(false);
        if (m_playWhenLoaded) {
            m_playWhenLoaded = false;
            m_player->play();
        }
    }
}

void AudioPlayback::onError(QMediaPlayer::Error error, const QString &detail)
{
    QString msg = detail;
    if (msg.isEmpty()) {
        switch (error) {
        case QMediaPlayer::NoError: return;
        case QMediaPlayer::ResourceError: msg = "Resource error"; break;
        case QMediaPlayer::FormatError: msg = "Format/codec error"; break;
        case QMediaPlayer::NetworkError: msg = "Network error"; break;
        case QMediaPlayer::AccessDeniedError: msg = "Access denied"; break;
        default: msg = "Media error"; break;
        }
    }
    emit errorOccurred(msg);
}
