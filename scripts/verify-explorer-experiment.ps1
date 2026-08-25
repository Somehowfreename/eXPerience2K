[CmdletBinding()]
param(
    [string]$RepositoryRoot
)

$ErrorActionPreference = 'Stop'
if (-not $RepositoryRoot) {
    $RepositoryRoot = Split-Path -Parent $PSScriptRoot
}
$RepositoryRoot = [System.IO.Path]::GetFullPath($RepositoryRoot)
$webRoot = Join-Path $RepositoryRoot 'payload\ExplorerWeb'
$manifestPath = Join-Path $RepositoryRoot 'payload\ExplorerWeb.sha256.tsv'
$stateRoot = Join-Path $RepositoryRoot 'payload\ExplorerState'
$configSource = Join-Path $RepositoryRoot 'src\eXPerience2KConfig.c'
$bandSource = Join-Path $RepositoryRoot 'src\eXPerience2KExplorerBand.cpp'
$coreSource = Join-Path $RepositoryRoot 'src\eXPerience2KCore.c'

foreach ($required in @($webRoot, $manifestPath, $stateRoot, $configSource, $bandSource, $coreSource)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Explorer experiment input is missing: $required"
    }
}

$core = [System.IO.File]::ReadAllText($coreSource)
foreach ($requiredText in @(
    'cleanup-explorer-registrations',
    'delete_registry_tree_native',
    'Native Explorer registration cleanup complete.'
)) {
    if (-not $core.Contains($requiredText)) {
        throw "Native Explorer cleanup marker is missing: $requiredText"
    }
}

$manifest = @(Import-Csv -LiteralPath $manifestPath -Delimiter "`t")
if ($manifest.Count -ne 74) {
    throw "Expected 74 exact Windows 2000 Web files; found $($manifest.Count)."
}

$manifestNames = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::OrdinalIgnoreCase
)
foreach ($entry in $manifest) {
    if (-not $manifestNames.Add($entry.relative_path)) {
        throw "Duplicate Explorer Web payload entry: $($entry.relative_path)"
    }
    $relative = $entry.relative_path.Replace('/', [System.IO.Path]::DirectorySeparatorChar)
    $path = Join-Path $webRoot $relative
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Explorer Web payload file is missing: $($entry.relative_path)"
    }
    $item = Get-Item -LiteralPath $path
    if ($item.Length -ne [long]$entry.length) {
        throw "Explorer Web payload length mismatch: $($entry.relative_path)"
    }
    $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
    if ($hash -cne $entry.sha256) {
        throw "Explorer Web payload hash mismatch: $($entry.relative_path)"
    }
}

$payloadFiles = @(Get-ChildItem -LiteralPath $webRoot -Recurse -File)
if ($payloadFiles.Count -ne $manifest.Count) {
    throw "Explorer Web payload contains unmanifested files ($($payloadFiles.Count) files, $($manifest.Count) entries)."
}
foreach ($file in $payloadFiles) {
    $relative = $file.FullName.Substring($webRoot.Length + 1).Replace('\', '/')
    if (-not $manifestNames.Contains($relative)) {
        throw "Unmanifested Explorer Web payload file: $relative"
    }
}

$expectedState = [ordered]@{
    'CabinetSettings.bin' = @('12',  'A7FB09581BF1CF6FED5FC7416F7C37E510DEB83657B22C718325529D973792EB')
    'CabView.bin'         = @('92',  'A538DFC70119340D904BE7F80EA03F88A5DBED60A94E98731D3DD37CF2AF5AF8')
    'ITBarLayout.bin'     = @('564', '45053621CB2F03790B339FE74EFF7ABA67A06776A6AC33F7D5BD61E3D5122D0B')
    'SaveLinksOrder.bin'  = @('4',   '67ABDD721024F0FF4E0B3F4C2FC13BC5BAD42D0B7851D456D88D203D15AAA450')
    'ShellBrowserBand.bin'= @('32',  '4CDEDAAD7D8B966F11A3C4E791F97DA22BCB91958E22D39960DBBE73F5DCF8EC')
    # Exact Windows 2000 ShellState with only SSF_WEBVIEW cleared. XP's own
    # Common Tasks renderer must be off while the native Explorer desk band supplies
    # the Windows 2000 pane.
    'ShellState.bin'      = @('36',  'E8736B2E909CD4F712735EEC8E96EB447C8DD891D1E6FB5D411CE8F2A1EFD61D')
    'ViewView2-0.bin'     = @('48',  '5048F844F10678E43B5AE421586163D2B862EF5DCE65D2F2EC3C337141158604')
    'ViewView2-1.bin'     = @('48',  '5048F844F10678E43B5AE421586163D2B862EF5DCE65D2F2EC3C337141158604')
}

$stateFiles = @(Get-ChildItem -LiteralPath $stateRoot -File)
if ($stateFiles.Count -ne $expectedState.Count) {
    throw "Expected $($expectedState.Count) exact Explorer registry-state assets; found $($stateFiles.Count)."
}
foreach ($name in $expectedState.Keys) {
    $path = Join-Path $stateRoot $name
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Explorer registry-state asset is missing: $name"
    }
    $item = Get-Item -LiteralPath $path
    if ($item.Length -ne [long]$expectedState[$name][0]) {
        throw "Explorer registry-state asset length mismatch: $name"
    }
    $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
    if ($hash -cne $expectedState[$name][1]) {
        throw "Explorer registry-state asset hash mismatch: $name"
    }
}

