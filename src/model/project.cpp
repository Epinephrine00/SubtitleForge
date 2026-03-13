#include "model/project.h"
#include <QJsonArray>
#include <algorithm>

QJsonObject VideoTitle::toJson() const
{
    QJsonObject o;
    o["text"]       = text;
    o["fontFamily"] = font.family();
    o["fontBold"]   = font.bold();
    o["fontItalic"] = font.italic();
    o["fontSize"]   = fontSize;
    o["color"]      = color.name(QColor::HexArgb);
    o["posY"]       = posY;
    o["outlineEnabled"]   = outlineEnabled;
    o["outlineColor"]     = outlineColor.name(QColor::HexArgb);
    o["outlineWidthPx"]   = outlineWidthPx;
    return o;
}

VideoTitle VideoTitle::fromJson(const QJsonObject &o)
{
    VideoTitle t;
    t.text     = o["text"].toString();
    t.font.setFamily(o["fontFamily"].toString("Arial"));
    t.font.setBold(o["fontBold"].toBool(false));
    t.font.setItalic(o["fontItalic"].toBool(false));
    t.fontSize = static_cast<float>(o["fontSize"].toDouble(36));
    t.color    = QColor(o["color"].toString("#ffffffff"));
    t.posY     = static_cast<float>(o["posY"].toDouble(0));
    t.outlineEnabled = o["outlineEnabled"].toBool(false);
    t.outlineColor   = QColor(o["outlineColor"].toString("#ff000000"));
    t.outlineWidthPx = static_cast<float>(o["outlineWidthPx"].toDouble(2.0));
    return t;
}

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

void Project::setTotalFrames(qint64 v)
{
    m_trimEnd = m_trimStart + std::max<qint64>(0, v);
}

qint64 Project::trimmedToSourceFrame(qint64 trimmedFrame) const
{
    return m_trimStart + std::max<qint64>(0, trimmedFrame);
}

void Project::clear()
{
    m_subs.clear();
    m_mediaPath.clear();
    m_sourceFrameRate = 60.0;
    m_trimStart = 0;
    m_trimEnd   = 0;
    m_totalSourceFrames = 0;
    m_nextId    = 1;
    m_globalSubtitleFont = QFont("Arial");
    m_globalSubtitleFontSize = 48.0f;
    m_globalSubtitleColor = Qt::white;
    m_globalSubtitleOutlineEnabled = false;
    m_globalSubtitleOutlineColor = Qt::black;
    m_globalSubtitleOutlineWidthPx = 2.0f;
    m_videoTitle = VideoTitle();
}

void Project::clearSubtitles()
{
    m_subs.clear();
    m_nextId = 1;
}

QJsonObject Project::toJson() const
{
    QJsonArray arr;
    for (const auto &s : m_subs) arr.append(s.toJson());

    QJsonObject o;
    o["mediaFilePath"]         = m_mediaPath;
    o["sourceFrameRate"]       = m_sourceFrameRate;
    o["trimStartFrame"]        = m_trimStart;
    o["trimEndFrame"]          = m_trimEnd;
    o["totalSourceFrames"]     = m_totalSourceFrames;
    o["nextId"]                = m_nextId;
    o["subtitles"]             = arr;
    o["globalEffect"]           = m_globalEffect.toJson();
    o["globalSubtitleFont"]    = m_globalSubtitleFont.family();
    o["globalSubtitleBold"]    = m_globalSubtitleFont.bold();
    o["globalSubtitleItalic"]  = m_globalSubtitleFont.italic();
    o["globalSubtitleFontSize"] = m_globalSubtitleFontSize;
    o["globalSubtitleColor"]   = m_globalSubtitleColor.name(QColor::HexArgb);
    o["globalSubtitleOutlineEnabled"] = m_globalSubtitleOutlineEnabled;
    o["globalSubtitleOutlineColor"]   = m_globalSubtitleOutlineColor.name(QColor::HexArgb);
    o["globalSubtitleOutlineWidthPx"] = m_globalSubtitleOutlineWidthPx;
    o["videoTitle"]            = m_videoTitle.toJson();
    return o;
}

Project Project::fromJson(const QJsonObject &o)
{
    Project p;
    p.m_mediaPath   = o["mediaFilePath"].toString();
    if (p.m_mediaPath.isEmpty())
        p.m_mediaPath = o["audioFilePath"].toString();  // legacy
    p.m_sourceFrameRate = o["sourceFrameRate"].toDouble(60.0);
    if (p.m_sourceFrameRate <= 0) p.m_sourceFrameRate = 60.0;
    p.m_trimStart   = o["trimStartFrame"].toInteger(0);
    p.m_trimEnd     = o["trimEndFrame"].toInteger(0);
    p.m_totalSourceFrames = o["totalSourceFrames"].toInteger(0);
    p.m_nextId      = o["nextId"].toInt(1);

    p.m_globalSubtitleFont.setFamily(o["globalSubtitleFont"].toString("Arial"));
    p.m_globalSubtitleFont.setBold(o["globalSubtitleBold"].toBool(false));
    p.m_globalSubtitleFont.setItalic(o["globalSubtitleItalic"].toBool(false));
    p.m_globalSubtitleFontSize = static_cast<float>(o["globalSubtitleFontSize"].toDouble(48));
    p.m_globalSubtitleColor = QColor(o["globalSubtitleColor"].toString("#ffffffff"));
    p.m_globalSubtitleOutlineEnabled = o["globalSubtitleOutlineEnabled"].toBool(false);
    p.m_globalSubtitleOutlineColor  = QColor(o["globalSubtitleOutlineColor"].toString("#ff000000"));
    p.m_globalSubtitleOutlineWidthPx = static_cast<float>(o["globalSubtitleOutlineWidthPx"].toDouble(2.0));

    if (o.contains("videoTitle"))
        p.m_videoTitle = VideoTitle::fromJson(o["videoTitle"].toObject());

    if (o.contains("globalEffect"))
        p.m_globalEffect = SubtitleEffect::fromJson(o["globalEffect"].toObject());

    for (const auto &v : o["subtitles"].toArray())
        p.m_subs.append(SubtitleEntry::fromJson(v.toObject()));
    return p;
}
