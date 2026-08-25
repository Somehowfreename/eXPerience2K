[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$coreX86 = Join-Path $repoRoot 'build\eXPerience2KCore-x86.exe'
$coreX64 = Join-Path $repoRoot 'build\eXPerience2KCore-x64.exe'
$configApp = Join-Path $repoRoot 'build\eXPerience2K.exe'
$explorerBandX86 = Join-Path $repoRoot 'build\eXPerience2KExplorerBand32.dll'
$explorerBandX64 = Join-Path $repoRoot 'build\eXPerience2KExplorerBand64.dll'
$installer = Join-Path $repoRoot 'dist\eXPerience2K-v3.0.0-Setup.exe'
$nsisSource = Join-Path $repoRoot 'installer\eXPerience2K.nsi'
$configSource = Join-Path $repoRoot 'src\eXPerience2KConfig.c'
$coreSource = Join-Path $repoRoot 'src\eXPerience2KCore.c'
$profilesSource = Join-Path $repoRoot 'payload\profiles.tsv'
$buildSource = Join-Path $repoRoot 'scripts\build.ps1'

foreach ($required in @($coreX86, $coreX64, $configApp, $explorerBandX86, $explorerBandX64, $installer)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required build output is missing: $required"
    }
}

function Assert-PeCompatibility {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][int]$ExpectedMagic,
        [Parameter(Mandatory = $true)][int]$ExpectedMinor
    )

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    $peOffset = [BitConverter]::ToInt32($bytes, 0x3c)
    $optionalHeader = $peOffset + 24
    $magic = [BitConverter]::ToUInt16($bytes, $optionalHeader)
    $osMajor = [BitConverter]::ToUInt16($bytes, $optionalHeader + 40)
    $osMinor = [BitConverter]::ToUInt16($bytes, $optionalHeader + 42)
    $subsystemMajor = [BitConverter]::ToUInt16($bytes, $optionalHeader + 48)
    $subsystemMinor = [BitConverter]::ToUInt16($bytes, $optionalHeader + 50)
    if ($magic -ne $ExpectedMagic -or $osMajor -ne 5 -or
        $osMinor -ne $ExpectedMinor -or $subsystemMajor -ne 5 -or
        $subsystemMinor -ne $ExpectedMinor) {
        throw "Unexpected PE compatibility fields in $Path."
    }
}

Assert-PeCompatibility -Path $coreX86 -ExpectedMagic 0x10b -ExpectedMinor 1
Assert-PeCompatibility -Path $coreX64 -ExpectedMagic 0x20b -ExpectedMinor 2
Assert-PeCompatibility -Path $configApp -ExpectedMagic 0x10b -ExpectedMinor 1
Assert-PeCompatibility -Path $explorerBandX86 -ExpectedMagic 0x10b -ExpectedMinor 1
Assert-PeCompatibility -Path $explorerBandX64 -ExpectedMagic 0x20b -ExpectedMinor 2

$operationCount = (Import-Csv -LiteralPath (Join-Path $repoRoot 'payload\operations.tsv') -Delimiter "`t").Count
$targetCount = (Import-Csv -LiteralPath (Join-Path $repoRoot 'payload\targets.tsv') -Delimiter "`t").Count
$resourceCount = (Get-ChildItem -LiteralPath (Join-Path $repoRoot 'payload\Resources\eXPerience2K') -Recurse -File).Count
$features = @(Import-Csv -LiteralPath (Join-Path $repoRoot 'payload\features.tsv') -Delimiter "`t")

if ($operationCount -ne 672) { throw "Expected 672 resource operations; found $operationCount." }
if ($targetCount -ne 147) { throw "Expected 147 manifest targets; found $targetCount." }
if ($resourceCount -ne 671) { throw "Expected 671 resource files; found $resourceCount." }
if ($features.Count -ne 11) { throw "Expected 11 configuration features; found $($features.Count)." }

$expectedDefaults = @{
    resource_conversion = 'on'
    classic_theme = 'on'
    classic_start_menu = 'on'
    classic_control_panel = 'on'
    start_menu_slide = 'on'
    start_menu_fade = 'off'
    classic_logon = 'on'
    install_wallpapers = 'on'
    classic_explorer = 'on'
    windows_2000_sounds = 'on'
    windows_2000_double_click_sound = 'on'
}
foreach ($feature in $features) {
    if (-not $expectedDefaults.ContainsKey($feature.feature_id)) {
        throw "Unexpected feature: $($feature.feature_id)"
    }
    if ($feature.first_launch_default -cne $expectedDefaults[$feature.feature_id]) {
        throw "Unexpected default for $($feature.feature_id)."
    }
}

