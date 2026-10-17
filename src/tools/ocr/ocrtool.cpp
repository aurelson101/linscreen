// SPDX-License-Identifier: GPL-3.0-or-later
#include "ocrtool.h"
#include "widgets/ocrdialog.h"

OcrTool::OcrTool(QObject* parent)
  : AbstractActionTool(parent)
{}

bool OcrTool::closeOnButtonPressed() const
{
    return true;
}
QIcon OcrTool::icon(const QColor& background, bool inEditor) const
{
    Q_UNUSED(inEditor)
    return QIcon(iconPath(background) + "text.svg");
}
QString OcrTool::name() const
{
    return tr("Extract text (OCR)");
}
QString OcrTool::description() const
{
    return tr(
      "Recognize text locally in the selected area; review before copying");
}
CaptureTool::Type OcrTool::type() const
{
    return CaptureTool::TYPE_OCR;
}
CaptureTool* OcrTool::copy(QObject* parent)
{
    return new OcrTool(parent);
}
QWidget* OcrTool::widget()
{
    return new OcrDialog(m_capture);
}
void OcrTool::pressed(CaptureContext& context)
{
    emit requestAction(REQ_CLEAR_SELECTION);
    // Use the edited image, never the unredacted original.
    m_capture = context.selectedScreenshotArea().toImage();
    emit requestAction(REQ_CAPTURE_DONE_OK);
    emit requestAction(REQ_ADD_EXTERNAL_WIDGETS);
    emit requestAction(REQ_CLOSE_GUI);
}
