#pragma once

#include <QVector>
#include "model/subtitle.h"
#include "model/subtitleeffect.h"

class Project;

struct InterpolatedParams {
    float posX     = 0.0f;
    float posY     = 0.0f;
    float scale    = 100.0f;
    float blur     = 0.0f;
    float opacity  = 0.0f;
    float fontSize = 48.0f;
    bool  visible  = false;
};

class AnimationEngine {
public:
    // Calculate interpolated parameters for a single subtitle at `frame`.
    // Uses the global effect for animation stages.
    static InterpolatedParams calculate(const SubtitleEntry &entry,
                                        const SubtitleEffect &effect,
                                        qint64 frame,
                                        qint64 prevKey,
                                        qint64 nextKey,
                                        qint64 nextNextKey);

    // Convenience: evaluate every subtitle in the project and return only
    // those visible at `frame` with their interpolated params.
    static QVector<std::pair<int /*id*/, InterpolatedParams>>
    calculateAll(const Project &project, qint64 frame);
};
