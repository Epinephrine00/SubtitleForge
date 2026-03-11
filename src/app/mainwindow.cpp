#include "app/mainwindow.h"
#include "widgets/timelinewidget.h"
#include "widgets/previewwidget.h"
#include "io/projectio.h"
#include "video/videoexporter.h"
#include "video/videodecoder.h"
#include "video/previewdecoder.h"

#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QProgressDialog>
#include <QComboBox>
#include <QLabel>
#include <QDialog>
#include <QDialogButtonBox>
#include <QThread>
#include <QApplication>
#include <QCoreApplication>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QSlider>
#include <QSettings>
#include <QTextEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QColorDialog>
#include <QScrollArea>
#include <QFontComboBox>
#include <QFile>
#include <QLineEdit>
#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QStatusBar>
#include <cmath>

static const int kFps = 60;  // output fps only; timeline uses project.sourceFrameRate()

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Reels Forge");
    setupUi();
    setupMenus();
    connectSignals();

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(16);
    connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::onRefreshTick);

    m_previewDecoder = new PreviewDecoder(this);
    connect(m_previewDecoder, &PreviewDecoder::frameDecoded, this, &MainWindow::onPreviewFrameDecoded, Qt::QueuedConnection);
    m_decoderThread = std::thread([this]() { m_previewDecoder->runDecoder(); });

    qApp->installEventFilter(this);
}

