#pragma once

#include <QObject>
#include <QString>
#include <QImage>
#include <QMutex>
#include <QWaitCondition>

// Runs in a dedicated thread; keeps the video file open and decodes requested
// frames without reopening the file (much faster than frameAt() per frame).
class PreviewDecoder : public QObject {
    Q_OBJECT
public:
    explicit PreviewDecoder(QObject *parent = nullptr);
    ~PreviewDecoder() override;

    // Call from any thread; decoder thread will process.
    void setFile(const QString &path);
    void requestFrame(qint64 frameIndex, double timeSec);
    void requestQuit();

signals:
    void frameDecoded(qint64 frameIndex, QImage image);

public slots:
    void runDecoder();

private:

    QString m_path;
    QMutex m_mutex;
    QWaitCondition m_cond;
    bool m_pathChanged = false;
    bool m_quit = false;
    qint64 m_requestedFrame = -1;
    double m_requestedTimeSec = -1.0;
    bool m_hasRequest = false;
};
