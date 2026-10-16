# LinScreen Packaging

The fork is named **LinScreen**.

- Binary: `linscreen`
- Windows CLI wrapper: `linscreen-cli`
- Desktop/AppStream/DBus ID: `org.linscreen.LinScreen`
- Debian package name: `linscreen`
- Flatpak app ID: `org.linscreen.LinScreen`
- AppImage artifact: `LinScreen-<version>-x86_64.AppImage`

## Debian package

Install the build dependencies first. On Debian/Ubuntu this is typically:

```sh
sudo apt install cmake build-essential ninja-build \
  qt6-base-dev qt6-tools-dev qt6-tools-dev-tools qt6-svg-dev \
  qt6-l10n-tools libgl-dev
```

Then build:

```sh
tools/package-linscreen-deb.sh
```

The script uses CPack and emits `.deb` and `.tar.gz` artifacts under
`build-linscreen-release/`.

## AppImage

Install the normal build dependencies plus `curl`. The script downloads
`linuxdeploy` and `linuxdeploy-plugin-qt` into the build directory if they are
not already present:

```sh
tools/package-linscreen-appimage.sh
```

The AppImage is written as:

```text
build-linscreen-appimage/LinScreen-<version>-x86_64.AppImage
```

The AppImage build installs into an AppDir with `CMAKE_INSTALL_PREFIX=/usr`,
uses relative desktop launcher commands, bundles Qt dependencies through the
linuxdeploy Qt plugin, and keeps the update checker enabled for portable
release builds.

The AppImage must include Qt's Wayland platform plugin and English/French Qt
translation catalogs so it works cleanly on Wayland sessions in both locales.

GitHub Actions builds and verifies the AppImage through the `AppImage package`
workflow. The workflow checks the Wayland plugin, the French LinScreen catalog,
an autonomous launch and the SHA-256 checksum before uploading the artifact.

## Flatpak

Install `flatpak-builder`, `org.kde.Platform//6.9`, and `org.kde.Sdk//6.9`,
then run:

```sh
tools/package-linscreen-flatpak.sh
```

The bundle is written as `LinScreen.flatpak`.

## Windows

Build on Windows with Qt 6, CMake, a supported C++ compiler, WiX Toolset, and
`windeployqt` available in `PATH`.

```powershell
powershell -ExecutionPolicy Bypass -File tools/package-linscreen-windows.ps1
```

Add `-Portable` to produce the self-contained configuration ZIP instead of the
system-integrated MSI build.

If Qt is not in `PATH`, pass both the Qt prefix and the deploy tool explicitly:

```powershell
powershell -ExecutionPolicy Bypass -File tools/package-linscreen-windows.ps1 `
  -QtPrefix C:\Qt\6.10.2\msvc2022_64 `
  -WindeployQt C:\Qt\6.10.2\msvc2022_64\bin\windeployqt.exe
```

On Linux, this script only works with a real Windows cross-compilation
toolchain and a matching Windows Qt build:

```powershell
pwsh -File tools/package-linscreen-windows.ps1 `
  -ToolchainFile /path/to/mingw-w64-toolchain.cmake `
  -QtPrefix /path/to/windows-qt-prefix `
  -WindeployQt /path/to/windows-qt-prefix/bin/windeployqt.exe
```

A native Linux Qt installation does not provide `windeployqt` and cannot
produce `linscreen.exe`.

The generated executable is `linscreen.exe`, with the console wrapper
`linscreen-cli.exe`.

For a public release, provide an Authenticode `.pfx` certificate. The script
signs and verifies `linscreen.exe`, `linscreen-cli.exe`, and the MSI with
SHA-256 plus an RFC 3161 timestamp:

```powershell
tools/package-linscreen-windows.ps1 `
  -SigningCertificatePath C:\secure\linscreen.pfx `
  -SigningCertificatePassword $env:WINDOWS_CERTIFICATE_PASSWORD `
  -RequireSigning
```

The GitHub release workflow expects `WINDOWS_CERTIFICATE_BASE64` and
`WINDOWS_CERTIFICATE_PASSWORD`. A tag build fails if signing is unavailable.
Authenticode substantially improves publisher trust, but SmartScreen reputation
still depends on a certificate issued by a trusted CA and on download history.

GitHub Actions also builds the native Windows installer and portable archive
through the `Windows package` workflow. Its `LinScreen-<version>-Windows`
artifact contains both the `.msi` and `.zip` outputs and can be started
manually with `workflow_dispatch`.

## macOS

Development DMGs can be built on macOS with:

```sh
tools/package-linscreen-macos.sh
```

The Qt 6.9 build targets macOS 12 or newer, matching Qt's supported platform
baseline.

Public releases require a Developer ID Application certificate and Apple
notarization. The tag workflow imports the certificate into a temporary
keychain, signs the app and DMG with the hardened runtime, submits the DMG to
Apple, staples the notarization ticket, validates it with Gatekeeper, and only
then attaches it to the GitHub release.

Configure these GitHub Actions secrets:

- `MACOS_CERTIFICATE_BASE64`
- `MACOS_CERTIFICATE_PASSWORD`
- `MACOS_SIGN_IDENTITY`
- `APPLE_API_KEY_BASE64`
- `APPLE_API_KEY_ID`
- `APPLE_API_ISSUER_ID`

Ad hoc development builds remain available for testing but are deliberately
not published by a release tag.
