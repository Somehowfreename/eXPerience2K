# eXPerience2K 2.4.1

eXPerience2K brings the Windows 2000-style conversion to Windows XP
Professional x64 Edition SP2 and adds a configurable, reversible layer around
the resource conversion.

## Highlights

- Native XP x64 protected-resource patching across 64-bit, WoW64, WinSxS, and
  Windows File Protection cache targets.
- Eleven independently configurable visual, shell, logon, wallpaper, Explorer,
  sound, and animation options.
- Exact first-run state capture and restoration rather than assumed XP
  defaults.
- Independent Solid Navy and Blue Gradient caption presets for logon and the
  signed-in desktop.
- Five optional wallpapers and exact Windows 2000 sound assets.
- Experimental native x64 Windows 2000-style Explorer folder pane without
  replacing `explorer.exe`.
- Startup repair, CRC verification, privacy-safe diagnostics, and fail-closed
  uninstall restoration.
- Fail-closed installer detection that accepts only AMD64 Windows XP
  Professional x64 Edition SP2 and rejects x86, IA-64, other XP x64 editions,
  and every Windows Server edition before installation.

## Supported platform

Only English Windows XP Professional x64 Edition SP2 is supported, tested, and
accepted by the installer. Server editions, other XP x64 editions, IA-64, and
all Windows XP x86 editions are rejected before any files or settings are
changed. Windows XP Professional x86 support is planned for the next major
update.

## Installation

Run `eXPerience2K-v2.4.1-Setup.exe` as an administrator, open the installed
configuration application, choose the desired features, click **Apply**, and
restart when prompted.

## Asset

The GitHub release contains only the installer:

```text
eXPerience2K-v2.4.1-Setup.exe
SHA-256: B64CE6446CD1D52B6B8DCDF0F085DED3BAA18D66123D722005A2E196B93CFFE3
Size:    4,307,201 bytes
```

## Important safety note

This unsigned tool intentionally modifies protected Windows system resources
and uses an XP-era Windows File Protection suspension technique. Keep a system
image or other recovery path. See `SECURITY.md` before use.
