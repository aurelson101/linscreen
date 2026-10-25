// SPDX-License-Identifier: GPL-3.0-or-later
#include "utils/globalvalues.h"
#include "widgets/capture/buttonhandler.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPointer>
#include <cstdio>

// Only tool creation/animation is replaced: exercise the production layout,
// real Qt widgets, font metrics, geometry and size constraints.
CaptureToolButton::CaptureToolButton(CaptureTool::Type type, QWidget* parent)
  : CaptureButton(parent)
  , m_tool(nullptr)
  , m_buttonType(type)
  , m_emergeAnimation(nullptr)
{}

CaptureToolButton::~CaptureToolButton() = default;

void CaptureToolButton::animatedShow()
{
    resize(GlobalValues::buttonBaseSize(), GlobalValues::buttonBaseSize());
    show();
}

void CaptureToolButton::mousePressEvent(QMouseEvent* event)
{
    QPushButton::mousePressEvent(event);
}

static QVector<CaptureToolButton*> buttons(QWidget& parent, int count)
{
    QVector<CaptureToolButton*> result;
    for (int i = 0; i < count; ++i) {
        result << new CaptureToolButton(CaptureTool::TYPE_PENCIL, &parent);
    }
    return result;
}

static bool checkInside(ButtonHandler& handler,
                        const QVector<CaptureToolButton*>& tools,
                        const QRect& monitor)
{
    handler.show();
    for (int i = 0; i < tools.size(); ++i) {
        const QRect geometry = tools[i]->geometry();
        if (!monitor.contains(geometry) || geometry.isEmpty() ||
            !handler.contains(geometry.center())) {
            std::fprintf(
              stderr,
              "Button %d is outside monitor or missing from hit test\n",
              i);
            return false;
        }
        for (int j = 0; j < i; ++j) {
            if (geometry.intersects(tools[j]->geometry())) {
                std::fputs("Toolbar buttons overlap\n", stderr);
                return false;
            }
        }
    }
    return true;
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QWidget parent;
    parent.show();
    for (int fontSize : { 10, 22 }) {
        QFont font = app.font();
        font.setPointSize(fontSize);
        app.setFont(font);
        const auto tools = buttons(parent, 25);
        ButtonHandler handler(tools);

        // Full-monitor selections force the bounded interior-grid fallback.
        for (const QRect monitor : { QRect(0, 0, 320, 180),
                                     QRect(0, 0, 60, 320),
                                     QRect(500, 30, 320, 180) }) {
            handler.hide();
            handler.updateScreenRegions(monitor);
            handler.updatePosition(monitor);
            if (!handler.buttonsAreInside() ||
                !checkInside(handler, tools, monitor)) {
                return 1;
            }
        }

        const QString compactStyle = tools.first()->QWidget::styleSheet();
        for (int i = 0; i < 20; ++i) {
            handler.updatePosition(QRect(500, 30, 320, 180));
            if (tools.first()->QWidget::styleSheet() != compactStyle) {
                std::fputs("Toolbar style grows during relayout\n", stderr);
                return 6;
            }
        }

        handler.hide();
        const QRect offsetMonitor(500, 30, 320, 180);
        handler.updatePosition(QRect(-100, -100, 1, 1));
        if (!checkInside(handler, tools, offsetMonitor)) {
            return 7;
        }

        // An absent monitor must not leave stale buttons visible.
        handler.updateScreenRegions(QVector<QRect>{});
        handler.updatePosition(QRect(0, 0, 1, 1));
        handler.show();
        if (handler.isVisible()) {
            return 8;
        }

        // Physically impossible capacity still terminates and stays bounded.
        const QRect onePixel(0, 0, 1, 1);
        handler.updateScreenRegions(onePixel);
        handler.updatePosition(onePixel);
        for (const auto* button : tools) {
            if (!onePixel.contains(button->geometry())) {
                return 9;
            }
        }

        // Switching back to a spacious monitor restores the normal size.
        handler.hide();
        const QRect large(0, 0, 1920, 1080);
        handler.updateScreenRegions(large);
        handler.updatePosition(QRect(400, 300, 800, 450));
        if (!checkInside(handler, tools, large)) {
            return 2;
        }
        for (const auto* button : tools) {
            if (button->width() != GlobalValues::buttonBaseSize()) {
                return 3;
            }
        }

        // The middle button is outside the first/last buttons' bounding box.
        tools[0]->move(10, 10);
        tools[1]->move(800, 100);
        tools.last()->move(20, 200);
        if (!handler.contains(tools[1]->geometry().center()) ||
            handler.contains(QPoint(-200, -200))) {
            return 4;
        }

        QPointer<CaptureToolButton> old = tools.first();
        handler.setButtons({});
        if (old || handler.size() != 0 || handler.contains(QPoint(10, 10))) {
            return 5;
        }
    }
    return 0;
}
