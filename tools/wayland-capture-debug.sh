#!/usr/bin/env sh
set -eu

build_dir="${1:-build-wayland-debug}"
linscreen_bin="./${build_dir}/src/linscreen"

echo "== LinScreen Wayland capture debug =="
echo "Date: $(date -Is)"
echo "Kernel: $(uname -a)"
echo

echo "== Session environment =="
env | sort | grep -E '^(XDG_|WAYLAND_|QT_|KDE_|GNOME_|GDM|DESKTOP_SESSION=)' || true
echo

echo "== Portal services =="
if command -v busctl >/dev/null 2>&1; then
    busctl --user list | grep -E 'org.freedesktop.portal|xdg-desktop-portal' || true
else
    echo "busctl not found"
fi
echo

echo "== Portal backend processes =="
ps -eo pid,comm,args | grep -E '[x]dg-desktop-portal($|-)' || true
echo

if [ ! -x "${linscreen_bin}" ]; then
    echo "Binary not found or not executable: ${linscreen_bin}" >&2
    echo "Build first with:" >&2
    echo "  cmake -S . -B ${build_dir} -DCMAKE_BUILD_TYPE=Debug -DLINSCREEN_DEBUG_CAPTURE=ON -DENABLE_IMGUR=OFF" >&2
    echo "  cmake --build ${build_dir} -j\"\$(nproc)\"" >&2
    exit 1
fi

echo "== Running LinScreen =="
QT_LOGGING_RULES="${QT_LOGGING_RULES:-qt.qpa.*=true;qt.dbus.*=true}" \
    "${linscreen_bin}" gui
