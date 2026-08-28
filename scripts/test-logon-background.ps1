[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$GccX86Path)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$output = Join-Path $root 'test-evidence\logon-background'
New-Item -ItemType Directory -Path $output -Force | Out-Null
$exe = Join-Path $root 'build\logon-background-tests.exe'
$savedPath = $env:PATH
try {
    $env:PATH = "$(Split-Path -Parent $GccX86Path);$env:PATH"
    & $GccX86Path '-std=c99' '-Os' '-Wall' '-Wextra' '-Werror' '-D_WIN32_WINNT=0x0501' `
        '-static' '-municode' '-Wl,--major-os-version,5,--minor-os-version,1,--major-subsystem-version,5,--minor-subsystem-version,1' `
        (Join-Path $root 'tests\logon-background-tests.c') (Join-Path $root 'build\eXPerience2KImage.o') `
        '-lstdc++' '-lgdiplus' '-lole32' '-ladvapi32' '-lcomctl32' '-lcomdlg32' '-lshell32' '-lshlwapi' '-lgdi32' `
        '-o' $exe
    if ($LASTEXITCODE -ne 0) { throw 'Test harness compilation failed.' }
} finally { $env:PATH = $savedPath }

Add-Type -AssemblyName System.Drawing
$bitmap = New-Object System.Drawing.Bitmap 40,20
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.Clear([System.Drawing.Color]::Lime)
$graphics.Dispose()
foreach ($extension in @('png','jpg','jpeg','bmp')) {
    $format = switch ($extension) {
        'png' { [System.Drawing.Imaging.ImageFormat]::Png }
        'bmp' { [System.Drawing.Imaging.ImageFormat]::Bmp }
        default { [System.Drawing.Imaging.ImageFormat]::Jpeg }
    }
    $source = Join-Path $output "input.$extension"
    $bitmap.Save($source, $format)
    $before = (Get-FileHash $source -Algorithm SHA256).Hash
    $destination = Join-Path $output "converted-$extension.bmp"
    & $exe convert $source $destination 800 600
    if ($LASTEXITCODE -ne 0) { throw "Conversion failed: $extension" }
    if ((Get-FileHash $source -Algorithm SHA256).Hash -ne $before) { throw 'Source image was modified.' }
    $bytes = [IO.File]::ReadAllBytes($destination)
    if ([Text.Encoding]::ASCII.GetString($bytes,0,2) -ne 'BM' -or
        [BitConverter]::ToUInt16($bytes,28) -ne 24 -or
        [BitConverter]::ToUInt32($bytes,30) -ne 0 -or
        [BitConverter]::ToInt32($bytes,18) -ne 800 -or
        [BitConverter]::ToInt32($bytes,22) -ne 600) { throw 'Output is not the required uncompressed 24-bit 800x600 BMP.' }
    $decoded = [Drawing.Bitmap]::FromFile($destination)
    if ($decoded.GetPixel(0,0).ToArgb() -ne [Drawing.Color]::FromArgb(58,110,165).ToArgb() -or
        $decoded.GetPixel(400,300).G -lt 250) { throw 'Aspect-preserving fit or blue letterboxing failed.' }
    $decoded.Dispose()
    Write-Host "PASS: $extension conversion, 24-bit BMP, aspect ratio, letterbox color, original preserved"
}
$bitmap.Dispose()
$jpeg = [IO.File]::ReadAllBytes((Join-Path $output 'input.jpg'))
# Minimal EXIF APP1 with orientation 6 (rotate 90 degrees clockwise).
$exif = [byte[]]@(0xff,0xe1,0,34,69,120,105,102,0,0,73,73,42,0,8,0,0,0,
    1,0,18,1,3,0,1,0,0,0,6,0,0,0,0,0,0,0)
$oriented = Join-Path $output 'oriented.jpg'
[IO.File]::WriteAllBytes($oriented, [byte[]]($jpeg[0..1] + $exif + $jpeg[2..($jpeg.Length-1)]))
& $exe convert $oriented (Join-Path $output 'oriented.bmp') 800 600
if ($LASTEXITCODE -ne 0) { throw 'JPEG orientation conversion failed.' }
$decoded = [Drawing.Bitmap]::FromFile((Join-Path $output 'oriented.bmp'))
if ($decoded.GetPixel(50,300).ToArgb() -ne [Drawing.Color]::FromArgb(58,110,165).ToArgb() -or
    $decoded.GetPixel(400,300).G -lt 250) { throw 'JPEG EXIF orientation was not applied.' }
$decoded.Dispose()
Write-Host 'PASS: JPEG camera orientation applied before proportional fitting'
$transparent = New-Object System.Drawing.Bitmap 20,20
$transparent.Save((Join-Path $output 'transparent.png'), [Drawing.Imaging.ImageFormat]::Png)
$transparent.Dispose()
& $exe convert (Join-Path $output 'transparent.png') (Join-Path $output 'flattened.bmp') 800 600
if ($LASTEXITCODE -ne 0) { throw 'Transparent PNG conversion failed.' }
$decoded = [Drawing.Bitmap]::FromFile((Join-Path $output 'flattened.bmp'))
if ($decoded.GetPixel(400,300).ToArgb() -ne [Drawing.Color]::FromArgb(58,110,165).ToArgb()) { throw 'PNG transparency was not flattened.' }
$decoded.Dispose()
Write-Host 'PASS: transparent PNG flattened to blue'
$unicodePath = Join-Path $output (([char]0x753b).ToString() + ([char]0x50cf).ToString() + '.png')
Copy-Item (Join-Path $output 'input.png') $unicodePath -Force
& $exe convert $unicodePath (Join-Path $output 'unicode.bmp') 800 600
if ($LASTEXITCODE -ne 0) { throw 'Unicode image path conversion failed.' }
Write-Host 'PASS: Unicode image filename'
$invalid = Join-Path $output 'invalid.png'
[IO.File]::WriteAllText($invalid, 'Not an image')
& $exe convert $invalid (Join-Path $output 'invalid-output.bmp') 800 600
if ($LASTEXITCODE -eq 0 -or (Test-Path (Join-Path $output 'invalid-output.bmp'))) { throw 'Invalid image was not rejected safely.' }
Write-Host 'PASS: invalid image rejected without output'
& $exe convert (Join-Path $output 'input.png') (Join-Path $output 'oversized.bmp') 99999 99999
if ($LASTEXITCODE -eq 0) { throw 'Unsafe output dimensions were accepted.' }
Write-Host 'PASS: oversized output rejected'
$stateRoot = Join-Path $output ('state-install-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path (Join-Path $stateRoot 'Assets') -Force | Out-Null
Copy-Item (Join-Path $root 'payload\Assets\logon-background.bmp') (Join-Path $stateRoot 'Assets') -Force
& $exe state $stateRoot (Join-Path $output 'input.png')
if ($LASTEXITCODE -ne 0) { throw 'Isolated registry-state lifecycle tests failed.' }
$teal = [Drawing.Bitmap]::FromFile((Join-Path $stateRoot 'Assets\teal-logon-background.bmp'))
if ($teal.GetPixel(0,0).ToArgb() -ne [Drawing.Color]::FromArgb(0,128,128).ToArgb()) { throw 'Teal bitmap color is incorrect.' }
$teal.Dispose()
Write-Host 'PASS: generated teal bitmap has exact RGB(0,128,128)'
Write-Host 'All image-conversion and isolated logon-state tests passed. These do not replace XP VM visual validation.'
