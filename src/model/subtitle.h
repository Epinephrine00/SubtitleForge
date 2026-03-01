#pragma once

#include <QString>
#include <QFont>
#include <QColor>
#include <QJsonObject>

struct SubtitleEntry {
    int id = 0;
    qint64 keyFrame = 0;          // frame number where this subtitle is "focused"
    QString text;
    QFont font;
    QColor textColor = Qt::white;
    Qt::Alignment alignment = Qt::AlignHCenter;

    SubtitleEntry()
    {
        font.setFamily("Arial");
        font.setPointSize(24);
    }

    QJsonObject toJson() const;
    static SubtitleEntry fromJson(const QJsonObject &obj);
};
