// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "buttonhandler.h"
#include "utils/globalvalues.h"

#include <QPoint>
#include <QScreen>
#include <algorithm>

// ButtonHandler is a handler for every active button. It makes easier to
// manipulate the buttons as a unit.

ButtonHandler::ButtonHandler(const QVector<CaptureToolButton*>& v,
                             QObject* parent)
  : QObject(parent)
{
    init();
    setButtons(v);
}

ButtonHandler::ButtonHandler(QObject* parent)
  : QObject(parent)
{
    init();
}

void ButtonHandler::hide()
{
    for (CaptureToolButton* b : m_vectorButtons) {
        b->hide();
    }
}

void ButtonHandler::show()
{
    if (m_vectorButtons.isEmpty() ||
        std::none_of(m_screenRegions.cbegin(),
                     m_screenRegions.cend(),
                     [](const QRect& screen) { return !screen.isEmpty(); }) ||
        m_vectorButtons.first()->isVisible()) {
        return;
    }
    for (CaptureToolButton* b : m_vectorButtons) {
        b->animatedShow();
    }
}

bool ButtonHandler::isVisible() const
{
    if (m_vectorButtons.isEmpty()) {
        return false;
    }
    bool ret = true;
    for (const CaptureToolButton* b : m_vectorButtons) {
        if (!b->isVisible()) {
            ret = false;
            break;
        }
    }
    return ret;
}

bool ButtonHandler::buttonsAreInside() const
{
    return m_buttonsAreInside;
}

size_t ButtonHandler::size() const
{
    return m_vectorButtons.size();
}

// updatePosition updates the position of the buttons around the
// selection area. Ignores the sides blocked by the end of the screen.
// When the selection is too small, it works on a virtual selection with
// the original in the center.
void ButtonHandler::updatePosition(const QRect& selection)
{
    resetRegionTrack();
    const int vecLength = m_vectorButtons.size();
    if (vecLength == 0 ||
        std::none_of(m_screenRegions.cbegin(),
                     m_screenRegions.cend(),
                     [](const QRect& screen) { return !screen.isEmpty(); })) {
        hide();
        return;
    }
    // Restore normal dimensions after a previous compact-screen layout.
    for (auto* button : m_vectorButtons) {
        button->setMinimumSize(0, 0);
        button->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        button->resize(m_buttonBaseSize, m_buttonBaseSize);
        button->setIconSize(QSize(m_buttonBaseSize, m_buttonBaseSize) * 0.6);
        button->setMask(
          QRegion(QRect(-1, -1, m_buttonBaseSize + 2, m_buttonBaseSize + 2),
                  QRegion::Ellipse));
        button->setStyleSheet(button->CaptureButton::styleSheet());
    }
    // Copy of the selection area for internal modifications
    m_selection = intersectWithAreas(selection);
    if (m_selection.isEmpty()) {
        positionButtonsInside(selection);
        return;
    }
    updateBlockedSides();
    ensureSelectionMinimumSize();
    // Indicates the actual button to be moved
    int elemIndicator = 0;

    while (elemIndicator < vecLength) {

        // Add them inside the area when there is no more space
        if (m_allSidesBlocked) {
            positionButtonsInside(selection);
            break; // the while
        }
        // Number of buttons per row column
        int buttonsPerRow =
          (m_selection.width() + m_separator) / (m_buttonExtendedSize);
        int buttonsPerCol =
          (m_selection.height() + m_separator) / (m_buttonExtendedSize);
        // Buttons to be placed in the corners
        int extraButtons =
          (vecLength - elemIndicator) - (buttonsPerRow + buttonsPerCol) * 2;
        int elemsAtCorners = extraButtons > 4 ? 4 : extraButtons;
        int maxExtra = 2;
        if (m_oneHorizontalBlocked) {
            maxExtra = 1;
        } else if (m_horizontalyBlocked) {
            maxExtra = 0;
        }
        int elemCornersTop = qBound(0, elemsAtCorners, maxExtra);
        elemsAtCorners -= elemCornersTop;
        int elemCornersBotton = qBound(0, elemsAtCorners, maxExtra);

        // Add buttons at the button of the selection
        if (!m_blockedBotton) {
            int addCounter = buttonsPerRow + elemCornersBotton;
            // Don't add more than we have
            addCounter = qBound(0, addCounter, vecLength - elemIndicator);
            QPoint center = QPoint(m_selection.center().x(),
                                   m_selection.bottom() + m_separator);
            if (addCounter > buttonsPerRow) {
                adjustHorizontalCenter(center);
            }
            // ElemIndicator, elemsAtCorners
            QVector<QPoint> positions =
              horizontalPoints(center, addCounter, true);
            moveButtonsToPoints(positions, elemIndicator);
        }
        // Add buttons to the right side of the selection
        if (!m_blockedRight && elemIndicator < vecLength) {
            int addCounter = buttonsPerCol;
            addCounter = qBound(0, addCounter, vecLength - elemIndicator);

            QPoint center = QPoint(m_selection.right() + m_separator,
                                   m_selection.center().y());
            QVector<QPoint> positions =
              verticalPoints(center, addCounter, false);
            moveButtonsToPoints(positions, elemIndicator);
        }
        // Add buttons at the top of the selection
        if (!m_blockedTop && elemIndicator < vecLength) {
            int addCounter = buttonsPerRow + elemCornersTop;
            addCounter = qBound(0, addCounter, vecLength - elemIndicator);
            QPoint center = QPoint(m_selection.center().x(),
                                   m_selection.top() - m_buttonExtendedSize);
            if (addCounter == 1 + buttonsPerRow) {
                adjustHorizontalCenter(center);
            }
            QVector<QPoint> positions =
              horizontalPoints(center, addCounter, false);
            moveButtonsToPoints(positions, elemIndicator);
        }
        // Add buttons to the left side of the selection
        if (!m_blockedLeft && elemIndicator < vecLength) {
            int addCounter = buttonsPerCol;
            addCounter = qBound(0, addCounter, vecLength - elemIndicator);

            QPoint center = QPoint(m_selection.left() - m_buttonExtendedSize,
                                   m_selection.center().y());
            QVector<QPoint> positions =
              verticalPoints(center, addCounter, true);
            moveButtonsToPoints(positions, elemIndicator);
        }
        // If there are elements for the next cycle, increase the size of the
        // base area
        if (elemIndicator < vecLength && !(m_allSidesBlocked)) {
            expandSelection();
        }
        updateBlockedSides();
    }
}

