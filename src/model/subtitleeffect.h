#pragma once

#include <QJsonObject>

struct StageParams {
    bool enabled = true;
    float posX = 0.0f;       // relative offset from center (pixels)
    float posY = 0.0f;
    float scale = 100.0f;    // percent
    float blur = 0.0f;       // gaussian blur radius
    float opacity = 100.0f;  // percent
    float fontSize = 48.0f;  // points

    static StageParams lerp(const StageParams &a, const StageParams &b, float t);

    QJsonObject toJson() const;
    static StageParams fromJson(const QJsonObject &obj);
};

struct SubtitleEffect {
    int fadeDurationFrames = 12;
    StageParams intro;
    StageParams approach;
    StageParams focus;
    StageParams recede;
    StageParams outro;

    static SubtitleEffect createDefault();

    QJsonObject toJson() const;
    static SubtitleEffect fromJson(const QJsonObject &obj);
};
