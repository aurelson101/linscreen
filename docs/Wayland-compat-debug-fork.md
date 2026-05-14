# Wayland Compatibility Debug Fork

This branch is a staging area for making LinScreen's Linux capture path easier
to debug on GNOME, KDE Plasma, and multi-monitor Wayland sessions.

## Compatibility target

Wayland does not allow applications to read pixels directly from other
applications or from the compositor. A compatible LinScreen build must therefore
use the desktop portal screenshot API on GNOME and KDE, and must treat direct
screen grabbing as an X11-only fallback.

The practical target for this fork is:

- GNOME Wayland through `xdg-desktop-portal-gnome`.
- KDE Plasma Wayland through `xdg-desktop-portal-kde`.
- One or more monitors, including negative monitor origins and different scale
  factors where the portal returns a single combined image.
- Explicit monitor selection after the portal capture when a compositor does not
  expose a trustworthy "active monitor" signal to clients.
- Debug logs that explain portal availability, environment detection, request
  failures, cancellation, and crop calculations.

## Known Wayland limits

These are compositor/security constraints, not LinScreen bugs:

- Capturing the active monitor under the pointer cannot be made fully automatic
  on generic Wayland without compositor-specific APIs or user interaction.
- Global shortcuts are controlled by the desktop environment. LinScreen can
  expose commands, but GNOME/KDE decide how shortcuts are registered and invoked.
- The portal may show a permission prompt or compositor UI. LinScreen must handle
  cancellation cleanly.

## Debug build

```sh
cmake -S . -B build-wayland-debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLINSCREEN_DEBUG_CAPTURE=ON \
  -DENABLE_IMGUR=OFF
cmake --build build-wayland-debug -j"$(nproc)"
```

Run with Qt and portal logs enabled:

```sh
QT_LOGGING_RULES="qt.qpa.*=true;qt.dbus.*=true" \
XDG_SESSION_TYPE=wayland \
./build-wayland-debug/src/linscreen gui
```

Or collect session, portal, and Qt logs with:

```sh
sh tools/wayland-capture-debug.sh build-wayland-debug
```

## First fork changes

- The portal capture path now logs the active Wayland/desktop environment when
  `LINSCREEN_DEBUG_CAPTURE` is enabled.
- Direct D-Bus errors from `org.freedesktop.portal.Screenshot` are reported
  immediately instead of looking like a 30 second hang.
- Portal cancellation/denial and empty image URI responses are logged explicitly.
- The unused `WaylandUtils::waylandDetected()` helper now returns a real value
  and is included in the build, so future Wayland-specific code can share one
  implementation.
- `tools/wayland-capture-debug.sh` collects session variables, portal services,
  backend processes, and then launches the debug build with Qt platform and D-Bus
  logging enabled.
