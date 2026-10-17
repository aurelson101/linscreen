// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDialog>
#include <QImage>

class QDialogButtonBox;

class OcrDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OcrDialog(const QImage& image, QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    QDialogButtonBox* m_buttons;
};
