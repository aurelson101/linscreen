# LinScreen

LinScreen is a screenshot and annotation tool for Linux desktops, with a focus on Wayland sessions on GNOME and KDE, including multi-monitor setups.

It is a rebranded, Wayland-focused fork intended to be packaged as `linscreen` with desktop ID `org.linscreen.LinScreen`.

## Features

- Region, full-screen, and per-monitor screenshots.
- Annotation tools for arrows, text, shapes, counters, blur/pixelate, copy, and save.
- Wayland capture through `xdg-desktop-portal`.
- X11 fallback support.
- GNOME/KDE desktop integration through desktop entry, AppStream metadata, DBus service, icons, completions, and man page.
- Debian package and Linux tarball generation from one script.
- Wayland portal debug helper for tricky GNOME/KDE setups.

## Install The Debian Package

Build or download the package, then install it with:

```sh
sudo apt install ./build-linscreen-release/linscreen_14.0.1_amd64.deb
```

The Debian package refreshes desktop/icon caches after install. If the launcher is still missing in GNOME or KDE, log out and back in once.

Quick checks:

```sh
linscreen --version
gtk-launch org.linscreen.LinScreen
grep -E '^(Exec|Icon|Name)=' /usr/share/applications/org.linscreen.LinScreen.desktop
```

Expected desktop entry values:

```ini
Name=LinScreen
Exec=/usr/bin/linscreen
Icon=org.linscreen.LinScreen
```

## Build On Debian Or Ubuntu

Install dependencies:

```sh
sudo apt install cmake build-essential ninja-build \
  qt6-base-dev qt6-tools-dev qt6-tools-dev-tools qt6-svg-dev \
  qt6-l10n-tools libgl-dev appstream desktop-file-utils
```

Build release artifacts:

```sh
tools/package-linscreen-deb.sh
```

Artifacts are written under `build-linscreen-release/`:

- `linscreen_14.0.1_amd64.deb`
- `linscreen-14.0.1-linux.tar.gz`
- `src/linscreen`

## Run

```sh
linscreen
linscreen gui
linscreen full --path ~/Pictures/capture.png
linscreen screen --number 0 --path ~/Pictures/monitor.png
linscreen config
```

## Wayland Portal Notes

LinScreen uses the desktop portal on Wayland. The important installed files are:

- `/usr/share/applications/org.linscreen.LinScreen.desktop`
- `/usr/share/dbus-1/services/org.linscreen.LinScreen.service`
- `/usr/share/icons/hicolor/scalable/apps/org.linscreen.LinScreen.svg`

If you see:

```text
Could not register app ID: App info not found for 'org.linscreen.LinScreen'
```

verify the desktop entry exists and points to `/usr/bin/linscreen`, then refresh caches or reconnect your session:

```sh
sudo update-desktop-database /usr/share/applications
sudo gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor
gtk-launch org.linscreen.LinScreen
```

## Wayland Debugging

For GNOME/KDE Wayland capture issues, build a debug binary and run:

```sh
cmake -S . -B build-wayland-debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLINSCREEN_DEBUG_CAPTURE=ON \
  -DENABLE_IMGUR=OFF
cmake --build build-wayland-debug -j"$(nproc)"
tools/wayland-capture-debug.sh build-wayland-debug
```

The debug output reports session variables, portal services, backend processes, and capture portal failures.

## Flatpak

Install `flatpak-builder`, `org.kde.Platform//6.9`, and `org.kde.Sdk//6.9`, then run:

```sh
tools/package-linscreen-flatpak.sh
```

The bundle is written to `LinScreen.flatpak`.

## Windows

Windows packaging requires a Windows Qt installation, `windeployqt`, CMake, a supported compiler, and an installer toolchain.

```powershell
powershell -ExecutionPolicy Bypass -File tools/package-linscreen-windows.ps1 `
  -QtPrefix C:\Qt\6.10.2\msvc2022_64 `
  -WindeployQt C:\Qt\6.10.2\msvc2022_64\bin\windeployqt.exe
```

A native Linux Qt installation cannot produce `linscreen.exe`.

## Release

Create or update the GitHub release with:

```sh
gh auth login
tools/publish-linscreen-github-release.sh aurelson101/linscreen
```

The script reads `LINSCREEN_VERSION` from `CMakeLists.txt`, builds missing artifacts, pushes the current branch to `main`, tags `v<version>`, and uploads:

- Debian package
- Linux tarball
- `SHA256SUMS`


## License

LinScreen is distributed under the GNU General Public License v3.0. See `LICENSE`.