void MainWindow::setupUi()
{
    m_timeline = new TimelineWidget;
    m_preview  = new PreviewWidget;
    m_audio    = new AudioPlayback(this);

    m_preview->setProject(&m_project);

    // ---- Left panel ----
    auto *leftScroll = new QScrollArea;
    leftScroll->setWidgetResizable(true);
    leftScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *leftPanel = new QWidget;
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(6, 6, 6, 6);

    // Media & Export
    auto *importGroup = new QGroupBox("Media");
    auto *importLayout = new QVBoxLayout(importGroup);
    auto *btnImportVideo = new QPushButton("Import Video…");
    connect(btnImportVideo, &QPushButton::clicked, this, &MainWindow::onImportVideo);
    importLayout->addWidget(btnImportVideo);
    auto *btnImportTxt = new QPushButton("Import Subtitles (TXT)…");
    connect(btnImportTxt, &QPushButton::clicked, this, &MainWindow::onImportTxt);
    importLayout->addWidget(btnImportTxt);
    auto *btnExportVideo = new QPushButton("Export Video…");
    btnExportVideo->setStyleSheet("font-weight: bold;");
    connect(btnExportVideo, &QPushButton::clicked, this, &MainWindow::onExportVideo);
    importLayout->addWidget(btnExportVideo);
    leftLayout->addWidget(importGroup);

    // Trim: set start/end from playhead
    auto *trimGroup = new QGroupBox("Trim");
    auto *trimLayout = new QVBoxLayout(trimGroup);
    auto *trimBtnRow = new QHBoxLayout;
    m_trimStartBtn = new QPushButton("Set start");
    m_trimStartBtn->setToolTip("Set trim start to current playhead position");
    connect(m_trimStartBtn, &QPushButton::clicked, this, &MainWindow::onSetTrimStart);
    m_trimEndBtn = new QPushButton("Set end");
    m_trimEndBtn->setToolTip("Set trim end to current playhead position");
    connect(m_trimEndBtn, &QPushButton::clicked, this, &MainWindow::onSetTrimEnd);
    trimBtnRow->addWidget(m_trimStartBtn);
    trimBtnRow->addWidget(m_trimEndBtn);
    trimLayout->addLayout(trimBtnRow);
    m_trimLabel = new QLabel("0:00 – 0:00");
    trimLayout->addWidget(m_trimLabel);
    leftLayout->addWidget(trimGroup);

    // Global subtitle style
    auto *subStyleGroup = new QGroupBox("Subtitle Style");
    auto *subForm = new QFormLayout(subStyleGroup);
    m_globalFontCombo = new QFontComboBox;
    m_globalFontCombo->setCurrentFont(QFont("Arial"));
    connect(m_globalFontCombo, &QFontComboBox::currentFontChanged, this, &MainWindow::onGlobalSubtitleFontChanged);
    m_globalFontSize = new QSpinBox;
    m_globalFontSize->setRange(8, 200);
    m_globalFontSize->setValue(48);
    connect(m_globalFontSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onGlobalSubtitleFontChanged);
    m_globalBoldCheck = new QCheckBox("B");
    m_globalBoldCheck->setStyleSheet("font-weight:bold;");
    connect(m_globalBoldCheck, &QCheckBox::toggled, this, &MainWindow::onGlobalSubtitleFontChanged);
    m_globalItalicCheck = new QCheckBox("I");
    m_globalItalicCheck->setStyleSheet("font-style:italic;");
    connect(m_globalItalicCheck, &QCheckBox::toggled, this, &MainWindow::onGlobalSubtitleFontChanged);
    m_globalPosYSpin = new QSpinBox;
    m_globalPosYSpin->setRange(-2000, 2000);
    m_globalPosYSpin->setSuffix(" px");
    m_globalPosYSpin->setValue(0);
    connect(m_globalPosYSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onGlobalSubtitleFontChanged);
    m_globalColorBtn = new QPushButton;
    m_globalColorBtn->setFixedSize(28, 28);
    m_globalColorBtn->setStyleSheet("background-color: white; border:1px solid #555;");
    connect(m_globalColorBtn, &QPushButton::clicked, this, [this]() {
        QColor c = QColorDialog::getColor(m_globalSubtitleColor, this, "Subtitle Color");
        if (c.isValid()) { m_globalSubtitleColor = c; m_globalColorBtn->setStyleSheet(QString("background-color: %1; border:1px solid #555;").arg(c.name())); onGlobalSubtitleColorChanged(); }
    });
    subForm->addRow("Font:", m_globalFontCombo);
    subForm->addRow("Size:", m_globalFontSize);
    auto *subBoldItalic = new QHBoxLayout;
    subBoldItalic->addWidget(m_globalBoldCheck);
    subBoldItalic->addWidget(m_globalItalicCheck);
    subForm->addRow(subBoldItalic);
    subForm->addRow("Vertical pos:", m_globalPosYSpin);
    subForm->addRow("Color:", m_globalColorBtn);
    leftLayout->addWidget(subStyleGroup);

    // Video title
    auto *titleGroup = new QGroupBox("Video Title");
    auto *titleLayout = new QVBoxLayout(titleGroup);
    m_titleEdit = new QTextEdit;
    m_titleEdit->setPlaceholderText("Title text (multiple lines)…");
    m_titleEdit->setMaximumHeight(80);
    connect(m_titleEdit, &QTextEdit::textChanged, this, &MainWindow::onVideoTitleChanged);
    titleLayout->addWidget(m_titleEdit);
    auto *titleForm = new QFormLayout;
    m_titleFontCombo = new QFontComboBox;
    m_titleFontCombo->setCurrentFont(QFont("Arial"));
    connect(m_titleFontCombo, &QFontComboBox::currentFontChanged, this, &MainWindow::onVideoTitleChanged);
    m_titleFontSize = new QSpinBox;
    m_titleFontSize->setRange(8, 200);
    m_titleFontSize->setValue(36);
    connect(m_titleFontSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onVideoTitleChanged);
    m_titleBoldCheck = new QCheckBox("B");
    m_titleBoldCheck->setStyleSheet("font-weight:bold;");
    connect(m_titleBoldCheck, &QCheckBox::toggled, this, &MainWindow::onVideoTitleChanged);
    m_titleItalicCheck = new QCheckBox("I");
    m_titleItalicCheck->setStyleSheet("font-style:italic;");
    connect(m_titleItalicCheck, &QCheckBox::toggled, this, &MainWindow::onVideoTitleChanged);
    m_titlePosYSpin = new QSpinBox;
    m_titlePosYSpin->setRange(-2000, 2000);
    m_titlePosYSpin->setSuffix(" px");
    m_titlePosYSpin->setValue(0);
    connect(m_titlePosYSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onVideoTitleChanged);
    m_titleColorBtn = new QPushButton;
    m_titleColorBtn->setFixedSize(28, 28);
    m_titleColorBtn->setStyleSheet("background-color: white; border:1px solid #555;");
    connect(m_titleColorBtn, &QPushButton::clicked, this, [this]() {
        QColor c = QColorDialog::getColor(m_titleColor, this, "Title Color");
        if (c.isValid()) { m_titleColor = c; m_titleColorBtn->setStyleSheet(QString("background-color: %1; border:1px solid #555;").arg(c.name())); onVideoTitleChanged(); }
    });
    titleForm->addRow("Font:", m_titleFontCombo);
    titleForm->addRow("Size:", m_titleFontSize);
    auto *titleBoldItalic = new QHBoxLayout;
    titleBoldItalic->addWidget(m_titleBoldCheck);
    titleBoldItalic->addWidget(m_titleItalicCheck);
    titleForm->addRow(titleBoldItalic);
    titleForm->addRow("Vertical pos:", m_titlePosYSpin);
    titleForm->addRow("Color:", m_titleColorBtn);
    titleLayout->addLayout(titleForm);
    leftLayout->addWidget(titleGroup);

    // Volume
    auto *volGroup = new QGroupBox("Volume");
    auto *volLayout = new QVBoxLayout(volGroup);
    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(static_cast<int>(m_audio->volume() * 100));
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int v) { m_audio->setVolume(v / 100.0f); });
    volLayout->addWidget(m_volumeSlider);
    leftLayout->addWidget(volGroup);

    leftLayout->addStretch();
    leftScroll->setWidget(leftPanel);

    // ---- Top: left | preview ----
    auto *topSplitter = new QSplitter(Qt::Horizontal);
    topSplitter->addWidget(leftScroll);
    topSplitter->addWidget(m_preview);
    topSplitter->setStretchFactor(0, 0);
    topSplitter->setStretchFactor(1, 1);

    // ---- Main: top | timeline ----
    auto *mainSplitter = new QSplitter(Qt::Vertical);
    mainSplitter->addWidget(topSplitter);
    mainSplitter->addWidget(m_timeline);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 0);

    setCentralWidget(mainSplitter);
}

