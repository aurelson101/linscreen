// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "tools/abstracttwopointtool.h"

class RedactTool : public AbstractTwoPointTool
{
    Q_OBJECT
public:
    explicit RedactTool(QObject* parent = nullptr);
    QIcon icon(const QColor& background, bool inEditor) const override;
    QString name() const override;
    QString description() const override;
    CaptureTool::Type type() const override;
    QRect boundingRect() const override;
    CaptureTool* copy(QObject* parent = nullptr) override;
    void process(QPainter& painter, const QPixmap& pixmap) override;
    void pressed(CaptureContext& context) override;
    void paintMousePreview(QPainter& painter,
                           const CaptureContext& context) override;
};
