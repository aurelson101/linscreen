// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021-2022 Jeremy Borgman & Contributors

#include "infowindow.h"
#include "./ui_infowindow.h"
#include "core/linscreendaemon.h"
#include "core/qguiappcurrentscreen.h"
#include "utils/diagnostics.h"
#include "utils/globalvalues.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>

InfoWindow::InfoWindow(QWidget* parent)
  : QWidget(parent)
  , ui(new Ui::InfoWindow)
{
    ui->setupUi(this);
    ui->verticalLayout->setAlignment(Qt::AlignTop);
    setAttribute(Qt::WA_DeleteOnClose);

    ui->IconSVG->setPixmap(QIcon(GlobalValues::iconPath())
                             .pixmap(QSize(64, 64), devicePixelRatioF()));
    ui->IconSVG->setAccessibleName(QStringLiteral("LinScreen"));
    for (auto* label : { ui->VersionDetails, ui->OperatingSystemDetails }) {
        label->setTextFormat(Qt::PlainText);
        label->setWordWrap(true);
        label->setMinimumWidth(0);
        label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    }
    ui->VersionDetails->setText(GlobalValues::versionInfo());
    ui->OperatingSystemDetails->setText(generateKernelString());

    connect(
      ui->CopyInfoButton, &QPushButton::clicked, this, &InfoWindow::copyInfo);
    auto* diagnosticsButton = new QPushButton(tr("Capture diagnostics"), this);
    diagnosticsButton->setObjectName("diagnosticsButton");
    ui->gridLayout->addWidget(diagnosticsButton, 2, 0);
    connect(diagnosticsButton,
            &QPushButton::clicked,
            this,
            &InfoWindow::showDiagnostics);

    QScreen* screen = QGuiAppCurrentScreen().currentScreen();
    const QSize available =
      screen ? screen->availableGeometry().size() : QSize(800, 600);
    resize(qMin(480, qMax(1, available.width() - 32)),
           qMin(520, qMax(1, available.height() - 64)));
    show();
    // Call show() first, otherwise the correct geometry cannot be fetched for
    // centering the window on the screen
    QRect position = frameGeometry();
    if (screen != nullptr) {
        position.moveCenter(screen->availableGeometry().center());
        move(position.topLeft());
    }
}

InfoWindow::~InfoWindow()
{
    delete ui;
}

void InfoWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
    } else {
        QWidget::keyPressEvent(event);
    }
}

QString generateKernelString()
{
    QString kernelVersion =
      QSysInfo::kernelType() + ": " + QSysInfo::kernelVersion() + "\n" +
      QSysInfo::productType() + ": " + QSysInfo::productVersion();
    return kernelVersion;
}

void InfoWindow::copyInfo()
{
    LinScreenDaemon::copyToClipboard(GlobalValues::versionInfo() + "\n" +
                                     generateKernelString());
}

void InfoWindow::showDiagnostics()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Capture diagnostics"));
    auto* layout = new QVBoxLayout(&dialog);
    auto* notice = new QLabel(
      tr("Local technical report: no usernames, hostnames, paths or images. "
         "Review it before copying. Nothing is sent automatically."),
      &dialog);
    notice->setWordWrap(true);
    layout->addWidget(notice);
    auto* report = new QPlainTextEdit(&dialog);
    report->setReadOnly(true);
    report->setAccessibleName(tr("Anonymized capture diagnostics"));
    report->setPlainText(captureDiagnostics());
    layout->addWidget(report);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    auto* refresh =
      buttons->addButton(tr("Refresh"), QDialogButtonBox::ActionRole);
    auto* copy =
      buttons->addButton(tr("Copy report"), QDialogButtonBox::ActionRole);
    connect(refresh, &QPushButton::clicked, report, [report]() {
        report->setPlainText(captureDiagnostics());
    });
    connect(copy, &QPushButton::clicked, report, [report]() {
        LinScreenDaemon::copyToClipboard(report->toPlainText());
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    const QScreen* screen = QGuiAppCurrentScreen().currentScreen();
    const QSize available =
      screen ? screen->availableGeometry().size() : QSize(800, 600);
    dialog.resize(qMin(680, available.width()), qMin(520, available.height()));
    dialog.exec();
}
