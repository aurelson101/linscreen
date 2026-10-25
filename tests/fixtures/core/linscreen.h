// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "core/capturerequest.h"
#include <QObject>

// Substitute only the capture backend: tests exercise the production launcher
// and its real Qt timers, without ever reading or capturing the user's screen.
class LinScreen : public QObject
{
    Q_OBJECT
public:
    int requests = 0;
    uint lastDelay = 0;
    CaptureRequest::CaptureMode lastMode = CaptureRequest::GRAPHICAL_MODE;
    QVariant lastData;
    bool lastHasSelectedMonitor = false;
    int lastSelectedMonitor = -1;
    bool captureSucceeds = false;
    static LinScreen* instance()
    {
        static LinScreen backend;
        return &backend;
    }
    void requestCapture(const CaptureRequest& request)
    {
        ++requests;
        lastDelay = request.delay();
        lastMode = request.captureMode();
        lastData = request.data();
        lastHasSelectedMonitor = request.hasSelectedMonitor();
        lastSelectedMonitor = request.selectedMonitor();
        emit requestStarted();
        if (captureSucceeds) {
            QPixmap image(8, 8);
            image.fill(Qt::white);
            emit captureTaken(image);
        } else {
            emit captureFailed();
        }
    }
signals:
    void requestStarted();
    void captureTaken(QPixmap pixmap);
    void captureFailed();
};
