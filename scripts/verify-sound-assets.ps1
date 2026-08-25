[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$RepositoryRoot
)

$ErrorActionPreference = 'Stop'
$manifestPath = Join-Path $RepositoryRoot 'payload\Sounds\Windows2000.sha256.tsv'
$soundRoot = Join-Path $RepositoryRoot 'payload\Sounds\Windows2000'
$configSource = Join-Path $RepositoryRoot 'src\eXPerience2KConfig.c'
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    throw "Windows 2000 sound manifest is missing: $manifestPath"
}
if (-not (Test-Path -LiteralPath $soundRoot -PathType Container)) {
    throw "Windows 2000 sound directory is missing: $soundRoot"
}

$manifest = @(Import-Csv -LiteralPath $manifestPath -Delimiter "`t")
if ($manifest.Count -ne 8) {
    throw "Expected 8 authoritative Windows 2000 sound assets; found $($manifest.Count)."
}
$expectedNames = @($manifest.file | Sort-Object)
$actualFiles = @(Get-ChildItem -LiteralPath $soundRoot -File)
$actualNames = @($actualFiles.Name | Sort-Object)
if (($expectedNames -join "`n") -cne ($actualNames -join "`n")) {
    throw 'The Windows 2000 sound payload does not exactly match its manifest.'
}

foreach ($entry in $manifest) {
    $path = Join-Path $soundRoot $entry.file
    $item = Get-Item -LiteralPath $path
    if ($item.Length -ne [long]$entry.length) {
        throw "Unexpected length for $($entry.file): $($item.Length)."
    }
    $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
    if ($hash -cne $entry.sha256) {
        throw "Unexpected SHA-256 for $($entry.file): $hash."
    }
    $stream = [System.IO.File]::OpenRead($path)
    try {
        $header = New-Object byte[] 12
        if ($stream.Read($header, 0, $header.Length) -ne $header.Length -or
            [System.Text.Encoding]::ASCII.GetString($header, 0, 4) -cne 'RIFF' -or
            [System.Text.Encoding]::ASCII.GetString($header, 8, 4) -cne 'WAVE') {
            throw "The expanded asset is not a valid RIFF/WAVE file: $($entry.file)"
        }
    }
    finally {
        $stream.Dispose()
    }
}

$startEntry = $manifest | Where-Object { $_.file -ceq 'start.wav' }
if (-not $startEntry -or
    $startEntry.iso_source -cne 'I386/START.WA_' -or
    $startEntry.sha256 -cne 'EFF8F5A74D7499B0991FF42E7963927AD37A17D63EF13EE784B849CAE2808ADA') {
    throw 'The authoritative Windows 2000 folder double-click sound manifest entry is missing or altered.'
}

if (-not (Test-Path -LiteralPath $configSource -PathType Leaf)) {
    throw "Configuration source is missing: $configSource"
}
$source = [System.IO.File]::ReadAllText($configSource)
foreach ($requiredText in @(
    'Enable the Windows 2000 folder double-click sound',
    '"Sound_Navigating", "Explorer", "Navigating", "start.wav"',
    'apply_windows_2000_double_click_sound',
    'FEATURE_WINDOWS_2000_DOUBLE_CLICK_SOUND'
)) {
    if (-not $source.Contains($requiredText)) {
        throw "Windows 2000 folder double-click sound implementation marker is missing: $requiredText"
    }
}

Write-Host 'Verified 8 exact Windows 2000 sound assets extracted from W2K-2011.iso.'
