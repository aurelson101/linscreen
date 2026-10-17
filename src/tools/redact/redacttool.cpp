// SPDX-License-Identifier: GPL-3.0-or-later
#include "redacttool.h"

RedactTool::RedactTool(QObject* parent)
  : AbstractTwoPointTool(parent)
{}

QIcon RedactTool::icon(const QColor& background, bool inEditor) const
{
    Q_UNUSED(inEditor)
    return QIcon(iconPath(background) + "square.svg");
}

QString RedactTool::name() const
{
    return tr("Redact");
}

QString RedactTool::description() const
{
    return tr("Cover sensitive information with opaque black. Exported images "
              "contain the mask; undo remains available while editing.");
}

CaptureTool::Type RedactTool::type() const
{
    return CaptureTool::TYPE_REDACT;
}

QRect RedactTool::boundingRect() const
{
    return QRect(points().first, points().second).normalized();
}

CaptureTool* RedactTool::copy(QObject* parent)
{
    auto* tool = new RedactTool(parent);
    copyParams(this, tool);
    return tool;
}

void RedactTool::process(QPainter& painter, const QPixmap& pixmap)
{
    Q_UNUSED(pixmap)
    painter.save();
    painter.setOpacity(1.0);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(boundingRect(), Qt::black);
    painter.restore();
}

void RedactTool::pressed(CaptureContext& context)
{
    Q_UNUSED(context)
}

void RedactTool::paintMousePreview(QPainter& painter,
                                   const CaptureContext& context)
{
    painter.save();
    painter.setOpacity(1.0);
    painter.fillRect(QRect(context.mousePos, QSize(8, 8)), Qt::black);
    painter.restore();
}
