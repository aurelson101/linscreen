// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2018 Alejandro Sirgo Rica & Contributors

#include "capturelauncher.h"
#include "./ui_capturelauncher.h"
#include "config/cacheutils.h"
#include "core/linscreen.h"
#include "core/qguiappcurrentscreen.h"
#include "utils/globalvalues.h"
#include "utils/screenshotsaver.h"

#include <QCloseEvent>
#include <QGridLayout>
#include <QGuiApplication>
#include <QIntValidator>
#include <QLineEdit>
#include <QLocale>
#include <QMimeData>
#include <QScreen>
#include <QSizePolicy>

// https://github.com/KDE/spectacle/blob/941c1a517be82bed25d1254ebd735c29b0d2951c/src/Gui/KSWidget.cpp
// https://github.com/KDE/spectacle/blob/941c1a517be82bed25d1254ebd735c29b0d2951c/src/Gui/KSMainWindow.cpp

CaptureLauncher::CaptureLauncher(QDialog* parent)
  : QDialog(parent)
  , ui(new Ui::CaptureLauncher)
  , m_countdownTimer(new QTimer(this))
  , m_captureTimer(new QTimer(this))
  , m_remainingSeconds(0)
{
    qApp->installEventFilter(this); // see eventFilter()
    ui->setupUi(this);
    m_captureTimer->setSingleShot(true);
    ui->cancelButton->hide();
    connect(ui->cancelButton,
            &QPushButton::clicked,
            this,
            &CaptureLauncher::cancelPendingCapture);
    ui->gridLayout_3->setColumnStretch(1, 1);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowIcon(QIcon(GlobalValues::iconPath()));

    ui->captureType->insertItem(
      1, tr("Rectangular Region"), CaptureRequest::GRAPHICAL_MODE);

#if defined(Q_OS_MACOS)
    // Following to MacOS philosophy (one application cannot be displayed on
    // more than one display)
    ui->captureType->insertItem(
      2, tr("Full Screen (Current Display)"), CaptureRequest::FULLSCREEN_MODE);
#else
    ui->captureType->insertItem(
      2, tr("Full Screen"), CaptureRequest::FULLSCREEN_MODE);
    const QList<QScreen*> screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        QScreen* screen = screens[i];
        QRect geom = screen->geometry();
        QString monitorText = tr("Monitor %1: %2 (%3x%4)")
                                .arg(i + 1)
                                .arg(screen->name())
                                .arg(geom.width())
                                .arg(geom.height());
        ui->monitorSelection->addItem(monitorText, i);
    }
    // Select current screen by default
    QScreen* currentScreen = QGuiAppCurrentScreen().currentScreen();
    int currentIndex = screens.indexOf(currentScreen);
    if (currentIndex >= 0) {
        ui->monitorSelection->setCurrentIndex(currentIndex);
    }
#endif

#ifdef Q_OS_MACOS
    ui->monitorLabel->setVisible(false);
    ui->monitorSelection->setVisible(false);
#else
    if (screens.size() <= 1) {
        ui->monitorLabel->setVisible(false);
        ui->monitorSelection->setVisible(false);
    }
