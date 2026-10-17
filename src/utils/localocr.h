// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QImage>
#include <QObject>
#include <QProcess>
#include <QTimer>

class LocalOcr : public QObject
{
    Q_OBJECT
public:
    explicit LocalOcr(QObject* parent = nullptr);
    ~LocalOcr() override;
    void start(const QImage& image,
               const QString& language,
               int timeoutMs = 30000);
    void cancel();
    bool busy() const { return m_active; }
signals:
    void recognized(const QString& text);
    void failed(const QString& message);

private:
    void drainOutput();
    void complete(int exitCode, QProcess::ExitStatus status);
    QProcess m_process;
    QTimer m_deadline;
    QByteArray m_input;
    QByteArray m_output;
    QString m_failure;
    bool m_active = false;
};