$nsisText = [System.IO.File]::ReadAllText($nsisSource)
$configText = [System.IO.File]::ReadAllText($configSource)
$coreText = [System.IO.File]::ReadAllText($coreSource)
$profilesText = [System.IO.File]::ReadAllText($profilesSource)
$buildText = [System.IO.File]::ReadAllText($buildSource)
$installerSupportedFragment = 'Windows XP Professional x86 Service Pack 3 and Windows XP Professional x64 Edition Service Pack 2'
if (-not $nsisText.Contains('Function .onInit') -or
    -not $nsisText.Contains('${If} ${RunningX64}') -or
    -not $nsisText.Contains($installerSupportedFragment)) {
    throw 'The dual XP Professional x86/x64 installer gate is missing.'
}
foreach ($strictGateContract in @(
    'ReadEnvStr $0 "PROCESSOR_ARCHITEW6432"',
    'StrCmp $0 "AMD64" x64_architecture_ok unsupported_os',
    'ReadEnvStr $0 "PROCESSOR_ARCHITECTURE"',
    'StrCmp $0 "x86" x86_architecture_ok unsupported_os',
    '"SYSTEM\CurrentControlSet\Control\ProductOptions" "ProductType"',
    'StrCmp $0 "WinNT" x64_workstation_ok unsupported_os_native_view',
    'StrCmp $0 "WinNT" x86_workstation_ok unsupported_os',
    '"SOFTWARE\Microsoft\Windows NT\CurrentVersion" "CurrentVersion"',
    'StrCmp $0 "5.2" x64_version_ok unsupported_os_native_view',
    'StrCmp $0 "5.1" x86_version_ok unsupported_os',
    '"SOFTWARE\Microsoft\Windows NT\CurrentVersion" "CurrentBuildNumber"',
    'StrCmp $0 "3790" x64_build_ok unsupported_os_native_view',
    'StrCmp $0 "2600" x86_build_ok unsupported_os',
    '"SYSTEM\CurrentControlSet\Control\Windows" "CSDVersion"',
    'IntCmp $0 0x200 supported_os unsupported_os_native_view unsupported_os_native_view',
    'IntCmp $0 0x300 x86_suite_check unsupported_os unsupported_os',
    '"SYSTEM\CurrentControlSet\Control\ProductOptions" "ProductSuite"',
    '"SYSTEM\WPA\MediaCenter" "Installed"',
    '"SYSTEM\WPA\TabletPC" "Installed"',
    'https://github.com/Somehowfreename/eXPerience2K',
    'No files or settings have been changed.'
)) {
    if (-not $nsisText.Contains($strictGateContract)) {
        throw "The strict dual-profile gate is missing: $strictGateContract"
    }
}
foreach ($architectureContract in @(
    'PROCESSOR_ARCHITECTURE_INTEL',
    'PROCESSOR_ARCHITECTURE_AMD64',
    'eXPerience2KCore-x86.exe',
    'eXPerience2KCore-x64.exe',
    'eXPerience2KExplorerBand32.dll',
    'eXPerience2KExplorerBand64.dll'
)) {
    if (-not $configText.Contains($architectureContract)) {
        throw "The configuration application's dual-architecture contract is missing: $architectureContract"
    }
}
foreach ($x86ComExportContract in @(
    '--kill-at',
    'exact undecorated COM exports'
)) {
    if (-not $buildText.Contains($x86ComExportContract)) {
        throw "The x86 Explorer COM-export contract is missing: $x86ComExportContract"
    }
}
foreach ($revertContract in @(
    '#define IDC_REVERT 2011',
    'Revert all eXPerience2K changes?',
    'MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2',
    'restore_all_managed_features(0)',
    'restore_all_managed_features(1)',
    'Complete Revert restoration succeeded; the immutable baseline was retained.',
    'write_user_dword(CONFIG_KEY, "ResourceConversionEnabled", 0)'
)) {
    if (-not $configText.Contains($revertContract)) {
        throw "The Revert-button contract is missing: $revertContract"
    }
}
if (-not $configText.Contains('Cabinet and toolbar blobs are initial defaults') -or
    $configText.Contains('user_binary_matches_asset')) {
    throw 'Explorer enablement detection still depends on mutable window-layout blobs.'
}
foreach ($windowContract in @(
    'WS_OVERLAPPEDWINDOW | WS_VSCROLL',
    'case WM_GETMINMAXINFO:',
    'case WM_SIZE:',
    'case WM_VSCROLL:',
    'case WM_MOUSEWHEEL:',
    'ShowScrollBar(window, SB_VERT, g_scrollbar_visible)',
    'layout_main_controls(window)',
    'scroll_main_controls(window'
)) {
    if (-not $configText.Contains($windowContract)) {
        throw "The resizable-window contract is missing: $windowContract"
    }
}
foreach ($packagedBinary in @(
    'File "..\build\eXPerience2KCore-x86.exe"',
    'File "..\build\eXPerience2KCore-x64.exe"',
    'File "..\build\eXPerience2KExplorerBand32.dll"',
    'File "..\build\eXPerience2KExplorerBand64.dll"'
)) {
    if (-not $nsisText.Contains($packagedBinary)) {
        throw "Required dual-architecture binary is not packaged: $packagedBinary"
    }
}

