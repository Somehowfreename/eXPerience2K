[CmdletBinding()]
param(
    [string]$GccX86Path,
    [string]$GccX64Path,
    [string]$MakeNsisPath
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot 'build'
$distDir = Join-Path $repoRoot 'dist'
$releaseDir = Join-Path $repoRoot 'release\v2.4.1'

Write-Host 'Restoring the exact original Windows 2002 branding artwork...'
& (Join-Path $PSScriptRoot 'use-original-2002-branding.ps1') `
    -RepositoryRoot $repoRoot
& (Join-Path $PSScriptRoot 'verify-final-branding.ps1') `
    -RepositoryRoot $repoRoot
Write-Host 'Verifying the complete original icon corpus...'
& (Join-Path $PSScriptRoot 'verify-original-icon-assets.ps1') `
    -RepositoryRoot $repoRoot
Write-Host 'Verifying the exact wallpaper payload...'
& (Join-Path $PSScriptRoot 'verify-wallpaper-assets.ps1') `
    -RepositoryRoot $repoRoot
Write-Host 'Verifying the exact Windows 2000 sound payload...'
& (Join-Path $PSScriptRoot 'verify-sound-assets.ps1') `
    -RepositoryRoot $repoRoot
Write-Host 'Verifying the isolated Explorer experiment payload and source...'
& (Join-Path $PSScriptRoot 'verify-explorer-experiment.ps1') `
    -RepositoryRoot $repoRoot

function Resolve-BuildTool {
    param(
        [string]$ProvidedPath,
        [string]$CommandName,
        [string]$FriendlyName
    )

    if ($ProvidedPath) {
        if (-not (Test-Path -LiteralPath $ProvidedPath -PathType Leaf)) {
            throw "$FriendlyName was not found at: $ProvidedPath"
        }
        return (Resolve-Path -LiteralPath $ProvidedPath).Path
    }

    $command = Get-Command $CommandName -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    throw "$FriendlyName was not found. Add it to PATH or pass its full path to this script."
}

$gccX86 = Resolve-BuildTool -ProvidedPath $GccX86Path -CommandName 'i686-w64-mingw32-gcc.exe' -FriendlyName 'MinGW-w64 x86 GCC'
$gccX64 = Resolve-BuildTool -ProvidedPath $GccX64Path -CommandName 'x86_64-w64-mingw32-gcc.exe' -FriendlyName 'MinGW-w64 x64 GCC'
$gppX64 = [System.Text.RegularExpressions.Regex]::Replace($gccX64, 'gcc\.exe$', 'g++.exe', 'IgnoreCase')
if (-not (Test-Path -LiteralPath $gppX64 -PathType Leaf)) {
    throw "MinGW-w64 x64 G++ was not found beside GCC: $gppX64"
}
$makeNsis = Resolve-BuildTool -ProvidedPath $MakeNsisPath -CommandName 'makensis.exe' -FriendlyName 'NSIS makensis'

New-Item -ItemType Directory -Path $buildDir, $distDir, $releaseDir -Force | Out-Null

function Build-Core {
    param(
        [string]$Compiler,
        [string]$Architecture,
        [string]$MinimumNtVersion,
        [string]$LinkerVersion,
        [string]$OutputName
    )

    $output = Join-Path $buildDir $OutputName
    $arguments = @(
        '-std=c99'
        '-Os'
        '-Wall'
        '-Wextra'
        '-Werror'
        "-D_WIN32_WINNT=$MinimumNtVersion"
        '-static'
        '-s'
        $LinkerVersion
        '-o'
        $output
        (Join-Path $repoRoot 'src\eXPerience2KCore.c')
        '-ladvapi32'
    )

    $savedPath = $env:PATH
    try {
        $env:PATH = "$(Split-Path -Parent $Compiler);$env:PATH"
        Write-Host "Compiling the native $Architecture engine..."
        & $Compiler @arguments
        if ($LASTEXITCODE -ne 0) {
            throw "$Architecture GCC failed with exit code $LASTEXITCODE."
        }
    }
    finally {
        $env:PATH = $savedPath
    }
}

Build-Core -Compiler $gccX86 -Architecture 'x86' -MinimumNtVersion '0x0501' `
    -LinkerVersion '-Wl,--major-os-version,5,--minor-os-version,1,--major-subsystem-version,5,--minor-subsystem-version,1' `
    -OutputName 'eXPerience2KCore-x86.exe'
Build-Core -Compiler $gccX64 -Architecture 'x64' -MinimumNtVersion '0x0502' `
    -LinkerVersion '-Wl,--major-os-version,5,--minor-os-version,2,--major-subsystem-version,5,--minor-subsystem-version,2' `
    -OutputName 'eXPerience2KCore-x64.exe'

$configOutput = Join-Path $buildDir 'eXPerience2K.exe'
$configArguments = @(
    '-std=c99'
    '-Os'
    '-Wall'
    '-Wextra'
    '-Werror'
    '-D_WIN32_WINNT=0x0501'
    '-static'
    '-s'
    '-mwindows'
    '-Wl,--major-os-version,5,--minor-os-version,1,--major-subsystem-version,5,--minor-subsystem-version,1'
    '-o'
    $configOutput
    (Join-Path $repoRoot 'src\eXPerience2KConfig.c')
    '-ladvapi32'
    '-lcomctl32'
    '-lcomdlg32'
    '-lshell32'
    '-lshlwapi'
)
$savedPath = $env:PATH
try {
    $env:PATH = "$(Split-Path -Parent $gccX86);$env:PATH"
    Write-Host 'Compiling the universal x86 configuration application...'
    & $gccX86 @configArguments
    if ($LASTEXITCODE -ne 0) {
        throw "Configuration application GCC failed with exit code $LASTEXITCODE."
    }
}
finally {
    $env:PATH = $savedPath
}

$bandOutput = Join-Path $buildDir 'eXPerience2KExplorerBand64.dll'
$bandArguments = @(
    '-std=gnu++11'
    '-Os'
    '-Wall'
    '-Wextra'
    '-Werror'
    '-D_WIN32_WINNT=0x0502'
    '-D_WIN32_IE=0x0600'
    '-fno-exceptions'
    '-fno-rtti'
    '-fno-threadsafe-statics'
    '-shared'
    '-static'
    '-static-libgcc'
    '-static-libstdc++'
    '-s'
    '-Wl,--major-os-version,5,--minor-os-version,2,--major-subsystem-version,5,--minor-subsystem-version,2'
    '-o'
    $bandOutput
    (Join-Path $repoRoot 'src\eXPerience2KExplorerBand.cpp')
    '-lole32'
    '-loleaut32'
    '-luuid'
    '-lshell32'
    '-lshlwapi'
    '-lcomctl32'
    '-luser32'
    '-lgdi32'
)
$savedPath = $env:PATH
try {
    $env:PATH = "$(Split-Path -Parent $gppX64);$env:PATH"
    Write-Host 'Compiling the experimental native x64 Windows 2000 Explorer pane...'
    & $gppX64 @bandArguments
    if ($LASTEXITCODE -ne 0) {
        throw "Explorer pane G++ failed with exit code $LASTEXITCODE."
    }
}
finally {
    $env:PATH = $savedPath
}

$objdumpX64 = Join-Path (Split-Path -Parent $gccX64) 'objdump.exe'
if (-not (Test-Path -LiteralPath $objdumpX64 -PathType Leaf)) {
    throw "MinGW-w64 x64 objdump was not found beside GCC: $objdumpX64"
}
$bandPe = (& $objdumpX64 -p $bandOutput | Out-String)
foreach ($forbidden in @('GetTickCount64', 'libstdc++-6.dll', 'libgcc_s_seh-1.dll', 'libwinpthread-1.dll')) {
    if ($bandPe -match [regex]::Escape($forbidden)) {
        throw "The experimental x64 Explorer pane imports an XP-incompatible runtime/API: $forbidden"
    }
}
if ($bandPe -notmatch 'DllGetClassObject' -or $bandPe -notmatch 'DllCanUnloadNow') {
    throw 'The experimental x64 Explorer pane is missing required COM exports.'
}

Write-Host 'Building the installer...'
Push-Location $repoRoot
try {
    & $makeNsis 'installer\eXPerience2K.nsi'
    if ($LASTEXITCODE -ne 0) {
        throw "NSIS failed with exit code $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}

$installer = Join-Path $distDir 'eXPerience2K-v2.4.1-Setup.exe'
if (-not (Test-Path -LiteralPath $installer -PathType Leaf)) {
    throw "Expected installer was not produced: $installer"
}

Copy-Item -LiteralPath $installer -Destination $releaseDir -Force
Write-Host "Build complete: $installer"
