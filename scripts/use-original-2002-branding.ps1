param(
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot),
    [switch]$VerifyOnly
)

$ErrorActionPreference = 'Stop'

$referenceRoot = Join-Path $RepositoryRoot 'reference-assets\windows-2002'
$destinationRoot = Join-Path $RepositoryRoot 'payload\Resources\eXPerience2K'
$assets = @(
    'explorer\158.bmp',
    'explorer\163.bmp',
    'explorer\164.bmp',
    'explorer\166.bmp',
    'explorer\167.bmp',
    'logon\201.bmp',
    'logon\204.bmp',
    'msgina\101.bmp',
    'msgina\107.bmp',
    'msgina\111.bmp',
    'msgina\113.bmp',
    'msgina\125.bmp',
    'msgina\126.bmp',
    'msgina\128.bmp',
    'msgina\129.bmp',
    'shell32\130.bmp',
    'shell32\131.bmp',
    'shell32\146.bmp',
    'shell32\147.bmp'
)

foreach ($relativePath in $assets) {
    $source = Join-Path $referenceRoot $relativePath
    $destination = Join-Path $destinationRoot $relativePath
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "Missing original branding asset: $relativePath"
    }
    if (-not $VerifyOnly) {
        [IO.Directory]::CreateDirectory((Split-Path -Parent $destination)) | Out-Null
        [IO.File]::Copy($source, $destination, $true)
    }
    if (-not (Test-Path -LiteralPath $destination -PathType Leaf)) {
        throw "Missing packaged original branding asset: $relativePath"
    }
    $sourceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $source).Hash
    $destinationHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $destination).Hash
    if ($sourceHash -ne $destinationHash) {
        throw "Branding asset is not byte-identical after copying: $relativePath"
    }
}

if ($VerifyOnly) {
    Write-Host "Verified $($assets.Count) byte-identical Windows 2002 branding assets."
} else {
    Write-Host "Restored and verified $($assets.Count) byte-identical Windows 2002 branding assets."
}
