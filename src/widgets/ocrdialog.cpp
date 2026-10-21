// SPDX-License-Identifier: GPL-3.0-or-later
#include "ocrdialog.h"
#include "utils/localocr.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QVBoxLayout>

OcrDialog::OcrDialog(const QImage& image, QWidget* parent)
  : QDialog(parent)
{
    setWindowTitle(tr("Extract text locally"));
    auto* layout = new QVBoxLayout(this);
    auto* notice = new QLabel(
      tr("Local OCR with Tesseract. No upload or temporary image file. "
         "Review and correct the recognized text before copying."),
      this);
    notice->setWordWrap(true);
    layout->addWidget(notice);
    auto* language = new QComboBox(this);
    language->setAccessibleName(tr("OCR language"));
    language->addItem(tr("French + English"), "fra+eng");
    language->addItem(tr("French"), "fra");
    language->addItem(tr("English"), "eng");
    layout->addWidget(language);
    auto* text = new QPlainTextEdit(this);
    text->setAccessibleName(tr("Recognized text (editable)"));
    layout->addWidget(text);
    auto* status =
      new QLabel(tr("Select a language, then recognize the text."), this);
    status->setWordWrap(true);
    status->setObjectName("ocrStatus");
    layout->addWidget(status);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    m_buttons = buttons;
    // Start compact so the horizontal button row cannot impose a large
    // minimum width before the first resize event.
    buttons->setOrientation(Qt::Vertical);
    buttons->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    auto* recognize =
      buttons->addButton(tr("Recognize"), QDialogButtonBox::ActionRole);
    auto* cancel =
      buttons->addButton(tr("Cancel OCR"), QDialogButtonBox::ActionRole);
    auto* copy =
      buttons->addButton(tr("Copy text"), QDialogButtonBox::ActionRole);
    cancel->setEnabled(false);
    copy->setEnabled(false);
    layout->addWidget(buttons);
    auto* engine = new LocalOcr(this);
    const auto setBusy = [=](bool busy) {
        language->setEnabled(!busy);
        recognize->setEnabled(!busy);
        cancel->setEnabled(busy);
        text->setReadOnly(busy);
        copy->setEnabled(!busy && !text->toPlainText().isEmpty());
    };
    connect(recognize, &QPushButton::clicked, this, [=]() {
        setBusy(true);
        status->setText(tr("Recognizing locally…"));
        engine->start(image, language->currentData().toString());
    });
    connect(cancel, &QPushButton::clicked, engine, &LocalOcr::cancel);
    connect(engine, &LocalOcr::recognized, this, [=](const QString& result) {
        if (!result.isEmpty()) {
            text->setPlainText(result);
        }
        setBusy(false);
        status->setText(result.isEmpty()
                          ? tr("No text recognized.") +
                              (text->toPlainText().isEmpty()
                                 ? QString()
                                 : " " + tr("Previous text was kept."))
                          : tr("Review the text before copying."));
    });
    connect(engine, &LocalOcr::failed, this, [=](const QString& error) {
        setBusy(false);
        status->setText(error + (text->toPlainText().isEmpty()
                                   ? QString()
                                   : " " + tr("Previous text was kept.")));
    });
    connect(text, &QPlainTextEdit::textChanged, this, [=]() {
        copy->setEnabled(!engine->busy() && !text->toPlainText().isEmpty());
    });
    connect(copy, &QPushButton::clicked, text, [=]() {
        QApplication::clipboard()->setText(text->toPlainText());
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(this, &QDialog::finished, engine, &LocalOcr::cancel);
    const QScreen* display = screen();
    const QSize available =
      display ? display->availableGeometry().size() : QSize(800, 600);
    resize(qMin(680, available.width()), qMin(520, available.height()));
}

void OcrDialog::resizeEvent(QResizeEvent* event)
{
    const auto margins = layout()->contentsMargins();
    int requiredWidth = margins.left() + margins.right();
    for (auto* button : m_buttons->buttons()) {
        requiredWidth +=
          button->sizeHint().width() + qMax(0, layout()->spacing());
    }
    m_buttons->setOrientation(event->size().width() < qMax(560, requiredWidth)
                                ? Qt::Vertical
                                : Qt::Horizontal);
    QDialog::resizeEvent(event);
}