$source = [System.IO.File]::ReadAllText($configSource)
foreach ($requiredText in @(
    '#define MAX_FEATURES 11',
    'Windows 2000 Explorer folder interface (experimental)',
    'explorer_experiment_architecture_supported',
    'PROCESSOR_ARCHITECTURE_INTEL',
    'PROCESSOR_ARCHITECTURE_AMD64',
    'eXPerience2KExplorerBand32.dll',
    'eXPerience2KExplorerBand64.dll',
    '@EXPLORERBAND@',
    'delete_machine_tree_view(subkey, KEY_WOW64_64KEY)',
    'delete_machine_tree_view(subkey, KEY_WOW64_32KEY)',
    'apply_w2k_explorer_user_state',
    'apply_w2k_explorer_machine_state',
    'ExplorerAllFoldersIconMode',
    'NoSimpleStartMenu',
    'ClassicStartPolicyManaged',
    '{"ActiveTitle", "10 36 106"',
    '{"GradientActiveTitle", "166 202 240"',
    'ExplorerExperimentBackup',
    '/apply-explorer-experiment',
    '/apply-all-defaults-unattended',
    '/restore-explorer-experiment'
)) {
    if (-not $source.Contains($requiredText)) {
        throw "Explorer experiment implementation marker is missing: $requiredText"
    }
}

$band = [System.IO.File]::ReadAllText($bandSource)
foreach ($requiredText in @(
    'CLSID_E2KExplorerBand',
    'CLSID_E2KExplorerHook',
    'public IDeskBand',
    'AttachInline',
    'SHELLDLL_DefView',
    'A BHO inserts a 200-pixel information pane',
    'initialize_shell_view_mode',
    'SetCurrentViewMode(FVM_ICON)',
    'wvleft.bmp'
)) {
    if (-not $band.Contains($requiredText)) {
        throw "Native Explorer pane implementation marker is missing: $requiredText"
    }
}

$referenceRoot = Join-Path $RepositoryRoot 'analysis\w2k-2011-explorer-reference\web-folder'
if (Test-Path -LiteralPath $referenceRoot -PathType Container) {
    $referenceFiles = @(Get-ChildItem -LiteralPath $referenceRoot -Recurse -File)
    if ($referenceFiles.Count -ne $manifest.Count) {
        throw "Extracted Windows 2000 reference Web tree contains $($referenceFiles.Count) files; expected $($manifest.Count)."
    }
    foreach ($entry in $manifest) {
        $relative = $entry.relative_path.Replace('/', [System.IO.Path]::DirectorySeparatorChar)
        $path = Join-Path $referenceRoot $relative
        if (-not (Test-Path -LiteralPath $path -PathType Leaf) -or
            (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -cne $entry.sha256) {
            throw "Packaged Explorer Web file does not match the extracted Windows 2000 reference: $($entry.relative_path)"
        }
    }
}

Write-Host "Verified $($manifest.Count) exact Windows 2000 Web files, $($expectedState.Count) exact Explorer-state assets, the one-bit XP inline-pane ShellState delta, and the native x86/x64 architecture selection boundary."
