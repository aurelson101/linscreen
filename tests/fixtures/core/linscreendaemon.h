// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QApplication>
#include <QClipboard>

// Isolate clipboard forwarding; exercise the production About window.
class LinScreenDaemon
{
public:
    static void copyToClipboard(const QString& text)
    {
        QApplication::clipboard()->setText(text);
    }
};
