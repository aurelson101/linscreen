# LinScreen Packaging

The fork is named **LinScreen**.

- Binary: `linscreen`
- Windows CLI wrapper: `linscreen-cli`
- Desktop/AppStream/DBus ID: `org.linscreen.LinScreen`
- Debian package name: `linscreen`
- Flatpak app ID: `org.linscreen.LinScreen`

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
