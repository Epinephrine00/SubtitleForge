#pragma once

#include <QString>
#include "model/project.h"

class PremiereXmlExporter {
public:
    static bool exportXml(const Project &project,
                          const QString &outputPath,
                          QString *errorOut = nullptr);
};
