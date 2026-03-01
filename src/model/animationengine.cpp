#include "model/animationengine.h"
#include "model/project.h"
#include <algorithm>

static InterpolatedParams interpFromStage(const StageParams &p)
{
    InterpolatedParams ip;
    ip.posX     = p.posX;
    ip.posY     = p.posY;
    ip.scale    = p.scale;
    ip.blur     = p.blur;
    ip.opacity  = p.opacity;
    ip.fontSize = p.fontSize;
    ip.visible  = true;
    return ip;
}

static InterpolatedParams lerpIP(const StageParams &a,
                                  const StageParams &b, float t)
{
    auto s = StageParams::lerp(a, b, t);
    return interpFromStage(s);
}

/*
 * Timeline for subtitle N with keyframe T_n, fade duration F:
 *
 *  T_prev     T_prev+F              T_n-F       T_n      T_next    T_next+F            T_nn-F      T_nn
 *     |          |                    |           |          |          |                  |          |
 *     |--Intro-->|-----Approach-------|--A>Focus->|--Focus---|--Recede->|------hold--------|--Outro-->|
 *
 * Disabled stages are skipped (neighbouring enabled values used instead).
 */
InterpolatedParams AnimationEngine::calculate(const SubtitleEntry &entry,
                                              const SubtitleEffect &eff,
                                              qint64 frame,
                                              qint64 prevKey,
                                              qint64 nextKey,
                                              qint64 nextNextKey)
{
    const qint64 F  = eff.fadeDurationFrames;
    const qint64 Tn = entry.keyFrame;

    const bool hasIntro    = eff.intro.enabled    && (prevKey >= 0);
    const bool hasApproach = eff.approach.enabled  && (prevKey >= 0);
    const bool hasRecede   = eff.recede.enabled    && (nextKey >= 0);
    const bool hasOutro    = eff.outro.enabled     && (nextKey >= 0);

    // --- Determine visibility window ---
    qint64 visStart = Tn;
    if (hasApproach)   visStart = std::max(prevKey + F, prevKey);
    if (hasIntro)      visStart = prevKey;

    qint64 visEnd = (nextKey >= 0) ? nextKey : Tn + F * 4;
    if (hasRecede) {
        qint64 recedeHoldEnd = (nextNextKey >= 0) ? nextNextKey - F
                                                   : visEnd + F * 2;
        visEnd = recedeHoldEnd;
    }
    if (hasOutro) {
        qint64 outroEnd = (nextNextKey >= 0) ? nextNextKey
                                              : visEnd + F;
        visEnd = outroEnd;
    }

    if (frame < visStart || frame >= visEnd) {
        InterpolatedParams ip;
        ip.visible = false;
        return ip;
    }

    // --- Pre-focus: stages Intro / Approach ---
    const StageParams &preTarget = hasApproach ? eff.approach : eff.focus;

    if (frame < Tn) {
        // Intro segment: prevKey .. prevKey+F
        if (hasIntro && frame < prevKey + F) {
            float t = (F > 0) ? static_cast<float>(frame - prevKey) / F : 1.0f;
            return lerpIP(eff.intro, preTarget, t);
        }

        // Approach hold: (after intro or prevKey) .. Tn-F
        if (frame < Tn - F) {
            return interpFromStage(hasApproach ? eff.approach : eff.focus);
        }

        // Approach -> Focus transition: Tn-F .. Tn
        {
            const StageParams &src = hasApproach ? eff.approach : eff.focus;
            float t = (F > 0) ? static_cast<float>(frame - (Tn - F)) / F : 1.0f;
            return lerpIP(src, eff.focus, t);
        }
    }

    // --- Focus segment: Tn .. nextKey-F ---
    // Focus ends F frames before the next subtitle's keyframe so that
    // the current subtitle starts leaving Focus at the same moment the
    // next subtitle begins its Approach→Focus transition.
    qint64 focusEnd = (nextKey >= 0) ? std::max(Tn, nextKey - F) : visEnd;
    if (frame < focusEnd) {
        return interpFromStage(eff.focus);
    }

    // --- Post-focus: stages Recede / Outro ---
    const StageParams &postTarget = hasRecede ? eff.recede : eff.focus;

    // Focus -> Recede transition: focusEnd .. nextKey
    if (nextKey >= 0 && frame < nextKey) {
        qint64 transLen = nextKey - focusEnd;
        float t = (transLen > 0) ? static_cast<float>(frame - focusEnd) / transLen : 1.0f;
        return lerpIP(eff.focus, postTarget, t);
    }

    // Recede hold: nextKey .. outroStart
    qint64 outroStart = (nextNextKey >= 0) ? nextNextKey - F : visEnd - F;
    if (frame < outroStart) {
        return interpFromStage(hasRecede ? eff.recede : eff.focus);
    }

    // Outro segment: outroStart .. visEnd
    if (hasOutro) {
        const StageParams &src = hasRecede ? eff.recede : eff.focus;
        float t = (F > 0) ? static_cast<float>(frame - outroStart) / F : 1.0f;
        return lerpIP(src, eff.outro, t);
    }

    return interpFromStage(hasRecede ? eff.recede : eff.focus);
}

QVector<std::pair<int, InterpolatedParams>>
AnimationEngine::calculateAll(const Project &project, qint64 frame)
{
    const auto &subs = project.subtitles();
    const auto &eff  = project.globalEffect();
    QVector<std::pair<int, InterpolatedParams>> out;

    for (int i = 0; i < subs.size(); ++i) {
        qint64 prev = (i > 0)               ? subs[i - 1].keyFrame : -1;
        qint64 next = (i < subs.size() - 1) ? subs[i + 1].keyFrame : -1;
        qint64 nn   = (i < subs.size() - 2) ? subs[i + 2].keyFrame : -1;

        auto ip = calculate(subs[i], eff, frame, prev, next, nn);
        if (ip.visible)
            out.append({subs[i].id, ip});
    }
    return out;
}
