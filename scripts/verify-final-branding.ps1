param(
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = 'Stop'

& (Join-Path $PSScriptRoot 'use-original-2002-branding.ps1') `
    -RepositoryRoot $RepositoryRoot `
    -VerifyOnly

$brandingRoot = Join-Path $RepositoryRoot 'payload\Resources\eXPerience2K'
$expectedStrings = @(
    @{ Path = 'sysdm\180.txt'; Expected = 'Microsoft Windows 2002' },
    @{ Path = 'sysdm\195.txt'; Expected = 'Version 2002' }
)

foreach ($definition in $expectedStrings) {
    $path = Join-Path $brandingRoot $definition.Path
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing System Properties branding string: $path"
    }
    $actual = (Get-Content -LiteralPath $path -Raw).Trim()
    if ($actual -cne $definition.Expected) {
        throw "Unexpected System Properties branding in $($definition.Path): '$actual'"
    }
}

Write-Host 'Verified the 19 byte-identical original Windows 2002 bitmaps and both Windows 2002 System Properties strings.'
