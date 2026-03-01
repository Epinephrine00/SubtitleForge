#include "model/subtitle.h"

QJsonObject SubtitleEntry::toJson() const
{
    QJsonObject o;
    o["id"]         = id;
    o["keyFrame"]   = keyFrame;
    o["text"]       = text;
    o["fontFamily"] = font.family();
    o["fontBold"]   = font.bold();
    o["fontItalic"] = font.italic();
    o["textColor"]  = textColor.name(QColor::HexArgb);
    o["alignment"]  = static_cast<int>(alignment);
    return o;
}

SubtitleEntry SubtitleEntry::fromJson(const QJsonObject &o)
{
    SubtitleEntry e;
    e.id        = o["id"].toInt();
    e.keyFrame  = o["keyFrame"].toInteger();
    e.text      = o["text"].toString();
    e.font.setFamily(o["fontFamily"].toString("Arial"));
    e.font.setBold(o["fontBold"].toBool(false));
    e.font.setItalic(o["fontItalic"].toBool(false));
    e.textColor  = QColor(o["textColor"].toString("#ffffffff"));
    e.alignment  = static_cast<Qt::Alignment>(o["alignment"].toInt(Qt::AlignHCenter));
    return e;
}
