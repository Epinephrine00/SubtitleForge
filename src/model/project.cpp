#include "model/project.h"
#include <QJsonArray>
#include <algorithm>

void Project::addSubtitle(const SubtitleEntry &e)
{
    m_subs.append(e);
    sortSubtitles();
}

void Project::removeSubtitle(int id)
{
    m_subs.erase(
        std::remove_if(m_subs.begin(), m_subs.end(),
                        [id](const SubtitleEntry &s) { return s.id == id; }),
        m_subs.end());
}

void Project::updateSubtitle(const SubtitleEntry &e)
{
    for (auto &s : m_subs) {
        if (s.id == e.id) { s = e; break; }
    }
    sortSubtitles();
}

SubtitleEntry *Project::subtitleById(int id)
{
    for (auto &s : m_subs)
        if (s.id == id) return &s;
    return nullptr;
}

const SubtitleEntry *Project::subtitleById(int id) const
{
    for (const auto &s : m_subs)
        if (s.id == id) return &s;
    return nullptr;
}

int Project::nextSubtitleId() { return m_nextId++; }

void Project::sortSubtitles()
{
    std::sort(m_subs.begin(), m_subs.end(),
              [](const SubtitleEntry &a, const SubtitleEntry &b) {
                  return a.keyFrame < b.keyFrame;
              });
}

void Project::convertSubtitleFrames(int oldFps, int newFps)
{
    if (oldFps == newFps || oldFps <= 0 || newFps <= 0) return;
    for (auto &sub : m_subs)
        sub.keyFrame = sub.keyFrame * newFps / oldFps;
}

void Project::clear()
{
    m_subs.clear();
    m_audioPath.clear();
    m_totalFrames = 0;
    m_nextId = 1;
}

QJsonObject Project::toJson() const
{
    QJsonArray arr;
    for (const auto &s : m_subs) arr.append(s.toJson());

    QJsonObject o;
    o["audioFilePath"] = m_audioPath;
    o["fps"]           = m_fps;
    o["videoWidth"]    = m_w;
    o["videoHeight"]   = m_h;
    o["totalFrames"]   = m_totalFrames;
    o["nextId"]        = m_nextId;
    o["subtitles"]     = arr;
    o["globalEffect"]  = m_globalEffect.toJson();
    return o;
}

Project Project::fromJson(const QJsonObject &o)
{
    Project p;
    p.m_audioPath   = o["audioFilePath"].toString();
    p.m_fps         = o["fps"].toInt(30);
    p.m_w           = o["videoWidth"].toInt(1920);
    p.m_h           = o["videoHeight"].toInt(1080);
    p.m_totalFrames = o["totalFrames"].toInteger();
    p.m_nextId      = o["nextId"].toInt(1);

    if (o.contains("globalEffect"))
        p.m_globalEffect = SubtitleEffect::fromJson(o["globalEffect"].toObject());

    for (const auto &v : o["subtitles"].toArray())
        p.m_subs.append(SubtitleEntry::fromJson(v.toObject()));
    return p;
}
