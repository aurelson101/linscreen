// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "linscreendbusadapter.h"
#include "core/linscreen.h"
#include "core/linscreendaemon.h"

#include <QDebug>

namespace {
constexpr qsizetype maxImageMessageBytes = 64 * 1024 * 1024;
constexpr qsizetype maxClipboardTextCharacters = 1024 * 1024;
constexpr qsizetype maxNotificationCharacters = 16 * 1024;
}

LinScreenDBusAdapter::LinScreenDBusAdapter(QObject* parent)
  : QDBusAbstractAdaptor(parent)
{}

LinScreenDBusAdapter::~LinScreenDBusAdapter() = default;

void LinScreenDBusAdapter::captureScreen()
{
    LinScreen::instance()->gui(CaptureRequest(CaptureRequest::GRAPHICAL_MODE));
}

void LinScreenDBusAdapter::attachScreenshotToClipboard(const QByteArray& data)
{
    if (data.size() > maxImageMessageBytes) {
        qWarning() << "Rejected oversized D-Bus screenshot payload";
        return;
    }
    LinScreenDaemon::instance()->attachScreenshotToClipboard(data);
}

void LinScreenDBusAdapter::attachTextToClipboard(const QString& text,
                                                 const QString& notification)
{
    if (text.size() > maxClipboardTextCharacters ||
        notification.size() > maxNotificationCharacters) {
        qWarning() << "Rejected oversized D-Bus clipboard payload";
        return;
    }
    LinScreenDaemon::instance()->attachTextToClipboard(text, notification);
}

void LinScreenDBusAdapter::attachPin(const QByteArray& data)
{
    if (data.size() > maxImageMessageBytes) {
        qWarning() << "Rejected oversized D-Bus pin payload";
        return;
    }
    LinScreenDaemon::instance()->attachPin(data);
}
