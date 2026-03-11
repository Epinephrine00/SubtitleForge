#pragma once

#include <QString>
#include <QFont>
#include <QColor>
#include <QJsonObject>

struct SubtitleEntry {
    int id = 0;
    qint64 keyFrame = 0;  // frame in trimmed timeline where this subtitle is shown
    QString text;

    QJsonObject toJson() const;
    static SubtitleEntry fromJson(const QJsonObject &obj);
};
