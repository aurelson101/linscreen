// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "linscreendbusadapter.h"
#include "core/linscreen.h"
#include "core/linscreendaemon.h"

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
    LinScreenDaemon::instance()->attachScreenshotToClipboard(data);
}

void LinScreenDBusAdapter::attachTextToClipboard(const QString& text,
                                                 const QString& notification)
{
    LinScreenDaemon::instance()->attachTextToClipboard(text, notification);
}

void LinScreenDBusAdapter::attachPin(const QByteArray& data)
{
    LinScreenDaemon::instance()->attachPin(data);
}