#endif

    ui->delayTime->setSpecialValueText(tr("No Delay"));
    ui->captureType->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->monitorSelection->setSizePolicy(QSizePolicy::Expanding,
                                        QSizePolicy::Fixed);
    ui->launchButton->setFocus();
    ui->launchButton->setDefault(true);
    setSizeGripEnabled(true);

    ui->countdownLabel->setVisible(false);
    ui->countdownLabel->setStyleSheet(
      "QLabel { font-size: 24px; font-weight: bold;}");
    ui->countdownLabel->setAlignment(Qt::AlignCenter);

    // Connect countdown timer
    connect(m_countdownTimer,
            &QTimer::timeout,
            this,
            &CaptureLauncher::updateCountdown);

    // Function to add or remove plural to seconds
    connect(ui->delayTime,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            [this](int val) {
                QString suffix = val == 1 ? tr(" second") : tr(" seconds");
                this->ui->delayTime->setSuffix(suffix);
            });

    connect(ui->launchButton,
            &QPushButton::clicked,
            this,
            &CaptureLauncher::startCapture);

    connect(ui->captureType,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this]() {
                auto mode = static_cast<CaptureRequest::CaptureMode>(
                  ui->captureType->currentData().toInt());
                if (mode == CaptureRequest::CaptureMode::GRAPHICAL_MODE) {
                    ui->sizeLabel->show();
                    ui->screenshotX->show();
                    ui->screenshotY->show();
                    ui->screenshotWidth->show();
                    ui->screenshotHeight->show();
                } else {
                    ui->sizeLabel->hide();
                    ui->screenshotX->hide();
                    ui->screenshotY->hide();
                    ui->screenshotWidth->hide();
                    ui->screenshotHeight->hide();
                }
            });

    auto lastRegion = getLastRegion();
    ui->screenshotX->setText(QString::number(lastRegion.x()));
    ui->screenshotY->setText(QString::number(lastRegion.y()));
    ui->screenshotWidth->setText(QString::number(lastRegion.width()));
    ui->screenshotHeight->setText(QString::number(lastRegion.height()));

    QLocale coordinateLocale = QLocale::c();
    coordinateLocale.setNumberOptions(QLocale::RejectGroupSeparator);
    for (QLineEdit* field : { ui->screenshotWidth,
                              ui->screenshotHeight,
                              ui->screenshotX,
                              ui->screenshotY }) {
        const int minimum =
          (field == ui->screenshotX || field == ui->screenshotY) ? -100000 : 0;
        auto* validator = new QIntValidator(minimum, 100000, field);
        validator->setLocale(coordinateLocale);
        field->setValidator(validator);
        field->setMinimumWidth(56);
        field->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        field->setClearButtonEnabled(true);
    }

    adjustSize();
    QScreen* screen = QGuiAppCurrentScreen().currentScreen();
    if (screen != nullptr) {
        const int availableWidth =
          qMax(320, screen->availableGeometry().width() - 32);
        resize(qMin(qMax(sizeHint().width(), 440), availableWidth), height());
    }
    show();
    // Call show() first, otherwise the correct geometry cannot be fetched
    // for centering the window on the screen
    QRect position = frameGeometry();
    if (screen != nullptr) {
        position.moveCenter(screen->availableGeometry().center());
        move(position.topLeft());
    }
}

// HACK:
// https://github.com/KDE/spectacle/blob/fa1e780b8bf3df3ac36c410b9ece4ace041f401b/src/Gui/KSMainWindow.cpp#L70
void CaptureLauncher::startCapture()
{
    if (!ui->launchButton->isEnabled()) {
        return;
    }

    auto mode = static_cast<CaptureRequest::CaptureMode>(
      ui->captureType->currentData().toInt());
    if (mode == CaptureRequest::GRAPHICAL_MODE) {
        for (QLineEdit* field : { ui->screenshotWidth,
                                  ui->screenshotHeight,
                                  ui->screenshotX,
                                  ui->screenshotY }) {
            if (!field->hasAcceptableInput()) {
                field->setFocus();
                field->selectAll();
                return;
            }
        }
    }
    setCaptureControlsEnabled(false);

    int delaySeconds = ui->delayTime->value();

    if (delaySeconds > 0) {
        m_remainingSeconds = delaySeconds;
        ui->countdownLabel->setVisible(true);
        ui->countdownLabel->setText(QString::number(m_remainingSeconds));
        m_countdownTimer->start(1000);
    } else {
        hide();
    }

    auto const additionalDelayToHideUI = 600;
    auto const secondsToMilliseconds = 1000;
    CaptureRequest req(mode, 0);

    if (mode == CaptureRequest::CaptureMode::GRAPHICAL_MODE) {
        req.setInitialSelection(QRect(ui->screenshotX->text().toInt(),
                                      ui->screenshotY->text().toInt(),
                                      ui->screenshotWidth->text().toInt(),
                                      ui->screenshotHeight->text().toInt()));
    }

#ifndef Q_OS_MACOS
    int selectedMonitor = ui->monitorSelection->currentData().toInt();
    req.setSelectedMonitor(selectedMonitor);
#else
    req.setSelectedMonitor(-1);
#endif

    connect(
      m_captureTimer,
      &QTimer::timeout,
      this,
      [this, req]() {
          ui->cancelButton->hide();
          connectCaptureSlots();
          LinScreen::instance()->requestCapture(req);
      },
      Qt::SingleShotConnection);
    ui->cancelButton->show();
    m_captureTimer->start(additionalDelayToHideUI +
                          delaySeconds * secondsToMilliseconds);
}

