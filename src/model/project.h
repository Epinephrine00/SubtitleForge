#pragma once

#include <QVector>
#include <QString>
#include <QFont>
#include <QColor>
#include <QJsonObject>
#include "model/subtitle.h"
#include "model/subtitleeffect.h"

struct VideoTitle {
    QString text;
    QFont font;
    float fontSize = 36.0f;
    QColor color = Qt::white;
    float posY = 0.0f;  // vertical offset from center (pixels)
    bool outlineEnabled = false;
    QColor outlineColor = Qt::black;
    float outlineWidthPx = 2.0f;

    QJsonObject toJson() const;
    static VideoTitle fromJson(const QJsonObject &o);
};

class Project {
public:
    Project() : m_globalEffect(SubtitleEffect::createDefault()) {}

    // Media (video or audio file path)
    const QString &mediaFilePath() const { return m_mediaPath; }
    void setMediaFilePath(const QString &p) { m_mediaPath = p; }

    // Trim: frames in source timeline; effective duration = trimEnd - trimStart
    qint64 trimStartFrame() const { return m_trimStart; }
    qint64 trimEndFrame() const { return m_trimEnd; }
    void setTrimStartFrame(qint64 v) { m_trimStart = std::max<qint64>(0, v); }
    void setTrimEndFrame(qint64 v) { m_trimEnd = v; }
    // Total source length (frames) for timeline/waveform; 0 = use trimEnd (legacy)
    qint64 totalSourceFrames() const { return m_totalSourceFrames > 0 ? m_totalSourceFrames : m_trimEnd; }
    void setTotalSourceFrames(qint64 v) { m_totalSourceFrames = v; }

    // Global subtitle style (all subtitles use this)
    QFont globalSubtitleFont() const { return m_globalSubtitleFont; }
    void setGlobalSubtitleFont(const QFont &f) { m_globalSubtitleFont = f; }
    float globalSubtitleFontSize() const { return m_globalSubtitleFontSize; }
    void setGlobalSubtitleFontSize(float v) { m_globalSubtitleFontSize = v; }
    QColor globalSubtitleColor() const { return m_globalSubtitleColor; }
    void setGlobalSubtitleColor(const QColor &c) { m_globalSubtitleColor = c; }
    bool globalSubtitleOutlineEnabled() const { return m_globalSubtitleOutlineEnabled; }
    void setGlobalSubtitleOutlineEnabled(bool v) { m_globalSubtitleOutlineEnabled = v; }
    QColor globalSubtitleOutlineColor() const { return m_globalSubtitleOutlineColor; }
    void setGlobalSubtitleOutlineColor(const QColor &c) { m_globalSubtitleOutlineColor = c; }
    float globalSubtitleOutlineWidthPx() const { return m_globalSubtitleOutlineWidthPx; }
    void setGlobalSubtitleOutlineWidthPx(float v) { m_globalSubtitleOutlineWidthPx = v; }

    // Video title (shown from start to end)
    const VideoTitle &videoTitle() const { return m_videoTitle; }
    void setVideoTitle(const VideoTitle &t) { m_videoTitle = t; }

    // Focus-only effect (position, scale for subtitles)
    const SubtitleEffect &globalEffect() const { return m_globalEffect; }
    void setGlobalEffect(const SubtitleEffect &e) { m_globalEffect = e; }

    // Fixed output: 60 fps, 1080x1920
    int fps() const { return 60; }
    int videoWidth() const { return 1080; }
    int videoHeight() const { return 1920; }

    // Source video frame rate (e.g. 30, 60). Used for timeline and decode; default 60.
    double sourceFrameRate() const { return m_sourceFrameRate; }
    void setSourceFrameRate(double r) { m_sourceFrameRate = r > 0 ? r : 60.0; }

    // Total frames = trimmed duration
    qint64 totalFrames() const { return std::max<qint64>(0, m_trimEnd - m_trimStart); }
    void setTotalFrames(qint64 v);  // sets trim end from trim start

    // Subtitles (sorted by keyFrame); keyFrame is in trimmed timeline (0 .. totalFrames())
    const QVector<SubtitleEntry> &subtitles() const { return m_subs; }
    void addSubtitle(const SubtitleEntry &e);
    void removeSubtitle(int id);
    void updateSubtitle(const SubtitleEntry &e);
    SubtitleEntry *subtitleById(int id);
    const SubtitleEntry *subtitleById(int id) const;
    int nextSubtitleId();
    void sortSubtitles();
    void clear();
    void clearSubtitles();  // remove all subtitles, keep media/trim/title/style

    // Convert frame from trimmed to source (for seeking)
    qint64 trimmedToSourceFrame(qint64 trimmedFrame) const;

    QJsonObject toJson() const;
    static Project fromJson(const QJsonObject &o);

private:
    QString m_mediaPath;
    double m_sourceFrameRate = 60.0;
    qint64 m_trimStart = 0;
    qint64 m_trimEnd   = 0;
    qint64 m_totalSourceFrames = 0;
    QVector<SubtitleEntry> m_subs;
    QFont m_globalSubtitleFont;
    float m_globalSubtitleFontSize = 48.0f;
    QColor m_globalSubtitleColor = Qt::white;
    bool m_globalSubtitleOutlineEnabled = false;
    QColor m_globalSubtitleOutlineColor = Qt::black;
    float m_globalSubtitleOutlineWidthPx = 2.0f;
    VideoTitle m_videoTitle;
    SubtitleEffect m_globalEffect;
    int m_nextId = 1;
};