void MainWindow::setupMenus()
{
    auto *file = menuBar()->addMenu("&File");
    file->addAction("&New Project", this, &MainWindow::onNewProject, QKeySequence::New);
    file->addAction("&Open Project…", this, &MainWindow::onOpenProject, QKeySequence::Open);
    file->addAction("&Save Project", this, &MainWindow::onSaveProject, QKeySequence::Save);
    file->addAction("Save Project &As…", this, &MainWindow::onSaveProjectAs, QKeySequence("Ctrl+Shift+S"));
    file->addSeparator();
    file->addAction("&Import Video…", this, &MainWindow::onImportVideo, QKeySequence("Ctrl+I"));
    file->addAction("Import &TXT…", this, &MainWindow::onImportTxt);
    file->addSeparator();
    file->addAction("&Export Video…", this, &MainWindow::onExportVideo, QKeySequence("Ctrl+E"));
    file->addSeparator();
    file->addAction("E&xit", this, &QWidget::close, QKeySequence::Quit);
}

void MainWindow::connectSignals()
{
    connect(m_timeline, &TimelineWidget::frameChanged, this, &MainWindow::onFrameChanged);
    connect(m_timeline, &TimelineWidget::playPauseRequested, this, &MainWindow::onPlayPause);
    connect(m_timeline, &TimelineWidget::stepForward, this, &MainWindow::onStepForward);
    connect(m_timeline, &TimelineWidget::stepBackward, this, &MainWindow::onStepBackward);
    connect(m_timeline, &TimelineWidget::stepForwardLarge, this, &MainWindow::onStepForwardLarge);
    connect(m_timeline, &TimelineWidget::stepBackwardLarge, this, &MainWindow::onStepBackwardLarge);
    connect(m_timeline, &TimelineWidget::subtitleSelected, this, &MainWindow::onSubtitleSelected);
    connect(m_timeline, &TimelineWidget::subtitleDeselected, this, &MainWindow::onSubtitleDeselected);
    connect(m_timeline, &TimelineWidget::subtitleMoved, this, &MainWindow::onSubtitleMoved);
    connect(m_timeline, &TimelineWidget::deleteSubtitleRequested, this, &MainWindow::onDeleteSubtitle);

    connect(m_audio, &AudioPlayback::positionChanged, this, &MainWindow::onAudioPositionChanged);
    connect(m_audio, &AudioPlayback::durationChanged, this, &MainWindow::onAudioDurationChanged);
    connect(m_audio, &AudioPlayback::errorOccurred, this, [this](const QString &msg) {
        statusBar()->showMessage(QString("Audio: %1").arg(msg), 8000);
    });
}

