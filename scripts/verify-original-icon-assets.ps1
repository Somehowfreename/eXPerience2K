param(
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = 'Stop'

$iconRoot = (Resolve-Path -LiteralPath (Join-Path $RepositoryRoot 'payload\Resources\eXPerience2K')).Path
$expectedCount = 501
$expectedDigest = 'A721F489623A70177D8AF6A0AD433D14A7E14EFEA0F1673C4E5E2228A5BFBE1A'

$lines = Get-ChildItem -LiteralPath $iconRoot -Recurse -File -Filter '*.ico' |
    ForEach-Object {
        $relativePath = $_.FullName.Substring($iconRoot.Length + 1).
            Replace('\', '/').ToLowerInvariant()
        $fileHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).
            Hash.ToLowerInvariant()
        "$relativePath`t$fileHash`n"
    } | Sort-Object

if ($lines.Count -ne $expectedCount) {
    throw "Expected $expectedCount original icon files, found $($lines.Count)."
}

$corpus = $lines -join ''
$sha256 = [Security.Cryptography.SHA256]::Create()
try {
    $digest = [BitConverter]::ToString(
        $sha256.ComputeHash([Text.Encoding]::UTF8.GetBytes($corpus))).Replace('-', '')
} finally {
    $sha256.Dispose()
}

if ($digest -ne $expectedDigest) {
    throw "Original icon corpus mismatch. Expected $expectedDigest, found $digest."
}

$operations = Import-Csv -Delimiter "`t" -LiteralPath (Join-Path $RepositoryRoot 'payload\operations.tsv')
$iconOperations = @($operations | Where-Object { $_.type -eq 'ICONGROUP' })
if ($iconOperations.Count -ne 503) {
    throw "Expected 503 original icon-group mappings, found $($iconOperations.Count)."
}

$payloadRoot = Join-Path $RepositoryRoot 'payload'
$scriptRoot = Join-Path $payloadRoot 'Resources\scripts'
$targets = Import-Csv -Delimiter "`t" -LiteralPath (Join-Path $payloadRoot 'targets.tsv')
$activeScripts = @($targets | Select-Object -ExpandProperty script -Unique)
$scriptMappings = @(
    foreach ($script in $activeScripts) {
        $scriptPath = Join-Path $scriptRoot $script
        if (-not (Test-Path -LiteralPath $scriptPath -PathType Leaf)) {
            throw "Target manifest references a missing resource script: $script"
        }
        Select-String -LiteralPath $scriptPath `
            -Pattern '^\s*-addoverwrite\s+"?([^",]+\.ico)"?\s*,\s*ICONGROUP\s*,'
    }
)
if ($scriptMappings.Count -ne 503) {
    throw "Expected 503 icon-group commands in the resource scripts, found $($scriptMappings.Count)."
}
foreach ($mapping in $scriptMappings) {
    $asset = $mapping.Matches[0].Groups[1].Value.Replace('/', '\')
    $assetPath = Join-Path $payloadRoot $asset
    if (-not (Test-Path -LiteralPath $assetPath -PathType Leaf)) {
        throw "Icon-group command references a missing original asset: $asset"
    }
}

Write-Host "Verified $expectedCount byte-identical original icon files and $($iconOperations.Count) executable icon-group mappings."
