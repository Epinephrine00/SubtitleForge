#pragma once

#include <QMainWindow>
#include <QTimer>
#include "model/project.h"
#include "audio/audiodecoder.h"
#include "audio/audioplayback.h"

class TextInputPanel;
class EffectsPanel;
class TimelineWidget;
class PreviewWidget;

class QComboBox;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    // menu actions
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onSaveProjectAs();
    void onImportAudio();
    void onExportVideo();

    // timeline signals
    void onFrameChanged(qint64 frame);
    void onPlayPause();
    void onStepForward();
    void onStepBackward();
    void onStepForwardLarge();
    void onStepBackwardLarge();
    void onSubtitleSelected(int id);
    void onSubtitleDeselected();
    void onSubtitleMoved(int id, qint64 newKeyFrame);

    // text panel signals
    void onAddSubtitle(const QString &text, const QFont &font,
                       const QColor &color, Qt::Alignment align);
    void onSubtitleTextChanged(int id, const QString &text);
    void onSubtitleFontChanged(int id, const QFont &font);
    void onSubtitleColorChanged(int id, const QColor &color);
    void onSubtitleAlignmentChanged(int id, Qt::Alignment align);
    void onDeleteSubtitle(int id);

    // effects panel
    void onEffectChanged(const SubtitleEffect &eff);

    // output settings
    void onProjectFpsChanged(int index);
    void onProjectResolutionChanged(int index);

    // playback
    void onAudioPositionChanged(qint64 frame);
    void onAudioDurationChanged(qint64 ms);

    // refresh timer during playback
    void onRefreshTick();

private:
    void setupUi();
    void setupMenus();
    void connectSignals();
    void refreshTimeline();
    void seekToFrame(qint64 frame);
    void pauseIfPlaying();

    Project        m_project;
    QString        m_projectPath;

    AudioPlayback *m_audio    = nullptr;
    AudioDecoder::WaveformData m_waveform;

    TextInputPanel *m_textPanel   = nullptr;
    EffectsPanel   *m_effectPanel = nullptr;
    TimelineWidget *m_timeline    = nullptr;
    PreviewWidget  *m_preview     = nullptr;

    QComboBox *m_fpsCombo   = nullptr;
    QComboBox *m_resCombo  = nullptr;
    class QSlider *m_volumeSlider = nullptr;

    QTimer *m_refreshTimer = nullptr;
    qint64  m_currentFrame = 0;

    void syncSettingsCombos();
};