int ButtonHandler::calculateShift(int elements, bool reverse) const
{
    int shift = 0;
    if (elements % 2 == 0) {
        shift = m_buttonExtendedSize * (elements / 2) - (m_separator / 2);
    } else {
        shift =
          m_buttonExtendedSize * ((elements - 1) / 2) + m_buttonBaseSize / 2;
    }
    if (!reverse) {
        shift -= m_buttonBaseSize;
    }

    return shift;
}
// horizontalPoints is an auxiliary method for the button position computation.
// starts from a known center and keeps adding elements horizontally
// and returns the computed positions.
QVector<QPoint> ButtonHandler::horizontalPoints(const QPoint& center,
                                                const int elements,
                                                const bool leftToRight) const
{
    QVector<QPoint> res;
    // Distance from the center to start adding buttons
    int shift = calculateShift(elements, leftToRight);

    int x = leftToRight ? center.x() - shift : center.x() + shift;
    QPoint i(x, center.y());
    while (elements > res.length()) {
        res.append(i);
        leftToRight ? i.setX(i.x() + m_buttonExtendedSize)
                    : i.setX(i.x() - m_buttonExtendedSize);
    }
    return res;
}

// verticalPoints is an auxiliary method for the button position computation.
// starts from a known center and keeps adding elements vertically
// and returns the computed positions.
QVector<QPoint> ButtonHandler::verticalPoints(const QPoint& center,
                                              const int elements,
                                              const bool upToDown) const
{
    QVector<QPoint> res;
    // Distance from the center to start adding buttons
    int shift = calculateShift(elements, upToDown);

    int y = upToDown ? center.y() - shift : center.y() + shift;
    QPoint i(center.x(), y);
    while (elements > res.length()) {
        res.append(i);
        upToDown ? i.setY(i.y() + m_buttonExtendedSize)
                 : i.setY(i.y() - m_buttonExtendedSize);
    }
    return res;
}

QRect ButtonHandler::intersectWithAreas(const QRect& rect)
{
    QRect res;
    for (const QRect& r : m_screenRegions) {
        QRect temp = rect.intersected(r);
        if (temp.height() * temp.width() > res.height() * res.width()) {
            res = temp;
        }
    }
    return res;
}

void ButtonHandler::init()
{
    m_buttonBaseSize = GlobalValues::buttonBaseSize();
    m_separator = m_buttonBaseSize / 4;
    m_buttonExtendedSize = m_buttonBaseSize + m_separator;
    m_buttonsAreInside = false;
}

void ButtonHandler::resetRegionTrack()
{
    m_buttonsAreInside = false;
}

void ButtonHandler::updateBlockedSides()
{
    QRegion screenRegion{};
    for (const QRect& rect : m_screenRegions) {
        screenRegion += rect;
    }

    const int EXTENSION = m_separator * 2 + m_buttonBaseSize;
    // Right
    QPoint pointA(m_selection.right() + EXTENSION, m_selection.bottom());
    QPoint pointB(pointA.x(), m_selection.top());
    m_blockedRight =
      !(screenRegion.contains(pointA) && screenRegion.contains(pointB));
    // Left
    pointA.setX(m_selection.left() - EXTENSION);
    pointB.setX(pointA.x());
    m_blockedLeft =
      !(screenRegion.contains(pointA) && screenRegion.contains(pointB));
    // Bottom
    pointA = QPoint(m_selection.left(), m_selection.bottom() + EXTENSION);
    pointB = QPoint(m_selection.right(), pointA.y());
    m_blockedBotton =
      !(screenRegion.contains(pointA) && screenRegion.contains(pointB));
    // Top
    pointA.setY(m_selection.top() - EXTENSION);
    pointB.setY(pointA.y());
    m_blockedTop =
      !(screenRegion.contains(pointA) && screenRegion.contains(pointB));
    // Auxiliary
    m_oneHorizontalBlocked =
      (!m_blockedRight && m_blockedLeft) || (m_blockedRight && !m_blockedLeft);
    m_horizontalyBlocked = (m_blockedRight && m_blockedLeft);
    m_allSidesBlocked =
      (m_blockedBotton && m_horizontalyBlocked && m_blockedTop);
}

