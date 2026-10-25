// SPDX-License-Identifier: GPL-3.0-or-later
#include "widgets/capture/capturetoolsmenu.h"

#include <QAction>
#include <QApplication>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMenu>
#include <QScreen>
#include <QTimer>
#include <cstdio>

namespace {
bool check(bool condition, const char* message)
{
    if (!condition) {
        std::fprintf(stderr, "Capture tools menu: %s\n", message);
    }
    return condition;
}
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QWidget host;
    host.resize(390, 300);
    CaptureToolsMenu tools(&host);
    auto* pen = tools.addTool(0, QIcon(), "Pencil", "Draw freely", "P", true);
    auto* redact = tools.addTool(
      25, QIcon(), "Redact", "Hide sensitive content", "Shift+R", true);
    auto* ocr = tools.addTool(
      26, QIcon(), "OCR", "Recognize text locally", "Shift+T", false);
    auto* copy = tools.addTool(
      10, QIcon(), "Copy", "Copy selected image", "Ctrl+C", false);
    auto* undo =
      tools.addTool(9, QIcon(), "Undo", "Undo last edit", "Ctrl+Z", false);

    if (!check(tools.objectName() == "captureToolsMenu",
               "stable object name missing") ||
        !check(!tools.accessibleName().isEmpty() && !tools.toolTip().isEmpty(),
               "accessible name or help missing") ||
        !check(tools.focusPolicy() == Qt::StrongFocus &&
                 tools.popupMode() == QToolButton::InstantPopup,
               "menu must be keyboard reachable") ||
        !check(tools.menu()->actions().size() == 5 &&
                 tools.menu()->actions().contains(redact) &&
                 tools.menu()->actions().contains(ocr),
               "all supplied tools must be discoverable") ||
        !check(ocr->data().toInt() == 26 && copy->data().toInt() == 10,
               "tool identity lost") ||
        !check(ocr->text() == "OCR\tShift+T" &&
                 ocr->toolTip() == "Recognize text locally" &&
                 ocr->statusTip() == ocr->toolTip(),
               "tool description or shortcut display lost")) {
        return 1;
    }
    for (auto* action : tools.menu()->actions()) {
        if (!check(action->shortcuts().isEmpty(),
                   "displaying a shortcut must not register it twice")) {
            return 2;
        }
    }
    tools.setActiveTool(25);
    if (!check(redact->isChecked() && !pen->isChecked() &&
                 !ocr->isCheckable() && !copy->isChecked(),
               "only the active selectable tool should be checked")) {
        return 3;
    }
    tools.setActiveTool(-1);
    if (!check(!redact->isChecked() && !pen->isChecked(),
               "clearing active tool must clear selection")) {
        return 4;
    }
    undo->setEnabled(false);
    int undoTriggers = 0;
    QObject::connect(undo, &QAction::triggered, [&]() { ++undoTriggers; });
    undo->trigger();
    if (!check(undoTriggers == 0, "disabled commands must not activate")) {
        return 5;
    }

    int copyTriggers = 0;
    QObject::connect(copy, &QAction::triggered, [&]() { ++copyTriggers; });
    bool popupSeen = false;
    bool watchdogExpired = false;
    QTimer watchdog;
    watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, [&]() {
        watchdogExpired = true;
        tools.menu()->close();
    });
    host.show();
    tools.show();
    tools.adjustSize();
    app.processEvents();
    if (!check(tools.sizeHint().width() <= 320 &&
                 tools.sizeHint().height() <= 96,
               "menu button must fit a compact capture")) {
        return 6;
    }
    QTimer::singleShot(0, [&]() {
        popupSeen = tools.menu()->isVisible();
        tools.menu()->setActiveAction(copy);
        QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QApplication::sendEvent(tools.menu(), &enter);
    });
    watchdog.start(1000);
    tools.showMenu();
    watchdog.stop();
    if (!check(popupSeen && !watchdogExpired && copyTriggers == 1,
               "keyboard activation must trigger the selected command once")) {
        return 7;
    }

    CaptureToolsMenu manyTools(&host);
    for (int id = 0; id < 60; ++id) {
        manyTools.addTool(id,
                          QIcon(),
                          QString("Tool %1").arg(id),
                          "Available capture tool",
                          QString(),
                          true);
    }
    manyTools.menu()->popup(host.mapToGlobal(QPoint(0, 0)));
    app.processEvents();
    const auto* screen = QGuiApplication::primaryScreen();
    const bool fitsScreen = screen && manyTools.menu()->height() <=
                                        screen->availableGeometry().height();
    manyTools.menu()->close();
    if (!check(manyTools.menu()->actions().size() == 60 && fitsScreen,
               "many tools must remain present in a screen-bounded menu")) {
        return 8;
    }
    return 0;
}
