param(
    [string]$BuildDir = "build-linscreen-windows",
    [string]$QtPrefix = $env:Qt6_DIR,
    [string]$ToolchainFile = "",
    [string]$WindeployQt = "",
    [string]$Generator = "",
    [switch]$Portable
)

$ErrorActionPreference = "Stop"

function Resolve-RequiredCommand {
    param(
        [string]$Command,
        [string]$Hint
    )

    $resolved = Get-Command $Command -ErrorAction SilentlyContinue
    if (-not $resolved) {
        throw "$Command is required to build LinScreen for Windows. $Hint"
    }

    return $resolved.Source
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "cmake is required to build LinScreen for Windows."
}

$runningOnWindows = [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
    [System.Runtime.InteropServices.OSPlatform]::Windows)

if (-not $runningOnWindows -and -not $ToolchainFile) {
    throw "This script must run on Windows, or be called from Linux with -ToolchainFile pointing to a Windows cross-compilation toolchain. A native Linux Qt build cannot produce linscreen.exe."
}

if ($WindeployQt) {
    if (-not (Test-Path $WindeployQt)) {
        throw "windeployqt was not found at '$WindeployQt'. Use the windeployqt.exe shipped in the bin directory of your Windows Qt installation."
    }
} else {
    $WindeployQt = Resolve-RequiredCommand "windeployqt" "Install Qt for Windows and add its bin directory to PATH, or pass -WindeployQt C:\Qt\...\bin\windeployqt.exe."
}

function Invoke-CheckedCommand {
    param(
        [string]$Command,
        [string[]]$Arguments
    )

    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Command failed with exit code $LASTEXITCODE"
    }
}

$cmakeArgs = @(
    "-S", ".",
    "-B", $BuildDir,
    "-DCMAKE_BUILD_TYPE=Release",
    "-DENABLE_IMGUR=OFF",
    "-DUSE_PORTABLE_CONFIG=$(if ($Portable) { 'ON' } else { 'OFF' })",
    "-DWINDEPLOYQT_EXECUTABLE=$WindeployQt"
)

if ($QtPrefix) {
    $cmakeArgs += "-DCMAKE_PREFIX_PATH=$QtPrefix"
}

if ($ToolchainFile) {
    $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$ToolchainFile"
}

if ($Generator) {
    $cmakeArgs = @("-G", $Generator) + $cmakeArgs
}

Invoke-CheckedCommand "cmake" $cmakeArgs
Invoke-CheckedCommand "cmake" @("--build", $BuildDir, "--config", "Release", "--parallel")
Invoke-CheckedCommand "cmake" @("--build", $BuildDir, "--config", "Release", "--target", "package")

Write-Host "Windows packages written under $BuildDir/"
