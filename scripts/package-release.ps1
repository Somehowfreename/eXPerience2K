[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$installer = Join-Path $repoRoot 'dist\eXPerience2K-v3.1.0-Setup.exe'
$releaseDir = Join-Path $repoRoot 'release\v3.1.0'

if (-not (Test-Path -LiteralPath $installer -PathType Leaf)) {
    throw "Release installer is missing: $installer"
}

New-Item -ItemType Directory -Path $releaseDir -Force | Out-Null
Copy-Item -LiteralPath $installer -Destination $releaseDir -Force

$hash = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash
$checksum = Join-Path $releaseDir 'SHA256SUMS.txt'
[System.IO.File]::WriteAllText(
    $checksum,
    "$hash  eXPerience2K-v3.1.0-Setup.exe`r`n",
    [System.Text.UTF8Encoding]::new($false)
)

Write-Host "Release installer staged: $releaseDir"
Write-Host "SHA-256: $hash"
Write-Host 'GitHub release policy: attach only the installer; publish the hash in the release body.'
