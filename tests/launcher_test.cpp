// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/linscreen.h"
#include "widgets/capturelauncher.h"

#include <QApplication>
#include <QComboBox>
#include <QEventLoop>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QSpinBox>

QRect getLastRegion()
{
    return QRect(0, 0, 0, 0);
}

int savedCaptures = 0;

bool saveToFilesystemGUI(const QPixmap&)
{
    ++savedCaptures;
    return true;
}

void waitForTimers()
{
    QEventLoop loop;
    QTimer::singleShot(1300, &loop, &QEventLoop::quit);
    loop.exec();
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QPointer<CaptureLauncher> launcher = new CaptureLauncher;
    auto* delay = launcher->findChild<QSpinBox*>("delayTime");
    auto* start = launcher->findChild<QPushButton*>("launchButton");
    auto* cancel = launcher->findChild<QPushButton*>("cancelButton");
    if (!delay || !start || !cancel) {
        return 1;
    }
    auto* mode = launcher->findChild<QComboBox*>("captureType");
    auto* monitor = launcher->findChild<QComboBox*>("monitorSelection");
    auto* regionWidth = launcher->findChild<QLineEdit*>("screenshotWidth");
    if (!mode || !monitor || !regionWidth || regionWidth->isHidden() ||
        mode->currentData().toInt() != CaptureRequest::GRAPHICAL_MODE) {
        return 8;
    }
    delay->setValue(1);
    start->click();
    if (start->isEnabled() || cancel->isHidden()) {
        return 2;
    }
    cancel->click();
    waitForTimers();
    if (LinScreen::instance()->requests || !start->isEnabled() ||
        !cancel->isHidden()) {
        return 3;
    }
    start->click();
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(launcher, &escape);
    waitForTimers();
    if (!launcher || LinScreen::instance()->requests || !start->isEnabled()) {
        return 4;
    }
    start->click();
    launcher->close();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    waitForTimers();
    if (launcher || LinScreen::instance()->requests) {
        return 5;
    }
    launcher = new CaptureLauncher;
    delay = launcher->findChild<QSpinBox*>("delayTime");
    start = launcher->findChild<QPushButton*>("launchButton");
    auto* width = launcher->findChild<QLineEdit*>("screenshotWidth");
    width->clear();
    start->click();
    waitForTimers();
    if (LinScreen::instance()->requests || !start->isEnabled()) {
        return 6;
    }
    width->setText("0");
    delay->setValue(1);
    bool hiddenBeforeDispatch = false;
    QObject::connect(LinScreen::instance(),
                     &LinScreen::requestStarted,
                     launcher,
                     [&]() { hiddenBeforeDispatch = launcher->isHidden(); });
    start->click();
    waitForTimers();
    if (LinScreen::instance()->requests != 1 ||
        LinScreen::instance()->lastDelay != 600 || !hiddenBeforeDispatch ||
        !start->isEnabled()) {
        return 7;
    }
    mode = launcher->findChild<QComboBox*>("captureType");
    monitor = launcher->findChild<QComboBox*>("monitorSelection");
    auto* backend = LinScreen::instance();
    backend->captureSucceeds = true;
    delay->setValue(0);
    mode->setCurrentIndex(mode->findData(CaptureRequest::FULLSCREEN_MODE));
    if (!width->isHidden() || !monitor->isHidden()) {
        return 9;
    }
    start->click();
    waitForTimers();
    if (backend->lastMode != CaptureRequest::FULLSCREEN_MODE ||
        backend->lastData.isValid() || savedCaptures != 1 ||
        !start->isEnabled()) {
        return 10;
    }
#ifndef Q_OS_MACOS
    if (backend->lastHasSelectedMonitor) {
        return 11;
    }
    const int screenMode = mode->findData(CaptureRequest::SCREEN_MODE);
    if (screenMode < 0) {
        return 12;
    }
    mode->setCurrentIndex(screenMode);
    launcher->show();
    if (!width->isHidden() || monitor->isHidden()) {
        return 13;
    }
    const int monitorIndex = monitor->currentData().toInt();
    start->click();
    waitForTimers();
    if (backend->lastMode != CaptureRequest::SCREEN_MODE ||
        backend->lastData.toInt() != monitorIndex ||
        !backend->lastHasSelectedMonitor ||
        backend->lastSelectedMonitor != monitorIndex || savedCaptures != 2) {
        return 14;
    }
    monitor->clear();
    const int requestsBeforeEmptyMonitor = backend->requests;
    start->click();
    if (backend->requests != requestsBeforeEmptyMonitor ||
        !start->isEnabled()) {
        return 15;
    }
#else
    if (mode->findData(CaptureRequest::SCREEN_MODE) >= 0) {
        return 12;
    }
#endif
    mode->setCurrentIndex(mode->findData(CaptureRequest::GRAPHICAL_MODE));
    if (width->isHidden()) {
        return 16;
    }
    delete launcher;
    return 0;
}