void MainWindow::onNewProject()
{
    m_project.clear();
    m_projectPath.clear();
    m_waveform = {};
    m_currentFrame = 0;
    m_previewFrameCache.clear();
    if (m_previewDecoder)
        m_previewDecoder->setFile(QString());
    m_project.setTrimStartFrame(0);
    m_project.setTrimEndFrame(0);
    updateTrimLabel();
    refreshTimeline();
    m_preview->setCurrentFrame(0);
    m_preview->setVideoFrame(QImage());
    setWindowTitle("Reels Forge");
}

void MainWindow::onOpenProject()
{
    QString path = QFileDialog::getOpenFileName(this, "Open Project", {}, "Reels Forge Project (*.sfproj)");
    if (path.isEmpty()) return;

    Project p;
    if (!ProjectIO::load(path, p)) {
        QMessageBox::warning(this, "Error", "Failed to load project.");
        return;
    }
    m_project = p;
    m_projectPath = path;
    m_currentFrame = 0;

    if (!m_project.mediaFilePath().isEmpty()) {
        if (m_project.sourceFrameRate() <= 0 || m_project.sourceFrameRate() == 60.0) {
            double srcFps = VideoDecoder::frameRate(m_project.mediaFilePath());
            if (srcFps > 0)
                m_project.setSourceFrameRate(srcFps);
        }
        double durSec = VideoDecoder::durationSec(m_project.mediaFilePath());
        if (durSec > 0)
            m_project.setTotalSourceFrames(static_cast<qint64>(durSec * m_project.sourceFrameRate()));
        if (m_previewDecoder)
            m_previewDecoder->setFile(m_project.mediaFilePath());
        m_waveform = AudioDecoder::decodeForWaveform(m_project.mediaFilePath());
        m_audio->setFps(static_cast<int>(std::round(m_project.sourceFrameRate())));
        m_audio->loadFile(m_project.mediaFilePath());
    } else if (m_previewDecoder) {
        m_previewDecoder->setFile(QString());
    }

    updateTrimLabel();

    m_globalFontCombo->setCurrentFont(m_project.globalSubtitleFont());
    m_globalFontSize->setValue(static_cast<int>(m_project.globalSubtitleFontSize()));
    m_globalBoldCheck->setChecked(m_project.globalSubtitleFont().bold());
    m_globalItalicCheck->setChecked(m_project.globalSubtitleFont().italic());
    SubtitleEffect eff = m_project.globalEffect();
    m_globalPosYSpin->setValue(static_cast<int>(eff.focus.posY));
    m_globalSubtitleColor = m_project.globalSubtitleColor();
    m_globalColorBtn->setStyleSheet(QString("background-color: %1; border:1px solid #555;").arg(m_globalSubtitleColor.name()));

    m_titleEdit->blockSignals(true);
    m_titleEdit->setPlainText(m_project.videoTitle().text);
    m_titleEdit->blockSignals(false);
    m_titleFontCombo->setCurrentFont(m_project.videoTitle().font);
    m_titleFontSize->setValue(static_cast<int>(m_project.videoTitle().fontSize));
    m_titleBoldCheck->setChecked(m_project.videoTitle().font.bold());
    m_titleItalicCheck->setChecked(m_project.videoTitle().font.italic());
    m_titlePosYSpin->setValue(static_cast<int>(m_project.videoTitle().posY));
    m_titleColor = m_project.videoTitle().color;
    m_titleColorBtn->setStyleSheet(QString("background-color: %1; border:1px solid #555;").arg(m_titleColor.name()));

    refreshTimeline();
    m_preview->setCurrentFrame(0);
    updatePreviewVideoFrame();
    setWindowTitle(QString("Reels Forge — %1").arg(path));
}