void CaptureLauncher::cancelPendingCapture()
{
    if (!m_captureTimer->isActive()) {
        return;
    }
    m_captureTimer->stop();
    disconnect(m_captureTimer, nullptr, this, nullptr);
    m_countdownTimer->stop();
    m_remainingSeconds = 0;
    ui->countdownLabel->hide();
    ui->cancelButton->hide();
    setCaptureControlsEnabled(true);
    show();
    ui->launchButton->setFocus();
}

void CaptureLauncher::reject()
{
    if (m_captureTimer->isActive()) {
        cancelPendingCapture();
    } else {
        QDialog::reject();
    }
}

void CaptureLauncher::closeEvent(QCloseEvent* event)
{
    cancelPendingCapture();
    QDialog::closeEvent(event);
}

void CaptureLauncher::setCaptureControlsEnabled(bool enabled)
{
    ui->launchButton->setEnabled(enabled);
    ui->captureType->setEnabled(enabled);
    ui->monitorSelection->setEnabled(enabled);
    ui->delayTime->setEnabled(enabled);
    ui->screenshotWidth->setEnabled(enabled);
    ui->screenshotHeight->setEnabled(enabled);
    ui->screenshotX->setEnabled(enabled);
    ui->screenshotY->setEnabled(enabled);
}

void CaptureLauncher::connectCaptureSlots() const
{
    connect(LinScreen::instance(),
            &LinScreen::captureTaken,
            this,
            &CaptureLauncher::onCaptureTaken);
    connect(LinScreen::instance(),
            &LinScreen::captureFailed,
            this,
            &CaptureLauncher::onCaptureFailed);
}

void CaptureLauncher::disconnectCaptureSlots() const
{
    // Hack for MacOS
    // for some strange reasons MacOS sends multiple "captureTaken" signals
    // (random number, usually from 1 up to 20).
    // So now it enables signal on "Capture new screenshot" button and disables
    // on first success of fail.
    disconnect(LinScreen::instance(),
               &LinScreen::captureTaken,
               this,
               &CaptureLauncher::onCaptureTaken);
    disconnect(LinScreen::instance(),
               &LinScreen::captureFailed,
               this,
               &CaptureLauncher::onCaptureFailed);
}

void CaptureLauncher::onCaptureTaken(QPixmap const& screenshot)
{
    // MacOS specific, more details in the function disconnectCaptureSlots()
    disconnectCaptureSlots();
    m_countdownTimer->stop();
    ui->countdownLabel->setVisible(false);

    auto mode = static_cast<CaptureRequest::CaptureMode>(
      ui->captureType->currentData().toInt());

    if (mode == CaptureRequest::FULLSCREEN_MODE) {
        saveToFilesystemGUI(screenshot);
    }
    setCaptureControlsEnabled(true);
}

void CaptureLauncher::onCaptureFailed()
{
    // MacOS specific, more details in the function disconnectCaptureSlots()
    disconnectCaptureSlots();
    m_countdownTimer->stop();
    ui->countdownLabel->setVisible(false);
    show();
    setCaptureControlsEnabled(true);
}

void CaptureLauncher::updateCountdown()
{
    m_remainingSeconds--;

    if (m_remainingSeconds > 0) {
        ui->countdownLabel->setText(QString::number(m_remainingSeconds));
    } else {
        m_countdownTimer->stop();
        ui->countdownLabel->setVisible(false);
        hide();
    }
}

CaptureLauncher::~CaptureLauncher()
{
    delete ui;
}
