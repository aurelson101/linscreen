#include "waylandutils.h"

#include <QProcessEnvironment>
#include <QString>

WaylandUtils::WaylandUtils() {}

bool WaylandUtils::waylandDetected()
{
    const auto env = QProcessEnvironment::systemEnvironment();
    return env.value(QStringLiteral("XDG_SESSION_TYPE")) ==
             QLatin1String("wayland") ||
           env.value(QStringLiteral("WAYLAND_DISPLAY"))
             .contains(QLatin1String("wayland"), Qt::CaseInsensitive);
}
