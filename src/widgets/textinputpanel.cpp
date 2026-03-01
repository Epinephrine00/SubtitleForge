#include "widgets/textinputpanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QColorDialog>

TextInputPanel::TextInputPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);

    m_infoLabel = new QLabel("Type text and press Enter to add a subtitle at the current time.");
    m_infoLabel->setWordWrap(true);
    root->addWidget(m_infoLabel);

    // text input
    m_textEdit = new QLineEdit;
    m_textEdit->setPlaceholderText("Subtitle text…");
    connect(m_textEdit, &QLineEdit::returnPressed, this, &TextInputPanel::onAddClicked);
    root->addWidget(m_textEdit);

    // font row
    auto *fontRow = new QHBoxLayout;
    m_fontCombo = new QFontComboBox;
    m_fontCombo->setCurrentFont(QFont("Arial"));
    connect(m_fontCombo, &QFontComboBox::currentFontChanged,
            this, &TextInputPanel::onFontChanged);
    fontRow->addWidget(m_fontCombo, 1);

    m_boldCheck = new QCheckBox("B");
    m_boldCheck->setStyleSheet("font-weight:bold;");
    connect(m_boldCheck, &QCheckBox::toggled, this, &TextInputPanel::onFontChanged);
    fontRow->addWidget(m_boldCheck);

    m_italicCheck = new QCheckBox("I");
    m_italicCheck->setStyleSheet("font-style:italic;");
    connect(m_italicCheck, &QCheckBox::toggled, this, &TextInputPanel::onFontChanged);
    fontRow->addWidget(m_italicCheck);
    root->addLayout(fontRow);

    // colour + alignment row
    auto *optRow = new QHBoxLayout;
    m_colorBtn = new QPushButton;
    m_colorBtn->setFixedSize(28, 28);
    m_colorBtn->setStyleSheet("background-color: white; border:1px solid #555;");
    connect(m_colorBtn, &QPushButton::clicked, this, &TextInputPanel::onColorClicked);
    optRow->addWidget(new QLabel("Color:"));
    optRow->addWidget(m_colorBtn);

    m_alignCombo = new QComboBox;
    m_alignCombo->addItem("Left",   Qt::AlignLeft);
    m_alignCombo->addItem("Center", Qt::AlignHCenter);
    m_alignCombo->addItem("Right",  Qt::AlignRight);
    m_alignCombo->setCurrentIndex(1);
    connect(m_alignCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TextInputPanel::onAlignChanged);
    optRow->addWidget(new QLabel("Align:"));
    optRow->addWidget(m_alignCombo);
    optRow->addStretch();
    root->addLayout(optRow);

    // action buttons
    auto *btnRow = new QHBoxLayout;
    m_addBtn = new QPushButton("Add Subtitle");
    connect(m_addBtn, &QPushButton::clicked, this, &TextInputPanel::onAddClicked);
    btnRow->addWidget(m_addBtn);

    m_deleteBtn = new QPushButton("Delete");
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &TextInputPanel::onDeleteClicked);
    btnRow->addWidget(m_deleteBtn);
    root->addLayout(btnRow);

    root->addStretch();
}

void TextInputPanel::loadSubtitle(const SubtitleEntry &entry)
{
    m_editingId = entry.id;
    m_textEdit->setText(entry.text);
    m_fontCombo->setCurrentFont(entry.font);
    m_boldCheck->setChecked(entry.font.bold());
    m_italicCheck->setChecked(entry.font.italic());
    m_currentColor = entry.textColor;
    m_colorBtn->setStyleSheet(
        QString("background-color: %1; border:1px solid #555;")
            .arg(m_currentColor.name()));
    int alignIdx = 1;
    if (entry.alignment & Qt::AlignLeft)  alignIdx = 0;
    if (entry.alignment & Qt::AlignRight) alignIdx = 2;
    m_alignCombo->setCurrentIndex(alignIdx);
    m_addBtn->setText("Update");
    m_deleteBtn->setEnabled(true);
    m_infoLabel->setText(QString("Editing subtitle #%1").arg(entry.id));
}

void TextInputPanel::clearFields()
{
    m_editingId = -1;
    m_textEdit->clear();
    m_addBtn->setText("Add Subtitle");
    m_deleteBtn->setEnabled(false);
    m_infoLabel->setText("Type text and press Enter to add a subtitle at the current time.");
}

void TextInputPanel::onAddClicked()
{
    QString text = m_textEdit->text();

    QFont f = m_fontCombo->currentFont();
    f.setBold(m_boldCheck->isChecked());
    f.setItalic(m_italicCheck->isChecked());
    auto align = static_cast<Qt::Alignment>(
        m_alignCombo->currentData().toInt());

    if (m_editingId >= 0) {
        emit subtitleTextChanged(m_editingId, text);
        emit subtitleFontChanged(m_editingId, f);
        emit subtitleColorChanged(m_editingId, m_currentColor);
        emit subtitleAlignmentChanged(m_editingId, align);
    } else {
        emit addSubtitleRequested(text, f, m_currentColor, align);
        m_textEdit->clear();
    }
}

void TextInputPanel::onColorClicked()
{
    QColor c = QColorDialog::getColor(m_currentColor, this, "Text Color");
    if (!c.isValid()) return;
    m_currentColor = c;
    m_colorBtn->setStyleSheet(
        QString("background-color: %1; border:1px solid #555;").arg(c.name()));
    if (m_editingId >= 0)
        emit subtitleColorChanged(m_editingId, c);
}

void TextInputPanel::onFontChanged()
{
    if (m_editingId < 0) return;
    QFont f = m_fontCombo->currentFont();
    f.setBold(m_boldCheck->isChecked());
    f.setItalic(m_italicCheck->isChecked());
    emit subtitleFontChanged(m_editingId, f);
}

void TextInputPanel::onAlignChanged()
{
    if (m_editingId < 0) return;
    auto align = static_cast<Qt::Alignment>(
        m_alignCombo->currentData().toInt());
    emit subtitleAlignmentChanged(m_editingId, align);
}

void TextInputPanel::onDeleteClicked()
{
    if (m_editingId >= 0)
        emit deleteSubtitleRequested(m_editingId);
}
