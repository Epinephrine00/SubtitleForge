#pragma once

#include <QWidget>
#include <QImage>
#include "model/project.h"

class PreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget *parent = nullptr);

    void setProject(const Project *project);
    void setCurrentFrame(qint64 frame);
    void setVideoFrame(const QImage &frame);

protected:
    void paintEvent(QPaintEvent *) override;
    QSize sizeHint() const override { return {360, 640}; }  // 9:16

private:
    const Project *m_project = nullptr;
    qint64 m_frame = 0;
    QImage m_videoFrame;
};
