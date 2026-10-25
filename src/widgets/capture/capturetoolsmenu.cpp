// SPDX-License-Identifier: GPL-3.0-or-later
#include "capturetoolsmenu.h"

#include <QAction>
#include <QMenu>

CaptureToolsMenu::CaptureToolsMenu(QWidget* parent)
  : QToolButton(parent)
{
    setObjectName(QStringLiteral("captureToolsMenu"));
    setText(tr("All tools"));
    setAccessibleName(tr("All capture tools"));
    setToolTip(tr("Show all available tools, including hidden toolbar tools"));
    setPopupMode(QToolButton::InstantPopup);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::ArrowCursor);
    setMenu(new QMenu(this));
}

QAction* CaptureToolsMenu::addTool(int toolId,
                                   const QIcon& icon,
                                   const QString& name,
                                   const QString& description,
                                   const QString& shortcut,
                                   bool selectable)
{
    auto* action = menu()->addAction(
      icon, shortcut.isEmpty() ? name : name + QLatin1Char('\t') + shortcut);
    action->setData(toolId);
    action->setToolTip(description);
    action->setStatusTip(description);
    action->setCheckable(selectable);
    // Existing capture shortcuts own activation. Displaying the shortcut in
    // the menu must not register a second, ambiguous shortcut.
    return action;
}

void CaptureToolsMenu::setActiveTool(int toolId)
{
    for (auto* action : menu()->actions()) {
        if (action->isCheckable()) {
            action->setChecked(action->data().toInt() == toolId);
        }
    }
}
