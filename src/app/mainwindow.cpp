#include "app/mainwindow.h"
#include "widgets/textinputpanel.h"
#include "widgets/effectspanel.h"
#include "widgets/timelinewidget.h"
#include "widgets/previewwidget.h"
#include "io/projectio.h"
#include "video/videoexporter.h"

#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressDialog>
#include <QComboBox>
#include <QLabel>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QThread>
#include <QApplication>
#include <QCheckBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QAbstractSpinBox>
#include <QKeyEvent>
#include <QSlider>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("SubtitleForge");
    setupUi();
    setupMenus();
    connectSignals();

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(16); // ~60 Hz polling
    connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::onRefreshTick);

    m_effectPanel->setEffect(m_project.globalEffect());
    syncSettingsCombos();

    qApp->installEventFilter(this);
}

// ---------- UI setup -------------------------------------------------------

void MainWindow::setupUi()
{
    m_textPanel   = new TextInputPanel;
    m_effectPanel = new EffectsPanel;
    m_timeline    = new TimelineWidget;
    m_preview     = new PreviewWidget;
    m_audio       = new AudioPlayback(this);

    m_preview->setProject(&m_project);

    // Output settings group
    auto *settingsGroup = new QGroupBox("Output Settings");
    auto *settingsForm  = new QFormLayout(settingsGroup);
    settingsForm->setContentsMargins(6, 6, 6, 6);

    m_fpsCombo = new QComboBox;
    m_fpsCombo->addItem("24 fps", 24);
    m_fpsCombo->addItem("30 fps", 30);
    m_fpsCombo->addItem("60 fps", 60);
    m_fpsCombo->setCurrentIndex(1);
    settingsForm->addRow("FPS:", m_fpsCombo);

    m_resCombo = new QComboBox;
    m_resCombo->addItem("720p  (1280\u00d7720)",   QSize(1280, 720));
    m_resCombo->addItem("1080p (1920\u00d71080)",  QSize(1920, 1080));
    m_resCombo->addItem("1440p (2560\u00d71440)",  QSize(2560, 1440));
    m_resCombo->addItem("2160p (3840\u00d72160)",  QSize(3840, 2160));
    m_resCombo->setCurrentIndex(1);
    settingsForm->addRow("Resolution:", m_resCombo);

    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(static_cast<int>(m_audio->volume() * 100));
    m_volumeSlider->setToolTip("Audio volume (saved across sessions)");
    settingsForm->addRow("Volume:", m_volumeSlider);

    // Left column: text panel + settings
    auto *leftColumn = new QWidget;
    auto *leftLayout = new QVBoxLayout(leftColumn);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(m_textPanel);
    leftLayout->addWidget(settingsGroup);

    // Top: left column | effects panel
    auto *topSplitter = new QSplitter(Qt::Horizontal);
    topSplitter->addWidget(leftColumn);
    topSplitter->addWidget(m_effectPanel);
    topSplitter->setStretchFactor(0, 1);
    topSplitter->setStretchFactor(1, 1);

    // Main vertical splitter
    auto *mainSplitter = new QSplitter(Qt::Vertical);
    mainSplitter->addWidget(topSplitter);
    mainSplitter->addWidget(m_preview);
    mainSplitter->addWidget(m_timeline);
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 3);
    mainSplitter->setStretchFactor(2, 0);

    setCentralWidget(mainSplitter);
}

void MainWindow::setupMenus()
{
    auto *file = menuBar()->addMenu("&File");
    file->addAction("&New Project",     this, &MainWindow::onNewProject,   QKeySequence::New);
    file->addAction("&Open Project…",   this, &MainWindow::onOpenProject,  QKeySequence::Open);
    file->addAction("&Save Project",    this, &MainWindow::onSaveProject,  QKeySequence::Save);
    file->addAction("Save Project &As…",this, &MainWindow::onSaveProjectAs,QKeySequence("Ctrl+Shift+S"));
    file->addSeparator();
    file->addAction("&Import Audio/Video…", this, &MainWindow::onImportAudio, QKeySequence("Ctrl+I"));
    file->addAction("&Export Video…",       this, &MainWindow::onExportVideo, QKeySequence("Ctrl+E"));
    file->addSeparator();
    file->addAction("E&xit", this, &QWidget::close, QKeySequence::Quit);
}

