// SPDX-License-Identifier: GPL-3.0-or-later
#include "tools/redact/redacttool.h"

#include <QApplication>
#include <QBuffer>
#include <QImage>
#include <QPainter>
#include <QtMath>
#include <cstdio>
#include <memory>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    for (qreal scale : { 1.0, 1.25, 1.5, 2.0 }) {
        for (bool reverse : { false, true }) {
            QPixmap source(QSize(80, 60) * scale);
            source.setDevicePixelRatio(scale);
            source.fill(QColor(255, 0, 120, 140));
            {
                QPainter control(&source);
                control.setCompositionMode(QPainter::CompositionMode_Source);
                control.fillRect(QRect(0, 0, 2, 2), QColor(12, 33, 44));
            }
            QPixmap output = source.copy();
            RedactTool tool;
            tool.move(reverse ? QPoint(50, 40) : QPoint(10, 10));
            tool.drawMove(reverse ? QPoint(10, 10) : QPoint(50, 40));
            tool.onColorChanged(QColor(0, 255, 0, 20));
            tool.onSizeChanged(100);
            std::unique_ptr<CaptureTool> clone(tool.copy());
            {
                QPainter painter(&output);
                painter.setOpacity(0.15);
                painter.setCompositionMode(
                  QPainter::CompositionMode_DestinationOver);
                const qreal previousOpacity = painter.opacity();
                clone->process(painter, source);
                if (painter.opacity() != previousOpacity ||
                    painter.compositionMode() !=
                      QPainter::CompositionMode_DestinationOver) {
                    std::fprintf(stderr, "Painter state was not restored\n");
                    return 1;
                }
            }
            QByteArray bytes;
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::WriteOnly);
            if (!output.save(&buffer, "PNG")) {
                std::fprintf(stderr, "PNG export failed\n");
                return 2;
            }
            QImage exported = QImage::fromData(bytes, "PNG");
            for (int y = qCeil(11 * scale); y < qFloor(39 * scale); ++y) {
                for (int x = qCeil(11 * scale); x < qFloor(49 * scale); ++x) {
                    if (exported.pixelColor(x, y) != QColor(Qt::black)) {
                        std::fprintf(
                          stderr, "Redaction is not opaque in exported PNG\n");
                        return 3;
                    }
                }
            }
            if (exported.pixelColor(0, 0) !=
                source.toImage().pixelColor(0, 0)) {
                std::fprintf(stderr, "Unmasked control pixel changed\n");
                return 4;
            }
        }
    }
    return 0;
}
