// SPDX-License-Identifier: GPL-3.0-or-later
#include "utils/localocr.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QGuiApplication>
#include <QPainter>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <cstdio>

int main(int argc, char** argv)
{
    if (argc > 1 && QByteArray(argv[1]) == "stdin") {
        QCoreApplication app(argc, argv);
        QFile input;
        input.open(stdin, QIODevice::ReadOnly);
        const QImage image = QImage::fromData(input.readAll(), "PNG");
        if (app.arguments().mid(1) !=
              QStringList(
                { "stdin", "stdout", "-l", "fra+eng", "--psm", "6" }) ||
            image.isNull() || image.pixelColor(0, 0) != QColor(Qt::black)) {
            return 2;
        }
        const auto mode = qgetenv("LINSCREEN_OCR_TEST_MODE");
        if (mode == "hang") {
            QThread::sleep(10);
        }
        if (mode == "fail") {
            std::fputs("private/path/should/not/be/disclosed", stderr);
            return 1;
        }
        QFile output;
        output.open(stdout, QIODevice::WriteOnly);
        output.write(mode == "large" ? QByteArray(2 * 1024 * 1024, 'x')
                                     : QByteArray("LOCAL OCR 12345\n"));
        output.flush();
        return 0;
    }
    QGuiApplication app(argc, argv);
    LocalOcr engine;
    QImage image(1200, 140, QImage::Format_RGB32);
    image.fill(Qt::white);
    QString result, error;
    bool done = false;
    QEventLoop loop;
    QObject::connect(
      &engine, &LocalOcr::recognized, &loop, [&](const QString& text) {
          result = text;
          done = true;
          loop.quit();
      });
    QObject::connect(
      &engine, &LocalOcr::failed, &loop, [&](const QString& text) {
          error = text;
          done = true;
          loop.quit();
      });
    const auto run = [&](const QString& mode,
                         const QString& language,
                         int timeout,
                         bool cancel) {
        result.clear();
        error.clear();
        done = false;
        qputenv("LINSCREEN_OCR_TEST_MODE", mode.toUtf8());
        engine.start(image, language, timeout);
        if (cancel) {
            engine.cancel();
        }
        if (!done) {
            QTimer watchdog;
            watchdog.setSingleShot(true);
            QObject::connect(
              &watchdog, &QTimer::timeout, &loop, &QEventLoop::quit);
            watchdog.start(5000);
            loop.exec();
        }
        return done && !engine.busy();
    };
    if (app.arguments().contains("--real")) {
        QPainter painter(&image);
        painter.setPen(Qt::black);
        painter.setFont(QFont("DejaVu Sans", 32));
        painter.drawText(30, 90, "BONJOUR LOCAL OCR 12345");
        painter.end();
        return run("", "fra+eng", 4000, false) && error.isEmpty() &&
                   result.contains("12345") && result.contains("BONJOUR")
                 ? 0
                 : 1;
    }
    image.setPixelColor(0, 0, Qt::black);
    QTemporaryDir directory;
    if (!directory.isValid() || !QFile::copy(app.applicationFilePath(),
                                             directory.filePath("tesseract"))) {
        return 1;
    }
    qputenv("PATH", directory.path().toUtf8());
    if (!run("", "fra+eng", 2000, false) || result != "LOCAL OCR 12345" ||
        !error.isEmpty())
        return 2;
    if (!run("", "eng;echo injected", 2000, false) || error.isEmpty())
        return 3;
    if (!run("fail", "fra+eng", 2000, false) || error.isEmpty() ||
        error.contains("private"))
        return 4;
    if (!run("large", "fra+eng", 2000, false) || !error.contains("limit"))
        return 5;
    if (!run("hang", "fra+eng", 30, false) || !error.contains("timed out"))
        return 6;
    if (!run("hang", "fra+eng", 2000, true) || !error.contains("cancelled"))
        return 7;
    if (!run("", "fra+eng", 2000, false) || result != "LOCAL OCR 12345")
        return 8;
    qputenv("PATH", directory.filePath("missing").toUtf8());
    if (!run("", "fra+eng", 2000, false) ||
        !error.contains("Install Tesseract"))
        return 9;
    image = QImage();
    if (!run("", "fra+eng", 2000, false) || !error.contains("non-empty"))
        return 10;
    image = QImage(4001, 4000, QImage::Format_RGB32);
    if (!run("", "fra+eng", 2000, false) || !error.contains("16 million"))
        return 11;
    return 0;
}
