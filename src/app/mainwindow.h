#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QImage>
#include <QPair>
#include <QFutureWatcher>
#include <QMap>
#include <thread>
#include "model/project.h"
#include "audio/audiodecoder.h"
#include "audio/audioplayback.h"

class TimelineWidget;
class PreviewWidget;
class PreviewDecoder;

class QCheckBox;
class QComboBox;
class QFontComboBox;
class QLabel;
class QSpinBox;
class QTextEdit;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onSaveProjectAs();
    void onImportVideo();
    void onImportTxt();
    void onExportVideo();

    void onFrameChanged(qint64 frame);
    void onPlayPause();
    void onStepForward();
    void onStepBackward();
    void onStepForwardLarge();
    void onStepBackwardLarge();
    void onSubtitleSelected(int id);
    void onSubtitleDeselected();
    void onSubtitleMoved(int id, qint64 newKeyFrame);

    void onDeleteSubtitle(int id);

    void onGlobalSubtitleFontChanged();
    void onGlobalSubtitleColorChanged();
    void onVideoTitleChanged();
    void onPreviewFrameDecoded(qint64 frameIndex, QImage image);

    void onSetTrimStart();
    void onSetTrimEnd();

    void onAudioPositionChanged(qint64 frame);
    void onAudioDurationChanged(qint64 ms);
    void onRefreshTick();

private:
    void setupUi();
    void setupMenus();
    void connectSignals();
    void refreshTimeline();
    void seekToFrame(qint64 frame);
    void pauseIfPlaying();
    void updatePreviewVideoFrame();
    void ensurePreviewDecodeRequested();
    void updateTrimLabel();

    Project m_project;
    QString m_projectPath;

    AudioPlayback *m_audio = nullptr;
    AudioDecoder::WaveformData m_waveform;

    TimelineWidget *m_timeline = nullptr;
    PreviewWidget  *m_preview  = nullptr;

    QTextEdit       *m_titleEdit      = nullptr;
    QFontComboBox  *m_titleFontCombo = nullptr;
    QSpinBox       *m_titleFontSize  = nullptr;
    QCheckBox      *m_titleBoldCheck = nullptr;
    QCheckBox      *m_titleItalicCheck = nullptr;
    QSpinBox       *m_titlePosYSpin   = nullptr;
    class QPushButton *m_titleColorBtn = nullptr;

    QFontComboBox  *m_globalFontCombo = nullptr;
    QSpinBox       *m_globalFontSize  = nullptr;
    QCheckBox      *m_globalBoldCheck = nullptr;
    QCheckBox      *m_globalItalicCheck = nullptr;
    QSpinBox       *m_globalPosYSpin   = nullptr;
    class QPushButton *m_globalColorBtn = nullptr;

    class QPushButton *m_trimStartBtn = nullptr;
    class QPushButton *m_trimEndBtn   = nullptr;
    QLabel         *m_trimLabel      = nullptr;
    class QSlider *m_volumeSlider = nullptr;

    QTimer *m_refreshTimer = nullptr;
    PreviewDecoder *m_previewDecoder = nullptr;
    std::thread m_decoderThread;
    QMap<qint64, QImage> m_previewFrameCache;
    static const int kPreviewCacheMax = 8;
    qint64 m_currentFrame = 0;
    QColor m_globalSubtitleColor = Qt::white;
    QColor m_titleColor = Qt::white;
};
