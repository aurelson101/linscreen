// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "desktopinfo.h"

#include <QProcessEnvironment>

DesktopInfo::DesktopInfo()
{
    auto e = QProcessEnvironment::systemEnvironment();
    XDG_CURRENT_DESKTOP = e.value(QStringLiteral("XDG_CURRENT_DESKTOP"));
    XDG_SESSION_DESKTOP = e.value(QStringLiteral("XDG_SESSION_DESKTOP"));
    XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));
    WAYLAND_DISPLAY = e.value(QStringLiteral("WAYLAND_DISPLAY"));
    KDE_FULL_SESSION = e.value(QStringLiteral("KDE_FULL_SESSION"));
    GNOME_DESKTOP_SESSION_ID =
      e.value(QStringLiteral("GNOME_DESKTOP_SESSION_ID"));
    DESKTOP_SESSION = e.value(QStringLiteral("DESKTOP_SESSION"));
}

bool DesktopInfo::waylandDetected()
{
    return XDG_SESSION_TYPE == QLatin1String("wayland") ||
           WAYLAND_DISPLAY.contains(QLatin1String("wayland"),
                                    Qt::CaseInsensitive);
}

DesktopInfo::WM DesktopInfo::windowManager()
{
    DesktopInfo::WM res = DesktopInfo::OTHER;
    QStringList desktops =
      (XDG_CURRENT_DESKTOP + QChar(':') + XDG_SESSION_DESKTOP + QChar(':') +
       DESKTOP_SESSION)
        .split(QChar(':'), Qt::SkipEmptyParts);
    for (auto& desktop : desktops) {
        if (desktop.contains(QLatin1String("XFCE"), Qt::CaseInsensitive)) {
            return DesktopInfo::XFCE;
        }
        if (desktop.contains(QLatin1String("Cinnamon"),
                             Qt::CaseInsensitive)) {
            return DesktopInfo::CINNAMON;
        }
        if (desktop.contains(QLatin1String("MATE"), Qt::CaseInsensitive)) {
            return DesktopInfo::MATE;
        }
        if (desktop.contains(QLatin1String("LXQt"), Qt::CaseInsensitive)) {
            return DesktopInfo::LXQT;
        }
        if (desktop.contains(QLatin1String("GNOME"), Qt::CaseInsensitive)) {
            return DesktopInfo::GNOME;
        }
        if (desktop.contains(QLatin1String("qtile"), Qt::CaseInsensitive)) {
            return DesktopInfo::QTILE;
        }
        if (desktop.contains(QLatin1String("sway"), Qt::CaseInsensitive) ||
            desktop.contains(QLatin1String("river"), Qt::CaseInsensitive)) {
            return DesktopInfo::WLROOTS;
        }
        if (desktop.contains(QLatin1String("Hyprland"), Qt::CaseInsensitive)) {
            return DesktopInfo::HYPRLAND;
        }
        if (desktop.contains(QLatin1String("KDE"), Qt::CaseInsensitive) ||
            desktop.contains(QLatin1String("Plasma"), Qt::CaseInsensitive)) {
            return DesktopInfo::KDE;
        }
        if (desktop.contains(QLatin1String("cosmic"), Qt::CaseInsensitive)) {
            return DesktopInfo::COSMIC;
        }
    }

    if (!GNOME_DESKTOP_SESSION_ID.isEmpty()) {
        return DesktopInfo::GNOME;
    }

    if (!KDE_FULL_SESSION.isEmpty()) {
        return DesktopInfo::KDE;
    }

    return res;
}
