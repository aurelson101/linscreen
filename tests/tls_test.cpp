// SPDX-License-Identifier: GPL-3.0-or-later
#include <QCoreApplication>
#include <QSslSocket>
#include <cstdio>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    if (!QSslSocket::supportsSsl()) {
        std::fprintf(stderr,
                     "TLS unavailable: backend=%s build=%s runtime=%s\n",
                     qPrintable(QSslSocket::activeBackend()),
                     qPrintable(QSslSocket::sslLibraryBuildVersionString()),
                     qPrintable(QSslSocket::sslLibraryVersionString()));
        return 1;
    }
    return 0;
}
