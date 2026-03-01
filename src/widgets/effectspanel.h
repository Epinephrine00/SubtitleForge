#pragma once

#include <QWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include "model/subtitleeffect.h"

// Editor for a single animation stage
class StageEditor : public QGroupBox {
    Q_OBJECT
public:
    StageEditor(const QString &title, bool canDisable,
                QWidget *parent = nullptr);

    void   setParams(const StageParams &p);
    StageParams params() const;

signals:
    void changed();

private:
    void onAnyChange();

    QCheckBox      *m_enableCheck = nullptr;
    QDoubleSpinBox *m_posX     = nullptr;
    QDoubleSpinBox *m_posY     = nullptr;
    QDoubleSpinBox *m_scale    = nullptr;
    QDoubleSpinBox *m_blur     = nullptr;
    QDoubleSpinBox *m_opacity  = nullptr;
    QDoubleSpinBox *m_fontSize = nullptr;
    bool m_canDisable;
};

// Panel holding fade-duration + 5 stage editors
class EffectsPanel : public QWidget {
    Q_OBJECT
public:
    explicit EffectsPanel(QWidget *parent = nullptr);

    void setEffect(const SubtitleEffect &eff);
    SubtitleEffect effect() const;

signals:
    void effectChanged(const SubtitleEffect &eff);

private slots:
    void onStageChanged();

private:
    QSpinBox    *m_fadeDuration = nullptr;
    StageEditor *m_intro    = nullptr;
    StageEditor *m_approach = nullptr;
    StageEditor *m_focus    = nullptr;
    StageEditor *m_recede   = nullptr;
    StageEditor *m_outro    = nullptr;
};
