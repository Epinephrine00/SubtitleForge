#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QFontComboBox>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include "model/subtitle.h"

class TextInputPanel : public QWidget {
    Q_OBJECT
public:
    explicit TextInputPanel(QWidget *parent = nullptr);

    // Populate fields from an existing subtitle (for editing).
    void loadSubtitle(const SubtitleEntry &entry);
    void clearFields();
    int  editingId() const { return m_editingId; }

signals:
    void addSubtitleRequested(const QString &text,
                              const QFont   &font,
                              const QColor  &color,
                              Qt::Alignment  align);
    void subtitleTextChanged(int id, const QString &text);
    void subtitleFontChanged(int id, const QFont &font);
    void subtitleColorChanged(int id, const QColor &color);
    void subtitleAlignmentChanged(int id, Qt::Alignment align);
    void deleteSubtitleRequested(int id);

private slots:
    void onAddClicked();
    void onColorClicked();
    void onFontChanged();
    void onAlignChanged();
    void onDeleteClicked();

private:
    QLineEdit     *m_textEdit     = nullptr;
    QFontComboBox *m_fontCombo    = nullptr;
    QCheckBox     *m_boldCheck    = nullptr;
    QCheckBox     *m_italicCheck  = nullptr;
    QPushButton   *m_colorBtn     = nullptr;
    QComboBox     *m_alignCombo   = nullptr;
    QPushButton   *m_addBtn       = nullptr;
    QPushButton   *m_deleteBtn    = nullptr;
    QLabel        *m_infoLabel    = nullptr;

    QColor m_currentColor = Qt::white;
    int    m_editingId    = -1;
};
