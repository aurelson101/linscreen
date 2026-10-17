// SPDX-License-Identifier: GPL-3.0-or-later
#include "diagnostics.h"
#include "desktopinfo.h"
#include "globalvalues.h"

#include <QGuiApplication>
#include <QScreen>
#include <QSysInfo>
#if defined(Q_OS_LINUX)
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusVariant>
#endif

QString captureDiagnostics()
{
    QStringList lines;
    lines << GlobalValues::versionInfo();
    lines << QStringLiteral("OS: %1 %2")
               .arg(QSysInfo::productType(), QSysInfo::productVersion());
    const QStringList desktops = { "GNOME", "KDE",     "XFCE",    "Cinnamon",
                                   "MATE",  "LXQt",    "COSMIC",  "Other",
                                   "Qtile", "wlroots", "Hyprland" };
    DesktopInfo desktop;
    lines << QStringLiteral("Desktop: %1")
               .arg(desktops.value(desktop.windowManager(), "Other"));
    lines << QStringLiteral("Session: %1")
               .arg(desktop.waylandDetected() ? "Wayland" : "X11/other");
    int index = 0;
    for (QScreen* screen : QGuiApplication::screens()) {
        lines << QStringLiteral("Screen %1: %2x%3, scale %4")
                   .arg(++index)
                   .arg(screen->geometry().width())
                   .arg(screen->geometry().height())
                   .arg(screen->devicePixelRatio());
    }
#if defined(Q_OS_LINUX)
    auto bus = QDBusConnection::sessionBus();
    auto* interface = bus.interface();
    const QString portal = QStringLiteral("org.freedesktop.portal.Desktop");
    for (const QString& service :
         { portal,
           QStringLiteral("org.freedesktop.impl.portal.desktop.gnome"),
           QStringLiteral("org.freedesktop.impl.portal.desktop.kde"),
           QStringLiteral("org.freedesktop.impl.portal.desktop.gtk"),
           QStringLiteral("org.freedesktop.impl.portal.desktop.wlr"),
           QStringLiteral("org.freedesktop.impl.portal.desktop.xapp") }) {
        QDBusReply<bool> registered =
          interface ? interface->isServiceRegistered(service)
                    : QDBusReply<bool>();
        lines << QStringLiteral("%1: %2").arg(service,
                                              !registered.isValid() ? "unknown"
                                              : registered.value()
                                                ? "running"
                                                : "not running");
    }
    for (const QString& name :
         { QStringLiteral("Screenshot"), QStringLiteral("GlobalShortcuts") }) {
        auto message = QDBusMessage::createMethodCall(
          portal,
          QStringLiteral("/org/freedesktop/portal/desktop"),
          QStringLiteral("org.freedesktop.DBus.Properties"),
          QStringLiteral("Get"));
        message.setAutoStartService(false);
        message << QStringLiteral("org.freedesktop.portal.") + name
                << QStringLiteral("version");
        const auto reply = bus.call(message, QDBus::Block, 500);
        QString status = QStringLiteral("unavailable or unknown");
        if (reply.type() == QDBusMessage::ReplyMessage &&
            reply.arguments().size() == 1) {
            const auto value =
              qvariant_cast<QDBusVariant>(reply.arguments().first()).variant();
            bool ok = false;
            const uint version = value.toUInt(&ok);
            if (ok) {
                status = QStringLiteral("version %1").arg(version);
            }
        }
        lines << QStringLiteral("%1 portal: %2").arg(name, status);
    }
#else
    lines << QStringLiteral("Desktop portals: not applicable on this OS");
#endif
    lines << QStringLiteral("Privacy: no host/user names, paths, screen names, "
                            "environment dumps or screenshots included.");
    return lines.join('\n');
}