void MainWindow::onSaveProject()
{
    if (m_projectPath.isEmpty()) { onSaveProjectAs(); return; }
    if (!ProjectIO::save(m_project, m_projectPath))
        QMessageBox::warning(this, "Error", "Failed to save project.");
}

void MainWindow::onSaveProjectAs()
{
    QString path = QFileDialog::getSaveFileName(this, "Save Project", {}, "Reels Forge Project (*.sfproj)");
    if (path.isEmpty()) return;
    m_projectPath = path;
    onSaveProject();
    setWindowTitle(QString("Reels Forge — %1").arg(path));
}

void MainWindow::onImportVideo()
{
    QString path = QFileDialog::getOpenFileName(this, "Import Video",
        {}, "Media (*.mp4 *.mkv *.avi *.mov *.webm *.mp3 *.wav *.aac *.flac *.ogg *.m4a);;All (*)");
    if (path.isEmpty()) return;

    m_project.setMediaFilePath(path);
    m_previewFrameCache.clear();
    if (m_previewDecoder)
        m_previewDecoder->setFile(path);
    m_waveform = AudioDecoder::decodeForWaveform(path);

    double srcFps = VideoDecoder::frameRate(path);
    if (srcFps <= 0) srcFps = 60.0;
    m_project.setSourceFrameRate(srcFps);
    int timelineFps = static_cast<int>(std::round(srcFps));

    double durSec = VideoDecoder::durationSec(path);
    if (durSec < 0 && m_waveform.valid)
        durSec = m_waveform.durationSec;
    if (durSec > 0) {
        qint64 totalF = static_cast<qint64>(durSec * srcFps);
        m_project.setTotalSourceFrames(totalF);
        m_project.setTrimStartFrame(0);
        m_project.setTrimEndFrame(totalF);
    }

    m_audio->setFps(timelineFps);
    m_audio->loadFile(path);
    updateTrimLabel();
    refreshTimeline();
    updatePreviewVideoFrame();
}

void MainWindow::onImportTxt()
{
    QString path = QFileDialog::getOpenFileName(this, "Import Subtitles", {}, "Text (*.txt);;All (*)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not open file.");
        return;
    }
    QString content = QString::fromUtf8(f.readAll());
    f.close();

    QStringList paragraphs = content.split("\n\n", Qt::SkipEmptyParts);
    if (paragraphs.isEmpty()) return;

    qint64 totalF = m_project.totalFrames();
    if (totalF <= 0) totalF = 60 * kFps;  // default 1 min

    m_project.clearSubtitles();

    for (int i = 0; i < paragraphs.size(); ++i) {
        SubtitleEntry e;
        e.id = m_project.nextSubtitleId();
        e.text = paragraphs[i].trimmed();
        if (e.text.isEmpty()) continue;
        e.keyFrame = (totalF * i) / paragraphs.size();
        m_project.addSubtitle(e);
    }
    refreshTimeline();
    m_preview->update();
}

