param(
    [Parameter(Mandatory = $true)]
    [string]$SourceDirectory,
    [Parameter(Mandatory = $true)]
    [string]$OutputIso
)

$ErrorActionPreference = 'Stop'
$source = (Resolve-Path -LiteralPath $SourceDirectory).Path
$output = [System.IO.Path]::GetFullPath($OutputIso)

$image = New-Object -ComObject IMAPI2FS.MsftFileSystemImage
$image.FileSystemsToCreate = 3
$image.VolumeName = 'EXPERIENCE2K64'
$image.Root.AddTree($source, $false)
$result = $image.CreateResultImage()

Add-Type @'
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
public static class Experience2K64ComStreamCopy {
    public static void Save(object source, string destination) {
        IStream stream = (IStream)source;
        using (FileStream file = new FileStream(destination, FileMode.Create, FileAccess.Write)) {
            byte[] buffer = new byte[65536];
            IntPtr count = Marshal.AllocHGlobal(sizeof(int));
            try {
                while (true) {
                    Marshal.WriteInt32(count, 0);
                    stream.Read(buffer, buffer.Length, count);
                    int read = Marshal.ReadInt32(count);
                    if (read <= 0) break;
                    file.Write(buffer, 0, read);
                }
            } finally {
                Marshal.FreeHGlobal(count);
            }
        }
    }
}
'@

[Experience2K64ComStreamCopy]::Save($result.ImageStream, $output)
Get-Item -LiteralPath $output
