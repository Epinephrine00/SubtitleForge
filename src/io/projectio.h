#pragma once

#include <QString>
#include "model/project.h"

class ProjectIO {
public:
    static bool save(const Project &project, const QString &path);
    static bool load(const QString &path, Project &out);
};