void MainWindow::onExportVideo()
{
    if (m_project.totalFrames() <= 0) {
        QMessageBox::information(this, "Export", "Import video and set trim first.");
        return;
    }

    QString outPath = QFileDialog::getSaveFileName(this, "Export Video", {}, "MP4 Video (*.mp4)");
    if (outPath.isEmpty()) return;

    VideoExporter::ExportSettings cfg;
    cfg.outputPath = outPath;
    cfg.fps = 60;
    cfg.width = 1080;
    cfg.height = 1920;
    cfg.alphaChannel = false;

    auto *progress = new QProgressDialog("Exporting…", "Cancel", 0, 100, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setAutoClose(false);
    progress->setAutoReset(false);

    auto *exporter = new VideoExporter(this);
    QMetaObject::Connection cancelConn = connect(progress, &QProgressDialog::canceled, exporter, &VideoExporter::requestCancel);
    connect(exporter, &VideoExporter::progressChanged, this, [progress](int pct) {
        progress->setValue(pct);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    });

    progress->show();
    Project projectCopy = m_project;
    bool ok = exporter->exportVideo(projectCopy, cfg);

    disconnect(cancelConn);
    progress->close();
    progress->deleteLater();

    if (!ok) {
        if (exporter->wasCancelled())
            QMessageBox::information(this, "Export", "Export cancelled.");
        else {
            QString msg = exporter->lastError();
            QMessageBox::warning(this, "Export", msg.isEmpty() ? "Export failed." : msg);
        }
    } else {
        QMessageBox::information(this, "Export", "Export complete!");
    }
}

void MainWindow::pauseIfPlaying()
{
    if (m_audio->isPlaying()) {
        m_audio->togglePlayPause();
        m_refreshTimer->stop();
    }
}

void MainWindow::onFrameChanged(qint64 frame)
{
    pauseIfPlaying();
    seekToFrame(frame);
}

void MainWindow::onPlayPause()
{
    m_audio->togglePlayPause();
    if (m_audio->isPlaying())
        m_refreshTimer->start();
    else
        m_refreshTimer->stop();
}

void MainWindow::onStepForward()
{
    pauseIfPlaying();
    seekToFrame(m_currentFrame + 1);
}

void MainWindow::onStepBackward()
{
    pauseIfPlaying();
    seekToFrame(std::max<qint64>(0, m_currentFrame - 1));
}

void MainWindow::onStepForwardLarge()
{
    pauseIfPlaying();
    seekToFrame(m_currentFrame + static_cast<qint64>(std::round(m_project.sourceFrameRate())));
}

void MainWindow::onStepBackwardLarge()
{
    pauseIfPlaying();
    seekToFrame(std::max<qint64>(0, m_currentFrame - static_cast<qint64>(std::round(m_project.sourceFrameRate()))));
}

void MainWindow::onSubtitleSelected(int /*id*/) {}
void MainWindow::onSubtitleDeselected() {}

void MainWindow::onSubtitleMoved(int id, qint64 newKeyFrame)
{
    auto *sub = m_project.subtitleById(id);
    if (!sub) return;
    sub->keyFrame = newKeyFrame;
    m_project.sortSubtitles();
    refreshTimeline();
    m_preview->update();
}

void MainWindow::onDeleteSubtitle(int id)
{
    m_project.removeSubtitle(id);
    m_timeline->clearSelection();
    refreshTimeline();
    m_preview->update();
}

void MainWindow::onGlobalSubtitleFontChanged()
{
    QFont f = m_globalFontCombo->currentFont();
    f.setBold(m_globalBoldCheck->isChecked());
    f.setItalic(m_globalItalicCheck->isChecked());
    m_project.setGlobalSubtitleFont(f);
    m_project.setGlobalSubtitleFontSize(static_cast<float>(m_globalFontSize->value()));
    SubtitleEffect e = m_project.globalEffect();
    e.focus.posY = static_cast<float>(m_globalPosYSpin->value());
    m_project.setGlobalEffect(e);
    m_preview->update();
}

void MainWindow::onGlobalSubtitleColorChanged()
{
    m_project.setGlobalSubtitleColor(m_globalSubtitleColor);
    m_preview->update();
}

void MainWindow::onVideoTitleChanged()
{
    VideoTitle t = m_project.videoTitle();
    t.text = m_titleEdit->toPlainText();
    t.font = m_titleFontCombo->currentFont();
    t.font.setBold(m_titleBoldCheck->isChecked());
    t.font.setItalic(m_titleItalicCheck->isChecked());
    t.fontSize = static_cast<float>(m_titleFontSize->value());
    t.posY = static_cast<float>(m_titlePosYSpin->value());
    t.color = m_titleColor;
    m_project.setVideoTitle(t);
    m_preview->update();
}

void MainWindow::onSetTrimStart()
{
    qint64 sourceFrame = m_project.trimmedToSourceFrame(m_currentFrame);
    m_project.setTrimStartFrame(sourceFrame);
    if (m_project.trimEndFrame() <= m_project.trimStartFrame())
        m_project.setTrimEndFrame(m_project.trimStartFrame() + static_cast<qint64>(std::round(m_project.sourceFrameRate())));
    updateTrimLabel();
    refreshTimeline();
    m_currentFrame = 0;
    seekToFrame(0);
    updatePreviewVideoFrame();
}

void MainWindow::onSetTrimEnd()
{
    qint64 sourceFrame = m_project.trimmedToSourceFrame(m_currentFrame);
    m_project.setTrimEndFrame(sourceFrame);
    if (m_project.trimEndFrame() < m_project.trimStartFrame())
        m_project.setTrimStartFrame(std::max<qint64>(0, m_project.trimEndFrame() - static_cast<qint64>(std::round(m_project.sourceFrameRate()))));
    updateTrimLabel();
    refreshTimeline();
    if (m_currentFrame >= m_project.totalFrames())
        seekToFrame(std::max<qint64>(0, m_project.totalFrames() - 1));
    updatePreviewVideoFrame();
}

void MainWindow::onAudioPositionChanged(qint64 sourceFrame)
{
    qint64 trimStart = m_project.trimStartFrame();
    qint64 trimEnd = m_project.trimEndFrame();
    int timelineFps = static_cast<int>(std::round(m_project.sourceFrameRate()));

    if (sourceFrame >= trimEnd) {
        m_audio->pause();
        m_audio->seekToFrame(trimEnd, timelineFps);
    }
    if (sourceFrame < trimStart)
        m_currentFrame = 0;
    else if (sourceFrame >= trimEnd)
        m_currentFrame = std::max<qint64>(0, trimEnd - trimStart - 1);
    else
        m_currentFrame = sourceFrame - trimStart;

    m_timeline->setCurrentFrame(m_currentFrame);
    m_preview->setCurrentFrame(m_currentFrame);
    updatePreviewVideoFrame();
}

void MainWindow::onAudioDurationChanged(qint64 ms)
{
    int timelineFps = static_cast<int>(std::round(m_project.sourceFrameRate()));
    qint64 totalSourceFrames = ms * timelineFps / 1000;
    if (m_project.trimEndFrame() == 0 || m_project.trimEndFrame() > totalSourceFrames)
        m_project.setTrimEndFrame(totalSourceFrames);
    updateTrimLabel();
    refreshTimeline();
}

void MainWindow::onRefreshTick()
{
    int timelineFps = static_cast<int>(std::round(m_project.sourceFrameRate()));
    qint64 sourceFrame = m_audio->currentFrame(timelineFps);
    qint64 trimStart = m_project.trimStartFrame();
    qint64 trimEnd = m_project.trimEndFrame();
    qint64 frame;
    if (sourceFrame < trimStart)
        frame = 0;
    else if (sourceFrame >= trimEnd)
        frame = std::max<qint64>(0, trimEnd - trimStart - 1);
    else
        frame = sourceFrame - trimStart;

    if (frame != m_currentFrame) {
        m_currentFrame = frame;
        m_timeline->setCurrentFrame(frame);
        m_preview->setCurrentFrame(frame);
        updatePreviewVideoFrame();
    }
}

void MainWindow::updatePreviewVideoFrame()
{
    if (m_project.mediaFilePath().isEmpty()) {
        m_previewFrameCache.clear();
        m_preview->setVideoFrame(QImage());
        return;
    }
    auto it = m_previewFrameCache.find(m_currentFrame);
    if (it != m_previewFrameCache.end()) {
        m_preview->setVideoFrame(it.value());
        return;
    }
    if (m_previewDecoder) {
        double timeSec = m_project.trimmedToSourceFrame(m_currentFrame) / m_project.sourceFrameRate();
        m_previewDecoder->requestFrame(m_currentFrame, timeSec);
    }
}

void MainWindow::ensurePreviewDecodeRequested()
{
    (void)this;
}

void MainWindow::onPreviewFrameDecoded(qint64 frameIndex, QImage image)
{
    if (frameIndex < 0) return;
    while (m_previewFrameCache.size() >= kPreviewCacheMax) {
        qint64 oldest = m_previewFrameCache.firstKey();
        m_previewFrameCache.remove(oldest);
    }
    m_previewFrameCache.insert(frameIndex, image);
    if (frameIndex == m_currentFrame && !image.isNull())
        m_preview->setVideoFrame(image);
}

void MainWindow::updateTrimLabel()
{
    qint64 startF = m_project.trimStartFrame();
    qint64 endF   = m_project.trimEndFrame();
    double startSec = startF / m_project.sourceFrameRate();
    double endSec   = endF   / m_project.sourceFrameRate();
    int startM = static_cast<int>(startSec) / 60;
    double startS = startSec - startM * 60;
    int endM = static_cast<int>(endSec) / 60;
    double endS = endSec - endM * 60;
    m_trimLabel->setText(QString("%1:%2 – %3:%4")
        .arg(startM).arg(startS, 0, 'f', 1)
        .arg(endM).arg(endS, 0, 'f', 1));
}

void MainWindow::refreshTimeline()
{
    m_timeline->setFps(static_cast<int>(std::round(m_project.sourceFrameRate())));
    m_timeline->setTotalFrames(m_project.totalFrames());
    m_timeline->setTrimRange(m_project.trimStartFrame(), m_project.totalSourceFrames());
    m_timeline->setWaveform(m_waveform);
    m_timeline->setSubtitles(m_project.subtitles());
}

void MainWindow::seekToFrame(qint64 frame)
{
    frame = std::clamp(frame, qint64(0), m_project.totalFrames());
    m_currentFrame = frame;
    qint64 sourceFrame = m_project.trimmedToSourceFrame(frame);
    m_audio->seekToFrame(sourceFrame, static_cast<int>(std::round(m_project.sourceFrameRate())));
    m_timeline->setCurrentFrame(frame);
    m_preview->setCurrentFrame(frame);
    updatePreviewVideoFrame();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_previewDecoder)
        m_previewDecoder->requestQuit();
    if (m_decoderThread.joinable())
        m_decoderThread.join();
    QMainWindow::closeEvent(event);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() != QEvent::KeyPress)
        return QMainWindow::eventFilter(obj, event);

    auto *ke = static_cast<QKeyEvent *>(event);
    QWidget *fw = QApplication::focusWidget();
    bool isInput = qobject_cast<QLineEdit *>(fw) || qobject_cast<QTextEdit *>(fw)
                || qobject_cast<QAbstractSpinBox *>(fw) || qobject_cast<QComboBox *>(fw)
                || qobject_cast<QCheckBox *>(fw);
    bool hasCtrl = ke->modifiers() & Qt::ControlModifier;

    if (hasCtrl) {
        switch (ke->key()) {
        case Qt::Key_Space: onPlayPause(); return true;
        case Qt::Key_Right: onStepForward(); return true;
        case Qt::Key_Left:  onStepBackward(); return true;
        default: break;
        }
    }

    if (!isInput) {
        switch (ke->key()) {
        case Qt::Key_Space: onPlayPause(); return true;
        case Qt::Key_Right: onStepForward(); return true;
        case Qt::Key_Left:  onStepBackward(); return true;
        case Qt::Key_Delete:
            if (m_timeline->selectedSubtitleId() >= 0)
                onDeleteSubtitle(m_timeline->selectedSubtitleId());
            return true;
        default: break;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}
