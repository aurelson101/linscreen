# LinScreen

LinScreen is a screenshot and annotation tool for Linux desktops, with portal capture on Wayland and automatic X11 fallback. It targets GNOME, KDE Plasma, XFCE, Cinnamon, MATE, LXQt, wlroots compositors, and multi-monitor setups. Windows and macOS packages are also built in CI for compatibility testing.

It is a rebranded, Wayland-focused fork intended to be packaged as `linscreen` with desktop ID `org.linscreen.LinScreen`.

Website: <https://linscreen.aurelson.com/>

Releases: <https://github.com/aurelson101/linscreen/releases>

Current release: **14.4.1** — [release notes](docs/releases/v14.4.1.md).

- [Download the AppImage](https://github.com/aurelson101/linscreen/releases/download/v14.4.1/LinScreen-14.4.1-x86_64.AppImage)
- [Download the Debian package](https://github.com/aurelson101/linscreen/releases/download/v14.4.1/linscreen_14.4.1_amd64.deb)

## Features

- Local French/English OCR (Shift+T): review and edit recognized text before explicitly copying it. Requires optional Tesseract; no cloud upload or temporary image file.
- Region, full-screen, and per-monitor screenshots.
- Annotation tools for arrows, text, shapes, counters, blur/pixelate, copy, and save.
- Wayland capture through `xdg-desktop-portal`.
- X11 fallback support.
- Desktop integration for GNOME, KDE Plasma, XFCE, Cinnamon, MATE, LXQt, and wlroots sessions.
- Responsive, system-themed configuration and capture dialogs for compact and high-DPI displays.
- Scrollable configuration tabs, accessible region fields, validated coordinates, and safer multi-monitor window placement.
- Scrollable settings panels on small displays and capture controls locked during the countdown.
- Cancel a delayed capture using the Cancel capture button or Escape; closing the launcher also cancels its pending timer.
- Opaque black redaction (Shift+R) for manually selected sensitive areas, flattened into exported raster images.
- Local, anonymized capture diagnostics under About > Capture diagnostics, with portal status and display scaling.
- Debian package, AppImage, and Linux tarball generation from release scripts.
- Windows MSI/ZIP and macOS DMG packaging workflows, with release signing and notarization support.
- Portal debug helper for tricky Wayland desktop setups.

## Install The Debian Package

Build or download the package, then install it with:

```sh
sudo apt install ./linscreen_14.4.1_amd64.deb
```

Run this command in the download directory. Locally built packages are under
`build-linscreen-release/`. The Debian package refreshes desktop/icon caches after
install. If the launcher is still missing, log out and back in once.

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

## Optional Local OCR

Install the local engine and language data (also needed for the AppImage):

```sh
sudo apt install tesseract-ocr tesseract-ocr-fra tesseract-ocr-eng
```

Select a region, choose **Extract text (OCR)** or Shift+T, select French,
English, or both, then click Recognize. Review/correct the result before Copy
text. OCR uses the edited capture, including opaque redactions. Recognition is
cancellable, limited to 30 seconds, 16 million pixels and 1 MiB of output.
Text recognition can make mistakes; it is not automatic sensitive-data detection.
Windows/macOS users need Tesseract and language data installed on their PATH.

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

- `linscreen_14.4.1_amd64.deb`
- `linscreen-14.4.1-linux.tar.gz`
- `LinScreen-14.4.1-x86_64.AppImage`
- `src/linscreen`

## Install The AppImage

Download the AppImage, make it executable, and run it:

```sh
chmod +x LinScreen-14.4.1-x86_64.AppImage
./LinScreen-14.4.1-x86_64.AppImage
```

The AppImage is portable and does not install desktop files globally. It bundles
the Qt Wayland platform plugin and English/French Qt translation catalogs. On
Wayland, make sure a desktop portal backend is available, for example
`xdg-desktop-portal-gtk`, `xdg-desktop-portal-gnome`,
`xdg-desktop-portal-kde`, or `xdg-desktop-portal-wlr`.

Build the AppImage locally with:

```sh
tools/package-linscreen-appimage.sh
```

The AppImage is written under `build-linscreen-appimage/` by default.

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

For Wayland capture issues on any supported desktop, build a debug binary and run:

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

Download the standalone bundle from the GitHub release, then install and run it:

```sh
flatpak install --user ./LinScreen-14.4.1-x86_64.flatpak
flatpak run org.linscreen.LinScreen
```

This is a direct-download bundle, not a Flathub app listing. It includes local
Tesseract OCR and French/English models. Flatpak may download the KDE runtime
from Flathub; host portal packages are still required for Wayland capture.
Updates are installed manually from a newer bundle. Verify the release checksums
before installation.

To build, install `jq` and either native `flatpak-builder` with `elfutils` or the builder Flatpak:

```sh
flatpak install --user flathub org.flatpak.Builder
flatpak install --user flathub org.kde.Platform//6.10 org.kde.Sdk//6.10
```

From a clean, committed checkout, run:

```sh
tools/package-linscreen-flatpak.sh
```

The versioned bundle and `SHA256SUMS-flatpak` are written under
`build-linscreen-flatpak/`. The script snapshots tracked sources and refuses
unmanaged output directories. It builds and tests inside the SDK without
installing system development packages. The application sandbox has no network
or unrestricted home access. Direct document access is limited to Pictures;
the KDE runtime can also read theme settings. Build parallelism defaults to two
jobs and can be changed with `FLATPAK_JOBS`.

## Windows

Windows packaging requires a Windows Qt installation, `windeployqt`, CMake, a supported compiler, and an installer toolchain.

```powershell
powershell -ExecutionPolicy Bypass -File tools/package-linscreen-windows.ps1 `
  -QtPrefix C:\Qt\6.10.2\msvc2022_64 `
  -WindeployQt C:\Qt\6.10.2\msvc2022_64\bin\windeployqt.exe
```

A native Linux Qt installation cannot produce `linscreen.exe`.

Public Windows releases must use an Authenticode certificate. The packaging
script signs and verifies the executables and MSI when a certificate is
provided:

```powershell
$env:WINDOWS_SIGNING_CERTIFICATE_PATH = "C:\secure\certificate.pfx"
$env:WINDOWS_SIGNING_CERTIFICATE_PASSWORD = "<secret>"
./tools/package-linscreen-windows.ps1 -RequireSigning
```

Keep the certificate and password outside the repository. Unsigned packages
are development artifacts and may trigger Microsoft SmartScreen.

## macOS

On macOS with Qt 6 and CMake installed, build the app and DMG with:

```sh
tools/package-linscreen-macos.sh
```

Public macOS releases require a Developer ID identity and App Store Connect API
credentials. With `REQUIRE_SIGNING=1`, the script signs the app and DMG,
submits the DMG for notarization, staples the ticket, and validates it with
Gatekeeper. Ad hoc builds are intended only for development.

## Release

Create or update the GitHub release with:

```sh
gh auth login
tools/publish-linscreen-github-release.sh aurelson101/linscreen
```

The script reads `LINSCREEN_VERSION` from `CMakeLists.txt`, builds missing artifacts, pushes the current branch to `main`, tags `v<version>`, and uploads:

- Debian package
- AppImage
- Linux tarball
- `SHA256SUMS`

The GitHub Actions Release workflow additionally validates the packages,
generates a CycloneDX SBOM, and signs `SHA256SUMS` with Sigstore. Prefer this
workflow for public releases. Windows and macOS tag workflows require their
signing secrets to be configured before publishing packages for those platforms.


## License

LinScreen is distributed under the GNU General Public License v3.0. See `LICENSE`.
