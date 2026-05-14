# LinScreen

LinScreen is a GPL-3.0 screenshot and annotation tool focused on reliable Linux desktop capture, especially Wayland sessions on GNOME and KDE with one or more monitors.

## Highlights

- Interactive region capture with annotation tools.
- Full-screen and per-monitor capture commands.
- Wayland capture through desktop portals.
- X11 fallback support.
- Desktop integration through AppStream, desktop entry, DBus, icons, shell completions, and man page.
- Debug helper for Wayland portal issues.
- Linux packaging through CPack Debian and tarball artifacts.

## Build on Debian or Ubuntu

Install dependencies:

```sh
sudo apt install cmake build-essential ninja-build \
  qt6-base-dev qt6-tools-dev qt6-tools-dev-tools qt6-svg-dev \
  qt6-l10n-tools libgl-dev appstream desktop-file-utils
```

Build packages:

```sh
tools/package-linscreen-deb.sh
```

The generated artifacts are written under `build-linscreen-release/`:

- `linscreen_14.0.0_amd64.deb`
- `linscreen-14.0.0-linux.tar.gz`
- `src/linscreen`

## Run

```sh
linscreen gui
linscreen full --path ~/Pictures/capture.png
linscreen screen --number 0 --path ~/Pictures/monitor.png
linscreen config
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

The debug output reports session variables, portal services, and capture portal failures.

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

The GitHub release workflow builds the Debian package and tarball when a version tag is pushed:

```sh
git tag v14.0.0
git push origin v14.0.0
```

Release assets include:

- `.deb`
- Linux `.tar.gz`
- `SHA256SUMS`

To create the GitHub repository and publish the current local `.deb` manually,
install GitHub CLI, authenticate, then run:

```sh
gh auth login
tools/publish-linscreen-github-release.sh aurelson101/linscreen v14.0.0
```

## License

LinScreen is distributed under the GNU General Public License v3.0. See `LICENSE`.
