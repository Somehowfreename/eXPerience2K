# eXPerience2K64 2.4.1

eXPerience2K64 brings the Windows 2000-style conversion to Windows XP
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
- Explicit x86 refusal before installation, with a clear message that x86
  support is in development.

## Supported platform

Only English Windows XP Professional x64 Edition SP2 is supported and tested.
Server editions are untested and unsupported. Windows XP x86 is unsupported in
this release and is blocked by the installer.

## Installation

Run `eXPerience2K64-v2.4.1-Setup.exe` as an administrator, open the installed
configuration application, choose the desired features, click **Apply**, and
restart when prompted.

## Asset

The GitHub release contains only the installer:

```text
eXPerience2K64-v2.4.1-Setup.exe
SHA-256: 5A5E86B179BEDF6F948140A9265C2BAE2D315BC2A6B06DE6530031E859C232AD
Size:    4,328,258 bytes
```

## Important safety note

This unsigned tool intentionally modifies protected Windows system resources
and uses an XP-era Windows File Protection suspension technique. Keep a system
image or other recovery path. See `SECURITY.md` before use.
