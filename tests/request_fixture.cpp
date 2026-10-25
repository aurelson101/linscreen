// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/capturerequest.h"

// Rendering/launcher fixtures must not read the user's saved configuration.
CaptureRequest::CaptureRequest(CaptureMode mode,
                               const uint delay,
                               QVariant data,
                               ExportTask tasks)
  : m_mode(mode)
  , m_delay(delay)
  , m_tasks(tasks)
  , m_data(data)
  , m_selectedMonitor(-1)
  , m_hasSelectedMonitor(false)
{}

void CaptureRequest::setInitialSelection(const QRect& selection)
{
    m_initialSelection = selection;
}

void CaptureRequest::setSelectedMonitor(int monitor)
{
    m_selectedMonitor = monitor;
    m_hasSelectedMonitor = true;
}

uint CaptureRequest::delay() const
{
    return m_delay;
}

CaptureRequest::CaptureMode CaptureRequest::captureMode() const
{
    return m_mode;
}

QVariant CaptureRequest::data() const
{
    return m_data;
}

bool CaptureRequest::hasSelectedMonitor() const
{
    return m_hasSelectedMonitor;
}

int CaptureRequest::selectedMonitor() const
{
    return m_selectedMonitor;
}
