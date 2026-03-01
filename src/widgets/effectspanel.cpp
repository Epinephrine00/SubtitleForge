#include "widgets/effectspanel.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QScrollArea>

// ===== StageEditor =========================================================

StageEditor::StageEditor(const QString &title, bool canDisable, QWidget *parent)
    : QGroupBox(title, parent), m_canDisable(canDisable)
{
    auto *form = new QFormLayout(this);
    form->setContentsMargins(4, 8, 4, 4);
    form->setSpacing(3);

    if (canDisable) {
        m_enableCheck = new QCheckBox("Enabled");
        m_enableCheck->setChecked(false);
        form->addRow(m_enableCheck);
        connect(m_enableCheck, &QCheckBox::toggled,
                this, &StageEditor::onAnyChange);
    }

    auto makeSpin = [&](double min, double max, double val, int dec,
                        const QString &suffix = {}) {
        auto *s = new QDoubleSpinBox;
        s->setRange(min, max);
        s->setValue(val);
        s->setDecimals(dec);
        if (!suffix.isEmpty()) s->setSuffix(suffix);
        connect(s, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &StageEditor::onAnyChange);
        return s;
    };

    m_posX     = makeSpin(-9999, 9999, 0,   0, " px");
    m_posY     = makeSpin(-9999, 9999, 0,   0, " px");
    m_scale    = makeSpin(1,     1000, 100,  1, " %");
    m_blur     = makeSpin(0,     200,  0,    1);
    m_opacity  = makeSpin(0,     100,  100,  1, " %");
    m_fontSize = makeSpin(1,     999,  48,   1, " pt");

    form->addRow("Position X:", m_posX);
    form->addRow("Position Y:", m_posY);
    form->addRow("Scale:",      m_scale);
    form->addRow("Blur:",       m_blur);
    form->addRow("Opacity:",    m_opacity);
    form->addRow("Font size:",  m_fontSize);
}

void StageEditor::setParams(const StageParams &p)
{
    blockSignals(true);
    if (m_enableCheck) m_enableCheck->setChecked(p.enabled);
    m_posX->setValue(p.posX);
    m_posY->setValue(p.posY);
    m_scale->setValue(p.scale);
    m_blur->setValue(p.blur);
    m_opacity->setValue(p.opacity);
    m_fontSize->setValue(p.fontSize);
    blockSignals(false);
}

StageParams StageEditor::params() const
{
    StageParams p;
    p.enabled  = m_canDisable ? (m_enableCheck ? m_enableCheck->isChecked() : true) : true;
    p.posX     = static_cast<float>(m_posX->value());
    p.posY     = static_cast<float>(m_posY->value());
    p.scale    = static_cast<float>(m_scale->value());
    p.blur     = static_cast<float>(m_blur->value());
    p.opacity  = static_cast<float>(m_opacity->value());
    p.fontSize = static_cast<float>(m_fontSize->value());
    return p;
}

void StageEditor::onAnyChange() { emit changed(); }

// ===== EffectsPanel ========================================================

EffectsPanel::EffectsPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(6, 6, 6, 6);

    // fade duration
    auto *fdRow = new QHBoxLayout;
    fdRow->addWidget(new QLabel("Fade Duration:"));
    m_fadeDuration = new QSpinBox;
    m_fadeDuration->setRange(0, 9999);
    m_fadeDuration->setValue(12);
    m_fadeDuration->setSuffix(" frames");
    connect(m_fadeDuration, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &EffectsPanel::onStageChanged);
    fdRow->addWidget(m_fadeDuration);
    outer->addLayout(fdRow);

    // scrollable area for stages
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    auto *inner  = new QWidget;
    auto *vbox   = new QVBoxLayout(inner);
    vbox->setContentsMargins(0, 0, 0, 0);

    m_intro    = new StageEditor("Intro",    true);
    m_approach = new StageEditor("Approach", true);
    m_focus    = new StageEditor("Focus",    false);
    m_recede   = new StageEditor("Recede",   true);
    m_outro    = new StageEditor("Outro",    true);

    for (auto *ed : {m_intro, m_approach, m_focus, m_recede, m_outro}) {
        vbox->addWidget(ed);
        connect(ed, &StageEditor::changed, this, &EffectsPanel::onStageChanged);
    }
    vbox->addStretch();

    scroll->setWidget(inner);
    outer->addWidget(scroll);
}

void EffectsPanel::setEffect(const SubtitleEffect &eff)
{
    blockSignals(true);
    m_fadeDuration->setValue(eff.fadeDurationFrames);
    m_intro->setParams(eff.intro);
    m_approach->setParams(eff.approach);
    m_focus->setParams(eff.focus);
    m_recede->setParams(eff.recede);
    m_outro->setParams(eff.outro);
    blockSignals(false);
}

SubtitleEffect EffectsPanel::effect() const
{
    SubtitleEffect e;
    e.fadeDurationFrames = m_fadeDuration->value();
    e.intro    = m_intro->params();
    e.approach = m_approach->params();
    e.focus    = m_focus->params();
    e.focus.enabled = true; // always on
    e.recede   = m_recede->params();
    e.outro    = m_outro->params();
    return e;
}

void EffectsPanel::onStageChanged()
{
    emit effectChanged(effect());
}
