[CmdletBinding()]
param(
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = 'Stop'
$wallpaperDirectory = Join-Path $RepositoryRoot 'payload\Assets\Wallpapers'
$expected = [ordered]@{
    'Windows_2000_1.jpg' = '13BB27DBB1ED32067ED2EE97EBA23217147CB6739F72218532F91FCD937E8836'
    'Windows_2000_2.jpg' = '0D54BCF27108619FE18E533CFF8F0B324B835647D43E5D7F1A5EC8665A46E54D'
    'Windows_9x.jpg' = 'D7747BEE8B66E65130A8164EA5DC71E000C82EB26C1B8C75F3EE9F2626A8220B'
    'Windows_2000_3.jpg' = 'B18816E2B1710B7BC6BB379ED557DA6743CBB84C3ECD98D9A6E8485A50104883'
    'Windows_2000_4.jpg' = '36BA6B49F1B961C9A2AD641BA632BD487B441EA38BDAE658672D75D992CA762C'
}

if (-not (Test-Path -LiteralPath $wallpaperDirectory -PathType Container)) {
    throw "Wallpaper payload directory is missing: $wallpaperDirectory"
}

$actualFiles = @(Get-ChildItem -LiteralPath $wallpaperDirectory -File)
if ($actualFiles.Count -ne $expected.Count) {
    throw "Expected $($expected.Count) wallpaper payload files, found $($actualFiles.Count)."
}

foreach ($entry in $expected.GetEnumerator()) {
    $path = Join-Path $wallpaperDirectory $entry.Key
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Required wallpaper is missing: $($entry.Key)"
    }
    $actualHash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
    if ($actualHash -ne $entry.Value) {
        throw "Wallpaper hash mismatch: $($entry.Key)"
    }
}

Add-Type -AssemblyName System.Drawing
$solidWallpaperPath = Join-Path $wallpaperDirectory 'Windows_2000_4.jpg'
$solidWallpaper = [System.Drawing.Bitmap]::FromFile($solidWallpaperPath)
try {
    if ($solidWallpaper.Width -ne 1920 -or $solidWallpaper.Height -ne 1080) {
        throw 'Windows_2000_4.jpg must be exactly 1920x1080.'
    }
    $sample = $solidWallpaper.GetPixel(960, 540)
    if ($sample.R -ne 58 -or $sample.G -ne 110 -or $sample.B -ne 165) {
        throw "Windows_2000_4.jpg does not decode to Windows 2000 desktop blue (#3A6EA5)."
    }
} finally {
    $solidWallpaper.Dispose()
}

Write-Host "Verified $($expected.Count) exact wallpaper payload files."