void MainWindow::connectSignals()
{
    // timeline
    connect(m_timeline, &TimelineWidget::frameChanged,        this, &MainWindow::onFrameChanged);
    connect(m_timeline, &TimelineWidget::playPauseRequested,  this, &MainWindow::onPlayPause);
    connect(m_timeline, &TimelineWidget::stepForward,         this, &MainWindow::onStepForward);
    connect(m_timeline, &TimelineWidget::stepBackward,        this, &MainWindow::onStepBackward);
    connect(m_timeline, &TimelineWidget::stepForwardLarge,    this, &MainWindow::onStepForwardLarge);
    connect(m_timeline, &TimelineWidget::stepBackwardLarge,   this, &MainWindow::onStepBackwardLarge);
    connect(m_timeline, &TimelineWidget::subtitleSelected,    this, &MainWindow::onSubtitleSelected);
    connect(m_timeline, &TimelineWidget::subtitleDeselected,  this, &MainWindow::onSubtitleDeselected);
    connect(m_timeline, &TimelineWidget::subtitleMoved,       this, &MainWindow::onSubtitleMoved);
    connect(m_timeline, &TimelineWidget::deleteSubtitleRequested,
            this, &MainWindow::onDeleteSubtitle);

    // text panel
    connect(m_textPanel, &TextInputPanel::addSubtitleRequested,      this, &MainWindow::onAddSubtitle);
    connect(m_textPanel, &TextInputPanel::subtitleTextChanged,       this, &MainWindow::onSubtitleTextChanged);
    connect(m_textPanel, &TextInputPanel::subtitleFontChanged,       this, &MainWindow::onSubtitleFontChanged);
    connect(m_textPanel, &TextInputPanel::subtitleColorChanged,      this, &MainWindow::onSubtitleColorChanged);
    connect(m_textPanel, &TextInputPanel::subtitleAlignmentChanged,  this, &MainWindow::onSubtitleAlignmentChanged);
    connect(m_textPanel, &TextInputPanel::deleteSubtitleRequested,   this, &MainWindow::onDeleteSubtitle);

    // effects (global)
    connect(m_effectPanel, &EffectsPanel::effectChanged, this, &MainWindow::onEffectChanged);

    // audio
    connect(m_audio, &AudioPlayback::positionChanged,      this, &MainWindow::onAudioPositionChanged);
    connect(m_audio, &AudioPlayback::durationChanged,       this, &MainWindow::onAudioDurationChanged);

    // output settings
    connect(m_fpsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onProjectFpsChanged);
    connect(m_resCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onProjectResolutionChanged);
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        m_audio->setVolume(value / 100.0f);
    });
}

// ---------- menu actions ---------------------------------------------------

void MainWindow::onNewProject()
{
    m_project.clear();
    m_projectPath.clear();
    m_waveform = {};
    m_currentFrame = 0;
    refreshTimeline();
    m_textPanel->clearFields();
    m_effectPanel->setEffect(m_project.globalEffect());
    syncSettingsCombos();
    m_preview->setCurrentFrame(0);
    setWindowTitle("SubtitleForge");
}

void MainWindow::onOpenProject()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Open Project", {}, "SubtitleForge Project (*.sfproj)");
    if (path.isEmpty()) return;

    Project p;
    if (!ProjectIO::load(path, p)) {
        QMessageBox::warning(this, "Error", "Failed to load project.");
        return;
    }
    m_project     = p;
    m_projectPath = path;
    m_currentFrame = 0;
    m_audio->setFps(m_project.fps());

    if (!m_project.audioFilePath().isEmpty()) {
        m_waveform = AudioDecoder::decodeForWaveform(m_project.audioFilePath());
        m_audio->loadFile(m_project.audioFilePath());
    }

    refreshTimeline();
    m_effectPanel->setEffect(m_project.globalEffect());
    syncSettingsCombos();
    m_preview->setCurrentFrame(0);
    setWindowTitle(QString("SubtitleForge — %1").arg(path));
}

