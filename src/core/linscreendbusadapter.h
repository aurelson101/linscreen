// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#pragma once

#include <QtDBus/QDBusAbstractAdaptor>

class LinScreenDBusAdapter : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.linscreen.LinScreen")

public:
    explicit LinScreenDBusAdapter(QObject* parent = nullptr);
    virtual ~LinScreenDBusAdapter();

public slots:
    Q_NOREPLY void captureScreen();
    Q_NOREPLY void attachScreenshotToClipboard(const QByteArray& data);
    Q_NOREPLY void attachTextToClipboard(const QString& text,
                                         const QString& notification);
    Q_NOREPLY void attachPin(const QByteArray& data);
};