void ButtonHandler::expandSelection()
{
    int& s = m_buttonExtendedSize;
    m_selection = m_selection + QMargins(s, s, s, s);
    m_selection = intersectWithAreas(m_selection);
}

void ButtonHandler::positionButtonsInside(const QRect& selection)
{
    // Use the actual monitor, not a potentially tiny original selection.
    // Reposition the entire toolbar so old outer rows cannot overlap this grid.
    QRect mainArea;
    qint64 bestOverlap = -1;
    for (const auto& screen : m_screenRegions) {
        const QRect overlap = screen.intersected(selection);
        const qint64 area = qint64(overlap.width()) * overlap.height();
        if (!screen.isEmpty() && area > bestOverlap) {
            bestOverlap = area;
            mainArea = screen;
        }
    }
    if (mainArea.isEmpty()) {
        return;
    }
    const int count = m_vectorButtons.size();
    int side =
      qMin(m_buttonBaseSize, qMin(mainArea.width(), mainArea.height()));
    int gap = qMin(m_separator, side / 4);
    while (side > 1 && ((mainArea.width() + gap) / (side + gap)) *
                           ((mainArea.height() + gap) / (side + gap)) <
                         count) {
        --side;
        gap = qMin(m_separator, side / 4);
    }
    const int columns =
      qMin(count, qMax(1, (mainArea.width() + gap) / (side + gap)));
    const int rows = (count + columns - 1) / columns;
    const int left =
      mainArea.left() +
      (mainArea.width() - columns * side - (columns - 1) * gap) / 2;
    const int top = mainArea.bottom() - rows * side - (rows - 1) * gap + 1;
    for (int index = 0; index < count; ++index) {
        auto* button = m_vectorButtons[index];
        button->setFixedSize(side, side);
        button->setIconSize(QSize(side, side) * 0.6);
        button->setMask(
          QRegion(QRect(-1, -1, side + 2, side + 2), QRegion::Ellipse));
        button->setStyleSheet(
          // Generate a fresh base; QWidget::styleSheet() would accumulate
          // rules.
          button->CaptureButton::styleSheet() +
          QStringLiteral("CaptureToolButton { border-radius: %1px; }")
            .arg(side / 2));
        button->move(left + (index % columns) * (side + gap),
                     qBound(mainArea.top(),
                            top + (index / columns) * (side + gap),
                            mainArea.bottom() - side + 1));
    }

    m_buttonsAreInside = true;
}

void ButtonHandler::ensureSelectionMinimumSize()
{
    // Detect if a side is smaller than a button in order to prevent collision
    // and redimension the base area to the base size of a single button per
    // side
    if (m_selection.width() < m_buttonBaseSize) {
        if (!m_blockedLeft) {
            m_selection.setX(m_selection.x() -
                             (m_buttonBaseSize - m_selection.width()) / 2);
        }
        m_selection.setWidth(m_buttonBaseSize);
    }
    if (m_selection.height() < m_buttonBaseSize) {
        if (!m_blockedTop) {
            m_selection.setY(m_selection.y() -
                             (m_buttonBaseSize - m_selection.height()) / 2);
        }
        m_selection.setHeight(m_buttonBaseSize);
    }
}

void ButtonHandler::moveButtonsToPoints(const QVector<QPoint>& points,
                                        int& index)
{
    for (const QPoint& p : points) {
        auto* button = m_vectorButtons[index];
        button->move(p);
        ++index;
    }
}

void ButtonHandler::adjustHorizontalCenter(QPoint& center)
{
    if (m_blockedLeft) {
        center.setX(center.x() + m_buttonExtendedSize / 2);
    } else if (m_blockedRight) {
        center.setX(center.x() - m_buttonExtendedSize / 2);
    }
}

// setButtons redefines the buttons of the button handler
void ButtonHandler::setButtons(const QVector<CaptureToolButton*>& v)
{
    for (CaptureToolButton* b : m_vectorButtons) {
        delete (b);
    }
    m_vectorButtons = v;
    init();
}

bool ButtonHandler::contains(const QPoint& p) const
{
    for (const auto* button : m_vectorButtons) {
        if (button->geometry()
              .adjusted(-m_separator, -m_separator, m_separator, m_separator)
              .contains(p)) {
            return true;
        }
    }
    return false;
}

void ButtonHandler::updateScreenRegions(const QVector<QRect>& rects)
{
    m_screenRegions = rects;
}

void ButtonHandler::updateScreenRegions(const QRect& rect)
{
    m_screenRegions = { rect };
}
