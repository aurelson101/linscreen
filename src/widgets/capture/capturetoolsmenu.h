// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QToolButton>

class QAction;

// Exposes every available capture action without changing toolbar preferences.
class CaptureToolsMenu : public QToolButton
{
    Q_OBJECT
public:
    explicit CaptureToolsMenu(QWidget* parent = nullptr);
    QAction* addTool(int toolId,
                     const QIcon& icon,
                     const QString& name,
                     const QString& description,
                     const QString& shortcut,
                     bool selectable);
    void setActiveTool(int toolId);
};
