#include "model/animationengine.h"
#include "model/project.h"
#include <algorithm>

// Focus-only: subtitle is visible from its keyFrame until the next subtitle's keyFrame.
// Uses only globalEffect.focus for position, scale, fontSize, etc.
InterpolatedParams AnimationEngine::calculate(const SubtitleEntry &entry,
                                              const SubtitleEffect &eff,
                                              qint64 frame,
                                              qint64 prevKey,
                                              qint64 nextKey,
                                              qint64 /*nextNextKey*/)
{
    const qint64 Tn = entry.keyFrame;

    // Visible from Tn until next keyFrame (or end of timeline if no next)
    qint64 visEnd = (nextKey >= 0) ? nextKey : Tn + 60 * 4;  // fallback 4 sec
    if (frame < Tn || frame >= visEnd) {
        InterpolatedParams ip;
        ip.visible = false;
        return ip;
    }

    const StageParams &focus = eff.focus;
    InterpolatedParams ip;
    ip.posX     = focus.posX;
    ip.posY     = focus.posY;
    ip.scale    = focus.scale;
    ip.blur     = focus.blur;
    ip.opacity  = focus.opacity;
    ip.fontSize = focus.fontSize;
    ip.visible  = true;
    return ip;
}

QVector<std::pair<int, InterpolatedParams>>
AnimationEngine::calculateAll(const Project &project, qint64 frame)
{
    const auto &subs = project.subtitles();
    const auto &eff  = project.globalEffect();
    QVector<std::pair<int, InterpolatedParams>> out;

    for (int i = 0; i < subs.size(); ++i) {
        qint64 prev = (i > 0) ? subs[i - 1].keyFrame : -1;
        qint64 next = (i < subs.size() - 1) ? subs[i + 1].keyFrame : -1;
        qint64 nn   = (i < subs.size() - 2) ? subs[i + 2].keyFrame : -1;

        auto ip = calculate(subs[i], eff, frame, prev, next, nn);
        if (ip.visible)
            out.append({subs[i].id, ip});
    }
    return out;
}
