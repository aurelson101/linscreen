// SPDX-License-Identifier: GPL-3.0-or-later
#include "localocr.h"

#include <QBuffer>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace {
constexpr qsizetype MAX_OUTPUT = 1024 * 1024;
constexpr qint64 MAX_PIXELS = 16000000;
constexpr qsizetype OCR_MAX_INPUT_BYTES = 16 * 1024 * 1024;
}

LocalOcr::LocalOcr(QObject* parent)
  : QObject(parent)
{
    m_deadline.setSingleShot(true);
    m_process.setStandardErrorFile(QProcess::nullDevice());
    connect(&m_process, &QProcess::started, this, [this]() {
        if (!m_failure.isEmpty()) {
            m_process.kill();
            return;
        }
        m_process.write(m_input);
        m_input.clear();
        m_process.closeWriteChannel();
    });
    connect(&m_process,
            &QProcess::readyReadStandardOutput,
            this,
            &LocalOcr::drainOutput);
    connect(&m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            &LocalOcr::complete);
    connect(&m_process,
            &QProcess::errorOccurred,
            this,
            [this](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart && m_active) {
                    m_failure = tr("Could not start the local OCR engine.");
                    complete(-1, QProcess::CrashExit);
                }
            });
    connect(&m_deadline, &QTimer::timeout, this, [this]() {
        m_failure = tr("OCR timed out. Select a smaller area and retry.");
        m_process.kill();
    });
}

LocalOcr::~LocalOcr()
{
    m_process.disconnect(this);
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
}

void LocalOcr::start(const QImage& image,
                     const QString& language,
                     int timeoutMs)
{
    if (m_active || m_process.state() != QProcess::NotRunning) {
        return;
    }
    if (language != "eng" && language != "fra" && language != "fra+eng") {
        emit failed(tr("Unsupported OCR language."));
        return;
    }
    if (image.isNull() || qint64(image.width()) * image.height() > MAX_PIXELS) {
        emit failed(
          tr("Select a non-empty area of at most 16 million pixels."));
        return;
    }
    const QString engine = QStandardPaths::findExecutable("tesseract");
    if (engine.isEmpty()) {
        emit failed(
          tr("Install Tesseract and the French/English language data, "
             "then retry. Nothing was uploaded."));
        return;
    }
    QBuffer buffer(&m_input);
    m_input.clear();
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, "PNG") || m_input.size() > OCR_MAX_INPUT_BYTES) {
        m_input.clear();
        emit failed(tr("The selected image is too large for local OCR."));
        return;
    }
    buffer.close();
    m_output.clear();
    m_failure.clear();
    auto environment = QProcessEnvironment::systemEnvironment();
    if (environment.contains("APPIMAGE")) {
        environment.remove("LD_LIBRARY_PATH");
        environment.remove("LD_PRELOAD");
    }
    environment.insert("OMP_THREAD_LIMIT", "2");
    m_process.setProcessEnvironment(environment);
    m_process.setProgram(engine);
    m_process.setArguments({ "stdin", "stdout", "-l", language, "--psm", "6" });
    m_active = true;
    m_deadline.start(qBound(1, timeoutMs, 60000));
    m_process.start();
}

void LocalOcr::cancel()
{
    if (m_active) {
        m_failure = tr("OCR cancelled.");
        m_deadline.stop();
        m_process.kill();
    }
}

void LocalOcr::drainOutput()
{
    if (!m_active) {
        return;
    }
    m_output += m_process.read(MAX_OUTPUT - m_output.size() + 1);
    if (m_output.size() > MAX_OUTPUT) {
        m_output.clear();
        m_failure = tr("OCR output exceeded the safety limit.");
        m_process.kill();
    }
}

void LocalOcr::complete(int exitCode, QProcess::ExitStatus status)
{
    if (!m_active) {
        return;
    }
    drainOutput();
    m_active = false;
    m_deadline.stop();
    m_input.clear();
    const QString text = QString::fromUtf8(m_output).trimmed();
    m_output.clear();
    if (!m_failure.isEmpty()) {
        emit failed(m_failure);
    } else if (status != QProcess::NormalExit || exitCode != 0) {
        emit failed(tr("Recognition failed. Check that the selected language "
                       "data is installed, or try another language."));
    } else {
        emit recognized(text);
    }
}
