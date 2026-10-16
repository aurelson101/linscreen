param(
    [string]$BuildDir = "build-linscreen-windows",
    [string]$QtPrefix = $env:Qt6_DIR,
    [string]$ToolchainFile = "",
    [string]$WindeployQt = "",
    [string]$Generator = "",
    [string]$SigningCertificatePath = $env:WINDOWS_SIGNING_CERTIFICATE_PATH,
    [string]$SigningCertificatePassword = $env:WINDOWS_SIGNING_CERTIFICATE_PASSWORD,
    [string]$TimestampUrl = "http://timestamp.digicert.com",
    [switch]$RequireSigning,
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

function Resolve-SignTool {
    $resolved = Get-Command signtool.exe -ErrorAction SilentlyContinue
    if ($resolved) {
        return $resolved.Source
    }

    $kitsRoot = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin"
    $candidate = Get-ChildItem -Path "$kitsRoot\*\x64\signtool.exe" -ErrorAction SilentlyContinue |
        Sort-Object FullName -Descending |
        Select-Object -First 1
    if (-not $candidate) {
        throw "signtool.exe is required for Authenticode signing. Install the Windows SDK."
    }

    return $candidate.FullName
}

$signingEnabled = -not [string]::IsNullOrWhiteSpace($SigningCertificatePath)
if ($RequireSigning -and -not $signingEnabled) {
    throw "A code-signing certificate is required for this release build."
}

$signTool = $null
if ($signingEnabled) {
    if (-not $runningOnWindows) {
        throw "Authenticode signing must run on Windows."
    }
    if (-not (Test-Path -LiteralPath $SigningCertificatePath -PathType Leaf)) {
        throw "The code-signing certificate file was not found."
    }
    $signTool = Resolve-SignTool
}

function Invoke-SignFile {
    param([string]$Path)

    $signArgs = @(
        "sign", "/fd", "SHA256", "/td", "SHA256", "/tr", $TimestampUrl,
        "/f", $SigningCertificatePath
    )
    if (-not [string]::IsNullOrEmpty($SigningCertificatePassword)) {
        $signArgs += @("/p", $SigningCertificatePassword)
    }
    $signArgs += $Path

    Invoke-CheckedCommand $signTool $signArgs
    Invoke-CheckedCommand $signTool @("verify", "/pa", "/all", "/v", $Path)
}

$cmakeArgs = @(
    "-S", ".",
    "-B", $BuildDir,
    "-DCMAKE_BUILD_TYPE=Release",
    "-DBUILD_TESTING=OFF",
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

if ($signingEnabled) {
    $executables = Get-ChildItem -Path (Join-Path $BuildDir "src") -Recurse -File -Filter "*.exe" |
        Where-Object { $_.Name -in @("linscreen.exe", "linscreen-cli.exe") }
    if (-not $executables) {
        throw "No LinScreen executable was found to sign."
    }
    foreach ($executable in $executables) {
        Invoke-SignFile $executable.FullName
    }
} else {
    Write-Warning "Unsigned development build: Windows may display a SmartScreen warning."
}

Invoke-CheckedCommand "cmake" @("--build", $BuildDir, "--config", "Release", "--target", "package")

if ($signingEnabled) {
    $installers = Get-ChildItem -Path $BuildDir -File -Filter "*.msi"
    if (-not $installers) {
        throw "No MSI installer was found to sign."
    }
    foreach ($installer in $installers) {
        Invoke-SignFile $installer.FullName
    }
}

Write-Host "Windows packages written under $BuildDir/"
