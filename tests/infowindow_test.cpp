// SPDX-License-Identifier: GPL-3.0-or-later
#include "widgets/infowindow.h"
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyleFactory>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    for (const auto& style : QStyleFactory::keys()) {
        app.setStyle(style);
        QFont largeFont = app.font();
        largeFont.setPointSize(18);
        app.setFont(largeFont);
        QPointer<InfoWindow> window = new InfoWindow;
        auto* version = window->findChild<QLabel*>("VersionDetails");
        auto* icon = window->findChild<QLabel*>("IconSVG");
        auto* system = window->findChild<QLabel*>("OperatingSystemDetails");
        auto* scroll = window->findChild<QScrollArea*>("infoScrollArea");
        auto* copy = window->findChild<QPushButton*>("CopyInfoButton");
        auto* diagnostics =
          window->findChild<QPushButton*>("diagnosticsButton");
        if (!version || !system || !scroll || !copy || !diagnostics || !icon)
            return 1;
        if (icon->pixmap().isNull() ||
            icon->pixmap().deviceIndependentSize().width() > 64)
            return 7;
        window->resize(320, 260);
        app.processEvents();
        if (!window->grab().save("about-" + style + "-compact.png"))
            return 8;
        const QString originalVersion = version->text();
        const QString originalSystem = system->text();
        version->setText(QString(240, 'v'));
        system->setText(QString(240, 's'));
        window->resize(320, 260);
        app.processEvents();
        if (window->width() > 320 || window->height() > 260 ||
            !version->wordWrap() || !system->wordWrap() ||
            scroll->horizontalScrollBar()->maximum() != 0 ||
            scroll->verticalScrollBar()->maximum() <= 0)
            return 2;
        for (auto* button : { copy, diagnostics }) {
            if (!window->rect().contains(
                  QRect(button->mapTo(window, QPoint()), button->size())) ||
                scroll->isAncestorOf(button))
                return 3;
        }
        copy->click();
        if (!app.clipboard()->text().contains("LinScreen"))
            return 4;
        window->resize(640, 520);
        version->setText(originalVersion);
        system->setText(originalSystem);
        app.processEvents();
        if (window->width() != 640 || window->height() != 520)
            return 5;
        if (!window->grab().save("about-" + style + "-expanded.png"))
            return 9;
        QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        app.sendEvent(window, &escape);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        if (window)
            return 6;
    }
    return 0;
}