void MainWindow::onSaveProject()
{
    if (m_projectPath.isEmpty()) { onSaveProjectAs(); return; }
    if (!ProjectIO::save(m_project, m_projectPath))
        QMessageBox::warning(this, "Error", "Failed to save project.");
}

void MainWindow::onSaveProjectAs()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Save Project", {}, "SubtitleForge Project (*.sfproj)");
    if (path.isEmpty()) return;
    m_projectPath = path;
    onSaveProject();
    setWindowTitle(QString("SubtitleForge — %1").arg(path));
}

void MainWindow::onImportAudio()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Import Audio / Video", {},
        "Media Files (*.mp3 *.wav *.aac *.flac *.ogg *.m4a "
        "*.mp4 *.mkv *.avi *.mov *.webm);;All Files (*)");
    if (path.isEmpty()) return;

    m_project.setAudioFilePath(path);
    m_waveform = AudioDecoder::decodeForWaveform(path);

    if (m_waveform.valid) {
        qint64 totalFrames = static_cast<qint64>(
            m_waveform.durationSec * m_project.fps());
        m_project.setTotalFrames(totalFrames);
    }

    m_audio->setFps(m_project.fps());
    m_audio->loadFile(path);
    refreshTimeline();
}

void MainWindow::onExportVideo()
{
    if (m_project.totalFrames() <= 0) {
        QMessageBox::information(this, "Export",
            "Import audio first to determine duration.");
        return;
    }

    // --- settings dialog ---
    QDialog dlg(this);
    dlg.setWindowTitle("Export Video");
    auto *form = new QFormLayout(&dlg);

    auto *fpsCombo = new QComboBox;
    fpsCombo->addItem("24 fps", 24);
    fpsCombo->addItem("30 fps", 30);
    fpsCombo->addItem("60 fps", 60);
    for (int i = 0; i < fpsCombo->count(); ++i) {
        if (fpsCombo->itemData(i).toInt() == m_project.fps()) {
            fpsCombo->setCurrentIndex(i); break;
        }
    }
    form->addRow("Frame rate:", fpsCombo);

    auto *resCombo = new QComboBox;
    resCombo->addItem("720p  (1280\u00d7720)",   QSize(1280, 720));
    resCombo->addItem("1080p (1920\u00d71080)",  QSize(1920, 1080));
    resCombo->addItem("1440p (2560\u00d71440)",  QSize(2560, 1440));
    resCombo->addItem("2160p (3840\u00d72160)",  QSize(3840, 2160));
    for (int i = 0; i < resCombo->count(); ++i) {
        QSize s = resCombo->itemData(i).toSize();
        if (s.width() == m_project.videoWidth() &&
            s.height() == m_project.videoHeight()) {
            resCombo->setCurrentIndex(i); break;
        }
    }
    form->addRow("Resolution:", resCombo);

    auto *alphaCheck = new QCheckBox("Export with alpha channel (MOV / ProRes 4444)");
    alphaCheck->setToolTip(
        "Transparent background instead of green.\n"
        "Use this for proper semi-transparent text compositing in Premiere.");
    form->addRow(alphaCheck);

    auto *btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(btns);

    if (dlg.exec() != QDialog::Accepted) return;

    bool useAlpha = alphaCheck->isChecked();
    QString filter = useAlpha ? "MOV Video (*.mov)" : "MP4 Video (*.mp4)";
    QString outPath = QFileDialog::getSaveFileName(
        this, "Export Video", {}, filter);
    if (outPath.isEmpty()) return;

    VideoExporter::ExportSettings cfg;
    cfg.outputPath    = outPath;
    cfg.alphaChannel  = useAlpha;
    cfg.fps    = fpsCombo->currentData().toInt();
    QSize res  = resCombo->currentData().toSize();
    cfg.width  = res.width();
    cfg.height = res.height();

    // recalculate total frames for chosen fps
    qint64 totalFrames = static_cast<qint64>(
        m_waveform.durationSec * cfg.fps);

    Project exportProject = m_project;
    exportProject.setFps(cfg.fps);
    exportProject.setResolution(cfg.width, cfg.height);
    exportProject.setTotalFrames(totalFrames);

    auto *progress = new QProgressDialog("Exporting…", "Cancel", 0, 100, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setAutoClose(false);
    progress->setAutoReset(false);

    auto *exporter = new VideoExporter;
    auto *thread   = new QThread;
    exporter->moveToThread(thread);

    connect(exporter, &VideoExporter::progressChanged,
            progress, &QProgressDialog::setValue);
    connect(progress, &QProgressDialog::canceled,
            exporter, &VideoExporter::requestCancel);

    Project projectCopy = exportProject;
    connect(thread, &QThread::started, [exporter, projectCopy, cfg]() {
        exporter->exportVideo(projectCopy, cfg);
    });
    connect(exporter, &VideoExporter::exportFinished,
            this, [this, progress, thread, exporter](bool ok, const QString &err) {
        progress->close();
        progress->deleteLater();
        thread->quit();
        if (!ok && !err.isEmpty())
            QMessageBox::warning(this, "Export", err);
        else if (ok)
            QMessageBox::information(this, "Export", "Export complete!");
        exporter->deleteLater();
    });
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    thread->start();
    progress->exec();
}

// ---------- timeline -------------------------------------------------------

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
    seekToFrame(m_currentFrame + m_project.fps());
}

