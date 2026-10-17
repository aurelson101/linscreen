// SPDX-License-Identifier: GPL-3.0-or-later
#include "utils/diagnostics.h"

#include <QApplication>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    const QByteArray secret("PRIVATE_DIAGNOSTIC_TEST_7d419");
    qputenv("XDG_CURRENT_DESKTOP", "GNOME:" + secret);
    qputenv("DESKTOP_SESSION", secret);
    qputenv("XDG_SESSION_DESKTOP", secret);
    qputenv("XDG_RUNTIME_DIR", secret);
    qputenv("API_TOKEN", secret);
    const QString report = captureDiagnostics();
    if (report.contains(QString::fromLatin1(secret)) ||
        !report.contains("Desktop: GNOME") || !report.contains("Privacy:") ||
        !report.contains("Screen 1:")) {
        return 1;
    }
    return 0;
}