foreach ($brandingContract in @(
    'original-windows-2002',
    'Windows 2002 Professional'
)) {
    if (-not $coreText.Contains($brandingContract) -or
        -not $profilesText.Contains($brandingContract)) {
        throw "The shared original Windows 2002 branding contract is missing: $brandingContract"
    }
}
foreach ($obsoleteBrandingContract in @(
    'xp-client-2001',
    'Windows 2001 Professional',
    'xp-x64-2005',
    'Windows 2005 Professional'
)) {
    if ($coreText.Contains($obsoleteBrandingContract) -or
        $profilesText.Contains($obsoleteBrandingContract)) {
        throw "An obsolete year-specific branding profile is still selectable: $obsoleteBrandingContract"
    }
}

& (Join-Path $PSScriptRoot 'use-original-2002-branding.ps1') `
    -RepositoryRoot $repoRoot -VerifyOnly
& (Join-Path $PSScriptRoot 'verify-original-icon-assets.ps1') `
    -RepositoryRoot $repoRoot
& (Join-Path $PSScriptRoot 'verify-wallpaper-assets.ps1') `
    -RepositoryRoot $repoRoot
& (Join-Path $PSScriptRoot 'verify-sound-assets.ps1') `
    -RepositoryRoot $repoRoot
& (Join-Path $PSScriptRoot 'verify-explorer-experiment.ps1') `
    -RepositoryRoot $repoRoot

$version = (Get-Item -LiteralPath $installer).VersionInfo
if ($version.ProductVersion -ne '3.0.0.0' -or
    $version.FileVersion -ne '3.0.0.0' -or
    $version.FileDescription -ne 'Windows 2000-style conversion for Windows XP Professional') {
    throw 'Installer version metadata does not match the dual-platform 3.0.0 release contract.'
}

$legacyMarker = -join (105,110,101,120,112,101,114,105,101,110,99,101 | ForEach-Object { [char]$_ })
$allowedReadme = [System.IO.Path]::GetFullPath((Join-Path $repoRoot 'README.md'))
foreach ($file in (Get-ChildItem -LiteralPath $repoRoot -Recurse -File -Force | Where-Object {
    $_.FullName -notlike "$repoRoot\.git*" -and
    $_.FullName -notlike "$repoRoot\build*" -and
    $_.FullName -notlike "$repoRoot\dist*" -and
    $_.FullName -notlike "$repoRoot\release*"
})) {
    if ([System.IO.Path]::GetFullPath($file.FullName) -eq $allowedReadme) { continue }
    $bytes = [System.IO.File]::ReadAllBytes($file.FullName)
    $ascii = [System.Text.Encoding]::ASCII.GetString($bytes).ToLowerInvariant()
    $unicode = [System.Text.Encoding]::Unicode.GetString($bytes).ToLowerInvariant()
    if ($ascii.Contains($legacyMarker) -or $unicode.Contains($legacyMarker)) {
        throw "Legacy project name found outside the README inspiration section: $($file.FullName)"
    }
}

$hash = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash
Write-Host "Verified $operationCount operations, $targetCount targets, $resourceCount resources, and $($features.Count) features."
Write-Host 'Verified NT 5.1/5.2 PE compatibility, strict XP Professional x86 SP3/x64 SP2 installer gates, dual-architecture packaging, and all asset manifests.'
Write-Host "Installer SHA-256: $hash"
