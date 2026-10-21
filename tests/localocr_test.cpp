// SPDX-License-Identifier: GPL-3.0-or-later
#include "utils/localocr.h"
#include "widgets/ocrdialog.h"
#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QEventLoop>
#include <QFile>
#include <QLabel>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
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
        if (mode == "empty") {
            return 0;
        }
        QFile output;
        output.open(stdout, QIODevice::WriteOnly);
        output.write(mode == "large" ? QByteArray(2 * 1024 * 1024, 'x')
                                     : QByteArray("LOCAL OCR 12345\n"));
        output.flush();
        return 0;
    }
    QApplication app(argc, argv);
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
        const bool success = run("", "fra+eng", 4000, false) &&
                             error.isEmpty() && result.contains("12345") &&
                             result.contains("BONJOUR");
        if (!success) {
            std::fprintf(stderr,
                         "Synthetic OCR failed: error=%s result=%s\n",
                         qPrintable(error),
                         qPrintable(result));
        }
        return success ? 0 : 1;
    }
    image.setPixelColor(0, 0, Qt::black);
    QTemporaryDir directory;
    if (!directory.isValid() || !QFile::copy(app.applicationFilePath(),
                                             directory.filePath("tesseract"))) {
        return 1;
    }
    qputenv("PATH", directory.path().toUtf8());
    qunsetenv("LINSCREEN_OCR_TEST_MODE");
    if (app.arguments().contains("--dialog")) {
        QApplication::clipboard()->setText("clipboard unchanged");
        OcrDialog dialog(image);
        dialog.resize(390, 520);
        dialog.show();
        app.processEvents();
        auto* buttons = dialog.findChild<QDialogButtonBox*>();
        auto* editor = dialog.findChild<QPlainTextEdit*>();
        auto* ocr = dialog.findChild<LocalOcr*>();
        QPushButton* recognize = nullptr;
        QPushButton* copy = nullptr;
        QPushButton* cancel = nullptr;
        for (auto* button : dialog.findChildren<QPushButton*>()) {
            if (button->text() == "Recognize")
                recognize = button;
            if (button->text() == "Copy text")
                copy = button;
            if (button->text() == "Cancel OCR")
                cancel = button;
        }
        auto* status = dialog.findChild<QLabel*>("ocrStatus");
        if (!buttons || !editor || !ocr || !recognize || !copy || !cancel ||
            !status || dialog.width() > 390 ||
            buttons->orientation() != Qt::Vertical || copy->isEnabled() ||
            ocr->busy())
            return 20;
        QEventLoop recognition;
        QTimer watchdog;
        watchdog.setSingleShot(true);
        QObject::connect(
          &watchdog, &QTimer::timeout, &recognition, &QEventLoop::quit);
        QObject::connect(
          ocr, &LocalOcr::recognized, &recognition, &QEventLoop::quit);
        QObject::connect(
          ocr, &LocalOcr::failed, &recognition, &QEventLoop::quit);
        recognize->click();
        if (!ocr->busy() || recognize->isEnabled() || !editor->isReadOnly() ||
            copy->isEnabled())
            return 21;
        watchdog.start(4000);
        recognition.exec();
        if (ocr->busy() || editor->toPlainText() != "LOCAL OCR 12345" ||
            editor->isReadOnly() || !copy->isEnabled() ||
            QApplication::clipboard()->text() != "clipboard unchanged")
            return 22;
        editor->setPlainText("Corrected text");
        copy->click();
        if (QApplication::clipboard()->text() != "Corrected text")
            return 23;
        // Failed, cancelled and empty retries must not erase user corrections.
        const auto retry = [&](const char* mode, bool cancelRetry) {
            qputenv("LINSCREEN_OCR_TEST_MODE", mode);
            recognize->click();
            if (editor->toPlainText() != "Corrected text")
                return false;
            if (cancelRetry)
                cancel->click();
            if (ocr->busy()) {
                watchdog.start(4000);
                recognition.exec();
            }
            watchdog.stop();
            return !ocr->busy() && editor->toPlainText() == "Corrected text" &&
                   !editor->isReadOnly() && copy->isEnabled() &&
                   recognize->isEnabled() &&
                   status->text().contains("Previous text was kept.") &&
                   QApplication::clipboard()->text() == "Corrected text";
        };
        if (!retry("fail", false))
            return 27;
        if (!retry("hang", true))
            return 28;
        if (!retry("empty", false))
            return 29;
        const auto enginePath = qgetenv("PATH");
        qputenv("PATH", directory.filePath("missing").toUtf8());
        if (!retry("", false))
            return 31;
        qputenv("PATH", enginePath);
        // A later successful retry still replaces the previous recognition.
        qunsetenv("LINSCREEN_OCR_TEST_MODE");
        recognize->click();
        watchdog.start(4000);
        recognition.exec();
        watchdog.stop();
        if (ocr->busy() || editor->toPlainText() != "LOCAL OCR 12345" ||
            QApplication::clipboard()->text() != "Corrected text")
            return 30;
        editor->clear();
        if (copy->isEnabled())
            return 24;
        dialog.resize(680, 520);
        app.processEvents();
        if (buttons->orientation() != Qt::Horizontal)
            return 25;
        dialog.resize(390, 520);
        app.processEvents();
        if (dialog.width() > 390 || buttons->orientation() != Qt::Vertical)
            return 26;
        dialog.close();
        return 0;
    }
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
