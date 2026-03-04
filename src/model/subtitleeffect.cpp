#include "model/subtitleeffect.h"
#include <algorithm>

StageParams StageParams::lerp(const StageParams &a, const StageParams &b, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    StageParams r;
    r.enabled   = b.enabled;
    r.posX      = a.posX     + (b.posX     - a.posX)     * t;
    r.posY      = a.posY     + (b.posY     - a.posY)     * t;
    r.scale     = a.scale    + (b.scale    - a.scale)    * t;
    r.blur      = a.blur     + (b.blur     - a.blur)     * t;
    r.opacity   = a.opacity  + (b.opacity  - a.opacity)  * t;
    r.fontSize  = a.fontSize + (b.fontSize - a.fontSize) * t;
    return r;
}

QJsonObject StageParams::toJson() const
{
    return {
        {"enabled",  enabled},
        {"posX",     static_cast<double>(posX)},
        {"posY",     static_cast<double>(posY)},
        {"scale",    static_cast<double>(scale)},
        {"blur",     static_cast<double>(blur)},
        {"opacity",  static_cast<double>(opacity)},
        {"fontSize", static_cast<double>(fontSize)}
    };
}

StageParams StageParams::fromJson(const QJsonObject &o)
{
    StageParams p;
    p.enabled  = o["enabled"].toBool(true);
    p.posX     = static_cast<float>(o["posX"].toDouble(0));
    p.posY     = static_cast<float>(o["posY"].toDouble(0));
    p.scale    = static_cast<float>(o["scale"].toDouble(100));
    p.blur     = static_cast<float>(o["blur"].toDouble(0));
    p.opacity  = static_cast<float>(o["opacity"].toDouble(100));
    p.fontSize = static_cast<float>(o["fontSize"].toDouble(48));
    return p;
}

// -----------------------------------------------------------------------

SubtitleEffect SubtitleEffect::createDefault()
{
    SubtitleEffect e;
    e.fadeDurationFrames = 12;

    // Intro/Approach/Recede/Outro enabled by default so full animation is visible
    // (including in MOV alpha export); user can disable per-stage in Effects panel.
    e.intro.enabled  = true;
    e.intro.opacity  = 100.0f;
    e.intro.blur     = 10.0f;

    e.approach.enabled = true;
    e.approach.opacity = 100.0f;

    e.focus.enabled  = true;
    e.focus.posX     = 0.0f;
    e.focus.posY     = 0.0f;
    e.focus.scale    = 100.0f;
    e.focus.blur     = 0.0f;
    e.focus.opacity  = 100.0f;
    e.focus.fontSize = 48.0f;

    e.recede.enabled = true;
    e.recede.opacity = 100.0f;

    e.outro.enabled  = true;
    e.outro.opacity  = 100.0f;
    e.outro.blur     = 10.0f;

    return e;
}

QJsonObject SubtitleEffect::toJson() const
{
    return {
        {"fadeDurationFrames", fadeDurationFrames},
        {"intro",    intro.toJson()},
        {"approach", approach.toJson()},
        {"focus",    focus.toJson()},
        {"recede",   recede.toJson()},
        {"outro",    outro.toJson()}
    };
}

SubtitleEffect SubtitleEffect::fromJson(const QJsonObject &o)
{
    SubtitleEffect e;
    e.fadeDurationFrames = o["fadeDurationFrames"].toInt(12);
    e.intro    = StageParams::fromJson(o["intro"].toObject());
    e.approach = StageParams::fromJson(o["approach"].toObject());
    e.focus    = StageParams::fromJson(o["focus"].toObject());
    e.recede   = StageParams::fromJson(o["recede"].toObject());
    e.outro    = StageParams::fromJson(o["outro"].toObject());

    // Migration: enabled non-focus stages with opacity 0 were invisible in alpha export
    if (e.intro.enabled    && e.intro.opacity <= 0.0f)    e.intro.opacity = 100.0f;
    if (e.approach.enabled && e.approach.opacity <= 0.0f) e.approach.opacity = 100.0f;
    if (e.recede.enabled   && e.recede.opacity <= 0.0f)    e.recede.opacity = 100.0f;
    if (e.outro.enabled    && e.outro.opacity <= 0.0f)    e.outro.opacity = 100.0f;

    return e;
}
