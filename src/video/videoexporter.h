#pragma once

#include <QObject>
#include <QString>
#include "model/project.h"

class VideoExporter : public QObject {
    Q_OBJECT
public:
    explicit VideoExporter(QObject *parent = nullptr);

    struct ExportSettings {
        QString outputPath;
        int fps    = 30;
        int width  = 1920;
        int height = 1080;
        bool alphaChannel = false;
    };

    bool exportVideo(const Project &project, const ExportSettings &settings);
    void requestCancel() { m_cancel = true; }
    bool wasCancelled() const { return m_cancel; }
    QString lastError() const { return m_lastError; }

signals:
    void progressChanged(int percent);
    void exportFinished(bool success, const QString &errorMsg);

private:
    bool m_cancel = false;
    QString m_lastError;
};
