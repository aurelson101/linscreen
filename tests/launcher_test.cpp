// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/linscreen.h"
#include "widgets/capturelauncher.h"

#include <QApplication>
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

bool saveToFilesystemGUI(const QPixmap&)
{
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
    delete launcher;
    return 0;
}
