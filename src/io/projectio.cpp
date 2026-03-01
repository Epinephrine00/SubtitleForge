#include "io/projectio.h"
#include <QFile>
#include <QJsonDocument>

bool ProjectIO::save(const Project &project, const QString &path)
{
    QJsonDocument doc(project.toJson());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

bool ProjectIO::load(const QString &path, Project &out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    out = Project::fromJson(doc.object());
    return true;
}