void MainWindow::onStepBackwardLarge()
{
    pauseIfPlaying();
    seekToFrame(std::max<qint64>(0, m_currentFrame - m_project.fps()));
}

void MainWindow::onSubtitleSelected(int id)
{
    const auto *sub = m_project.subtitleById(id);
    if (!sub) return;
    m_textPanel->loadSubtitle(*sub);
}

void MainWindow::onSubtitleDeselected()
{
    m_textPanel->clearFields();
}

void MainWindow::onSubtitleMoved(int id, qint64 newKeyFrame)
{
    auto *sub = m_project.subtitleById(id);
    if (!sub) return;
    sub->keyFrame = newKeyFrame;
    m_project.sortSubtitles();
    refreshTimeline();
    m_preview->update();
}

// ---------- text panel -----------------------------------------------------

void MainWindow::onAddSubtitle(const QString &text, const QFont &font,
                               const QColor &color, Qt::Alignment align)
{
    SubtitleEntry e;
    e.id        = m_project.nextSubtitleId();
    e.keyFrame  = m_currentFrame;
    e.text      = text;
    e.font      = font;
    e.textColor = color;
    e.alignment = align;
    m_project.addSubtitle(e);
    refreshTimeline();
    m_preview->update();
}

void MainWindow::onSubtitleTextChanged(int id, const QString &text)
{
    auto *s = m_project.subtitleById(id);
    if (s) { s->text = text; m_preview->update(); }
}

void MainWindow::onSubtitleFontChanged(int id, const QFont &font)
{
    auto *s = m_project.subtitleById(id);
    if (s) { s->font = font; m_preview->update(); }
}

void MainWindow::onSubtitleColorChanged(int id, const QColor &color)
{
    auto *s = m_project.subtitleById(id);
    if (s) { s->textColor = color; m_preview->update(); }
}

void MainWindow::onSubtitleAlignmentChanged(int id, Qt::Alignment align)
{
    auto *s = m_project.subtitleById(id);
    if (s) { s->alignment = align; m_preview->update(); }
}

void MainWindow::onDeleteSubtitle(int id)
{
    m_project.removeSubtitle(id);
    m_textPanel->clearFields();
    m_timeline->clearSelection();
    refreshTimeline();
    m_preview->update();
}

// ---------- effects --------------------------------------------------------

void MainWindow::onEffectChanged(const SubtitleEffect &eff)
{
    m_project.setGlobalEffect(eff);
    m_preview->update();
}

// ---------- audio ----------------------------------------------------------

void MainWindow::onAudioPositionChanged(qint64 frame)
{
    m_currentFrame = frame;
    m_timeline->setCurrentFrame(frame);
    m_preview->setCurrentFrame(frame);
}

void MainWindow::onAudioDurationChanged(qint64 ms)
{
    qint64 totalFrames = ms * m_project.fps() / 1000;
    m_project.setTotalFrames(totalFrames);
    m_timeline->setTotalFrames(totalFrames);
}

