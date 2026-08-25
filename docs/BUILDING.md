# Building from source

## Requirements

- Windows PowerShell 5.1 or later for the build scripts;
- MinGW-w64 i686 GCC/G++ and `objdump.exe` for the configuration application,
  native x86 resource engine, and x86 Explorer module;
- MinGW-w64 x86_64 GCC/G++ and `objdump.exe` for the native XP x64 engine and
  Explorer module; and
- NSIS 3 with `makensis.exe`.

The produced applications are native Win32/Win64 binaries and do not depend on
.NET. The build script sets the PE operating-system and subsystem versions
needed by NT 5.1/5.2 rather than accepting modern toolchain defaults.

## Build

From the repository root:

```powershell
.\scripts\build.ps1 `
  -GccX86Path C:\msys64\mingw32\bin\gcc.exe `
  -GccX64Path C:\msys64\mingw64\bin\gcc.exe `
  -MakeNsisPath "C:\Program Files (x86)\NSIS\makensis.exe"
```

If the three tools are already on `PATH`, the corresponding arguments may be
omitted.

Expected outputs:

```text
build/eXPerience2K.exe
build/eXPerience2KCore-x86.exe
build/eXPerience2KCore-x64.exe
build/eXPerience2KExplorerBand32.dll
build/eXPerience2KExplorerBand64.dll
dist/eXPerience2K-v3.0.0-Setup.exe
```

Both native cores and both Explorer modules are included. The installer and
configuration application enforce the exact XP Professional x86 SP3/x64 SP2
support boundary.

## Verify

```powershell
.\scripts\verify-release.ps1
```

Verification checks the expected output names, PE architecture/NT-version
fields, payload counts, branding, icon, wallpaper, sound and Explorer hashes,
feature defaults, strict dual-profile gates, undecorated x86 COM exports, and
the installer version metadata.

## GitHub Actions

`.github/workflows/build.yml` runs the same build and verification sequence on
Windows. Workflow artifacts are continuous-integration outputs, not official
releases. The official 3.0.0 release asset is the single installer attached to
the GitHub release.

## Reproducibility note

PE/NSIS timestamps and compressor output can prevent byte-for-byte identical
local rebuilds across toolchain versions. Source and payload equivalence are
attested separately from the exact published installer hash. See
[REPRODUCIBILITY.md](REPRODUCIBILITY.md).
