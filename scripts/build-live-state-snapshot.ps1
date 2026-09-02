[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$GccX86Path)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$image=Join-Path $root 'build\eXPerience2KImage.o'
if(-not(Test-Path -LiteralPath $image)){throw 'Run build.ps1 first.'}
$savedPath=$env:PATH
try {
    $env:PATH="$(Split-Path -Parent $GccX86Path);$env:PATH"
    & $GccX86Path -std=c99 -Os -Wall -Wextra -Werror -D_WIN32_WINNT=0x0501 -static -s `
        '-Wl,--major-os-version,5,--minor-os-version,1,--major-subsystem-version,5,--minor-subsystem-version,1' `
        -o (Join-Path $root 'build\live-state-snapshot.exe') `
        (Join-Path $root 'tests\live-state-snapshot.c') $image `
        -lstdc++ -lgdiplus -lgdi32 -lole32 -ladvapi32 -lcomctl32 -lcomdlg32 -lshell32 -lshlwapi
    if($LASTEXITCODE){throw 'Live-state snapshot build failed.'}
} finally { $env:PATH=$savedPath }
Write-Host 'Built the read-only guest lifecycle snapshot tool (not part of the installer).'
