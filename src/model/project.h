#pragma once

#include <QVector>
#include <QString>
#include <QJsonObject>
#include "model/subtitle.h"
#include "model/subtitleeffect.h"

class Project {
public:
    Project() : m_globalEffect(SubtitleEffect::createDefault()) {}

    // Audio
    const QString &audioFilePath() const { return m_audioPath; }
    void  setAudioFilePath(const QString &p) { m_audioPath = p; }

    // Global effect (applied to all subtitles)
    const SubtitleEffect &globalEffect() const { return m_globalEffect; }
    void setGlobalEffect(const SubtitleEffect &e) { m_globalEffect = e; }

    // Timing
    int   fps() const { return m_fps; }
    void  setFps(int v) { m_fps = v; }
    qint64 totalFrames() const { return m_totalFrames; }
    void  setTotalFrames(qint64 v) { m_totalFrames = v; }

    // Resolution
    int   videoWidth()  const { return m_w; }
    int   videoHeight() const { return m_h; }
    void  setResolution(int w, int h) { m_w = w; m_h = h; }

    // Subtitles (always sorted by keyFrame)
    const QVector<SubtitleEntry> &subtitles() const { return m_subs; }
    void  addSubtitle(const SubtitleEntry &e);
    void  removeSubtitle(int id);
    void  updateSubtitle(const SubtitleEntry &e);
    SubtitleEntry       *subtitleById(int id);
    const SubtitleEntry *subtitleById(int id) const;
    int   nextSubtitleId();
    void  sortSubtitles();
    void  convertSubtitleFrames(int oldFps, int newFps);
    void  clear();

    // Serialisation
    QJsonObject toJson() const;
    static Project fromJson(const QJsonObject &o);

private:
    QString m_audioPath;
    QVector<SubtitleEntry> m_subs;
    SubtitleEffect m_globalEffect;
    int    m_fps = 30;
    int    m_w   = 1920;
    int    m_h   = 1080;
    qint64 m_totalFrames = 0;
    int    m_nextId = 1;
};
