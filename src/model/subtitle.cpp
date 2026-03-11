#include "model/subtitle.h"

QJsonObject SubtitleEntry::toJson() const
{
    QJsonObject o;
    o["id"]       = id;
    o["keyFrame"] = keyFrame;
    o["text"]     = text;
    return o;
}

SubtitleEntry SubtitleEntry::fromJson(const QJsonObject &o)
{
    SubtitleEntry e;
    e.id       = o["id"].toInt();
    e.keyFrame = o["keyFrame"].toInteger();
    e.text     = o["text"].toString();
    return e;
}
