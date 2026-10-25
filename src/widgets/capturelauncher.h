// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2018 Alejandro Sirgo Rica & Contributors

#pragma once

#include <QDialog>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class CaptureLauncher;
}
QT_END_NAMESPACE

class CaptureLauncher : public QDialog
{
    Q_OBJECT

public:
    explicit CaptureLauncher(QDialog* parent = nullptr);
    ~CaptureLauncher();

private:
    Ui::CaptureLauncher* ui;
    QTimer* m_countdownTimer;
    QTimer* m_captureTimer;
    int m_remainingSeconds;
    void setCaptureControlsEnabled(bool enabled);
    void updateCaptureModeControls();
    void connectCaptureSlots() const;
    void disconnectCaptureSlots() const;
    void updateCountdown();

protected:
    void reject() override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void startCapture();
    void cancelPendingCapture();
    void onCaptureTaken(QPixmap const& p);
    void onCaptureFailed();
};