void MainWindow::onRefreshTick()
{
    qint64 frame = m_audio->currentFrame(m_project.fps());
    if (frame != m_currentFrame) {
        m_currentFrame = frame;
        m_timeline->setCurrentFrame(frame);
        m_preview->setCurrentFrame(frame);
    }
}

// ---------- output settings ------------------------------------------------

void MainWindow::onProjectFpsChanged(int /*index*/)
{
    int newFps = m_fpsCombo->currentData().toInt();
    int oldFps = m_project.fps();
    if (newFps == oldFps) return;

    m_project.convertSubtitleFrames(oldFps, newFps);
    m_project.setFps(newFps);

    if (m_waveform.valid) {
        qint64 totalFrames = static_cast<qint64>(m_waveform.durationSec * newFps);
        m_project.setTotalFrames(totalFrames);
    }

    m_currentFrame = m_currentFrame * newFps / oldFps;
    m_audio->setFps(newFps);
    refreshTimeline();
    seekToFrame(m_currentFrame);
}

void MainWindow::onProjectResolutionChanged(int /*index*/)
{
    QSize res = m_resCombo->currentData().toSize();
    m_project.setResolution(res.width(), res.height());
    m_preview->update();
}

void MainWindow::syncSettingsCombos()
{
    m_fpsCombo->blockSignals(true);
    for (int i = 0; i < m_fpsCombo->count(); ++i) {
        if (m_fpsCombo->itemData(i).toInt() == m_project.fps()) {
            m_fpsCombo->setCurrentIndex(i);
            break;
        }
    }
    m_fpsCombo->blockSignals(false);

    m_resCombo->blockSignals(true);
    for (int i = 0; i < m_resCombo->count(); ++i) {
        QSize s = m_resCombo->itemData(i).toSize();
        if (s.width() == m_project.videoWidth() &&
            s.height() == m_project.videoHeight()) {
            m_resCombo->setCurrentIndex(i);
            break;
        }
    }
    m_resCombo->blockSignals(false);
}

// ---------- event filter (global keyboard shortcuts) -----------------------

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() != QEvent::KeyPress)
        return QMainWindow::eventFilter(obj, event);

    auto *ke = static_cast<QKeyEvent *>(event);
    QWidget *fw = QApplication::focusWidget();
    bool isInput = qobject_cast<QLineEdit *>(fw)
                || qobject_cast<QAbstractSpinBox *>(fw)
                || qobject_cast<QComboBox *>(fw);
    bool hasCtrl = ke->modifiers() & Qt::ControlModifier;

    // Ctrl+shortcuts work even inside text input widgets
    if (hasCtrl) {
        switch (ke->key()) {
        case Qt::Key_Space:
            onPlayPause();
            return true;
        case Qt::Key_Right:
            onStepForward();
            return true;
        case Qt::Key_Left:
            onStepBackward();
            return true;
        default:
            break;
        }
    }

    // Non-Ctrl shortcuts only when NOT in a text input widget
    if (!isInput) {
        switch (ke->key()) {
        case Qt::Key_Space:
            onPlayPause();
            return true;
        case Qt::Key_Right:
            onStepForward();
            return true;
        case Qt::Key_Left:
            onStepBackward();
            return true;
        case Qt::Key_Delete:
            if (m_timeline->selectedSubtitleId() >= 0)
                onDeleteSubtitle(m_timeline->selectedSubtitleId());
            return true;
        default:
            break;
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

// ---------- helpers --------------------------------------------------------

void MainWindow::refreshTimeline()
{
    m_timeline->setFps(m_project.fps());
    m_timeline->setTotalFrames(m_project.totalFrames());
    m_timeline->setWaveform(m_waveform);
    m_timeline->setSubtitles(m_project.subtitles());
}

void MainWindow::seekToFrame(qint64 frame)
{
    frame = std::clamp(frame, qint64(0), m_project.totalFrames());
    m_currentFrame = frame;
    m_audio->seekToFrame(frame, m_project.fps());
    m_timeline->setCurrentFrame(frame);
    m_preview->setCurrentFrame(frame);
}
