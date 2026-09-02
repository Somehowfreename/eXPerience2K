# eXPerience2K

eXPerience2K is an open-source Windows 2000-style conversion for **Windows XP
Professional x86 SP3** and **Windows XP Professional x64 Edition SP2**. It
preserves each operating system's native application compatibility while
replacing supported shell artwork and automating the native Windows settings
needed for a cohesive Windows 2000 experience.

> [!IMPORTANT]
> Version 3.1.1 supports only English Windows XP Professional
> x86 SP3 and Windows XP Professional x64 Edition SP2. Home, Starter, Media
> Center, Tablet PC, Embedded, IA-64, Server editions, and other service-pack
> levels are rejected before any files or settings are changed.

The current release is **eXPerience2K 3.1.1**. Download
[`eXPerience2K-v3.1.1-Setup.exe`](https://github.com/Somehowfreename/eXPerience2K/releases/latest)
from the repository's **Latest** release.

Version 3.1.1 fixes the Explorer information pane's Network Places link,
restores image thumbnails and Windows 2000-style media preview controls,
selects native Small toolbar icons, and keeps XP's Start-menu choice visible.
It also improves upgrade and exact-state restoration behavior. See the
[changelog](CHANGELOG.md) for details.

## Why I made it

I always loved InexperiencePatcher and the way it transformed Windows XP into
something that felt like Windows 2000. What always bugged me was that it
explicitly did not support 64-bit Windows and never gained Windows XP x64
support. I decided to build that missing x64 implementation myself, preserve
the spirit and artwork of the original, and make the result maintainable and
open source.

The x86 port brings the later eXPerience2K configuration, restoration, sound,
wallpaper, Explorer, and quality-of-life work back to the platform supported
by the original patcher, rather than merely wrapping the older x86 conversion.

Disclosure: eXPerience2K was created with the assistance of Codex, however please know I directed this project and manually tested each version on virtual machines and real hardware. I do not release anything without extensive prior testing.

## What eXPerience2K adds

The original patcher already provided the Windows 2000-style resource set,
protected-file backups, a startup reloader, verification, and uninstall
restoration. eXPerience2K retains that foundation and adds:

- native Windows XP Professional resource discovery and patching for both x86
  SP3 and x64 SP2, including the applicable System32, WoW64, WinSxS common
  controls, and Windows File Protection caches;
- a reopenable configuration application whose selections reflect the
  machine's current state;
- a native resizable configuration window with responsive control widths and
  a right-side vertical scrollbar whenever the available height cannot show
  the complete interface;
- an immutable first-Apply baseline that restores the exact original registry
  data, type, value absence, shell state, sounds, fonts, metrics, theme,
  Explorer state, and logon settings when features are later cleared;
- individually reversible Classic theme, Classic Start menu/taskbar, Classic
  Control Panel, classic logon, wallpaper, sound, animation, and Explorer
  options;
- independent Solid Navy and Blue Gradient caption presets for the secure
  logon desktop and the signed-in desktop;
- a separate logon-background choice: current blue, Windows 95 teal, or a
  custom PNG/JPG/JPEG/BMP image automatically converted for XP;
- mutually exclusive sliding and fading Start-menu animation choices;
- the exact Windows 2000 system sounds, with a separately controllable folder
  double-click/navigation sound;
- five optional Windows 2000-style wallpapers installed to the interactive
  user's My Pictures folder without overwriting existing files;
- architecture-matched native x86 and x64 Windows 2000-style Explorer folder
  interfaces that keep XP's original `explorer.exe` and shell functionality;
- correct interactive-user targeting when XP's **Run As** command uses a
  different administrator account;
- privacy-safe in-memory diagnostics with user-controlled Open and Save
  actions; and
- fail-closed restoration: the uninstaller keeps recovery data in place if a
  complete restore cannot be verified.

## Complete feature list

The configuration application exposes eleven options:

1. **Windows 2000 visual resource conversion** — applies the supported Windows
   2000-style icons, bitmaps, shell animations, strings, and branding artwork.
2. **Automatically change the Windows theme to Classic** — applies the
   Windows Classic visual style, Tahoma 8 interface fonts, 18-pixel captions,
   and the selected signed-in caption preset.
3. **Enable the Classic Start menu and taskbar layout** — selects XP's native
   Classic Start Menu and matching taskbar layout.
4. **Use Classic Control Panel view by default** — opens Control Panel in its
   classic icon view.
5. **Use sliding Start menu and submenu animations** — enables menu animation
   without fading. This is the first-launch default.
6. **Use fading Start menu and submenu animations** — enables the alternative
   fade transition. Sliding and fading are mutually exclusive.
7. **Windows 2000 style login window** — disables the XP Welcome screen and
   uses XP's native classic GINA logon with Windows 2000-style artwork and
   background, with a choice of current blue, Windows 95 teal or a custom image.
8. **Install Windows 2000 style wallpapers to My Pictures** — installs five
   curated JPEG wallpapers named `Windows_2000_1.jpg` through
   `Windows_2000_4.jpg`, plus `Windows_9x.jpg`.
9. **Windows 2000 Explorer folder interface (experimental)** — adds a native
   architecture-matched Windows 2000-style information pane and Windows 2000
   WebView assets to XP folder windows without replacing `explorer.exe`.
   The pane provides native Network Places navigation, proportional image
   previews, and legacy Media Player controls for supported media files.
   The toolbar initially uses Small icons; later user preferences are retained.
10. **Replace all Windows XP sounds with Windows 2000 equivalents** — maps the
    XP event set to eight exact Windows 2000 WAV files; XP-only events with no
    Windows 2000 counterpart are intentionally silent.
11. **Enable the Windows 2000 folder double-click sound** — independently maps
    Explorer's navigation event to the exact Windows 2000 folder sound.

Default-on choices are defaults only on the first launch. After the first
Apply, reopening the program shows the current configuration and never
silently re-enables a choice the user cleared.

The bottom-row **Revert** button provides a separate full-reset path. It asks
for Yes/No confirmation, defaults to No, and on confirmation restores every
managed feature and caption setting to the immutable baseline captured before
the first Apply. The baseline is retained after a successful Revert so later
launches continue to show the actual restored configuration instead of
reasserting first-launch defaults.

The configuration window can be resized from every edge and corner or
maximized. Its controls widen with the window. If the available height is too
small for the full interface, the native vertical scrollbar and mouse wheel
move through the hidden content; opening the diagnostic log scrolls directly
to it.

### Caption presets

The secure logon prompt and signed-in Windows each have their own independent
choice:

- **Solid Navy:** `RGB(0, 0, 128)` / `#000080`
- **Blue Gradient:** `RGB(10, 36, 106)` / `#0A246A` to
  `RGB(166, 202, 240)` / `#A6CAF0`

The logon prompt defaults to Solid Navy. The signed-in desktop defaults to the
Blue Gradient. On a basic or low-color VGA driver, Windows may render a chosen
gradient as a solid caption until a suitable display driver is installed.

### Logon background (new in 3.1.0)

With **Windows 2000 style login window** enabled, the **Logon background**
selector offers:

- **Current blue:** `RGB(58, 110, 165)` / `#3A6EA5` (the unchanged default).
- **Windows 95 teal:** `RGB(0, 128, 128)` / `#008080`.
- **Custom image:** select this, click **Choose image...**, and choose a PNG,
  JPG/JPEG, or BMP file. Click **Apply**, then restart Windows.

The app automatically decodes the image with XP's built-in GDI+ and creates an
uncompressed 24-bit BMP. It preserves the image's proportions and fits it to
the current display dimensions; transparent areas and unused space are blue.
JPEG orientation metadata is respected. The original file is never changed.
No downloads, .NET, or additional image converter are required.

The converted copy is stored in
`%WINDIR%\eXPerience2K\Assets\custom-logon-background.bmp`, so it remains
available before sign-in and after the original image is moved or deleted.
Your custom image is not included in diagnostic logs or uploaded anywhere.
Use images up to 64 MB, 32 megapixels and 16,384 pixels per dimension. Invalid
or unsupported files are rejected before any settings are changed. After
changing display resolution, choose the source image again to refit it.

This affects only the background behind the classic logon prompt, not the
signed-in wallpaper, the branding, or either caption preset. The selection
persists across reopening and in-place updates. Clearing the classic login
option, using **Revert**, or uninstalling restores the original logon settings;
the saved custom BMP stays available until the application is uninstalled.

## Exact-state restoration

Before the first Apply transaction changes anything, eXPerience2K captures one
immutable baseline of every setting managed by every checkbox. That baseline
is stored separately from the current configuration and is never overwritten
by later Apply operations.

Clearing a feature and clicking Apply therefore restores what was actually on
that installation—not an assumed XP default. This includes the complete
Start-menu/taskbar `ShellState`, Control Panel values, every managed sound
event, theme colors, caption-gradient state, fonts and window metrics,
Explorer settings, Winlogon selection, and values that originally did not
exist.

## Installation and use

1. Use English Windows XP Professional x86 SP3 or Windows XP Professional x64
   Edition SP2 and log on with an
   administrator account, or use XP's **Run As** command with an administrator
   account.
2. Keep installation media or a complete system image available. Modifying
   protected operating-system resources always carries risk.
3. Run `eXPerience2K-v3.1.1-Setup.exe`.
4. Open eXPerience2K, review the selections and caption presets, then click
   **Apply**.
5. Restart when prompted. Allow the startup reloader to finish.
6. Reopen eXPerience2K whenever you want to inspect, enable, or restore
   individual features.

Machine-level choices require administrator rights. User-level choices target
the interactive desktop account even when the administrator supplied through
**Run As** is a different account. If a requested transaction needs rights the
process does not have, Apply stops before making partial changes and explains
what is required.

### Updating an existing installation

Close the configuration application and run the new installer. Restart if
setup requests it: Explorer can hold a previous version's module open until
then. After restarting, reopen eXPerience2K and click **Apply** with your
desired options. Keep the existing program and restoration backup data in
place; the update preserves the original first-Apply baseline.

Media previews use XP's installed legacy Media Player control and codecs.
A small 32-bit helper hosts that control on both systems; Explorer itself
remains native x86 or x64. Selecting a file does not start playback. Playback
stops when its preview is dismissed or its folder window closes.

## Diagnostics, persistence, and uninstall

Diagnostics stay in memory unless the user chooses **Save log...** and a
destination. Logs do not contain account names, profile paths, SIDs, product
keys, or installation-media keys.

The patch engine validates staged PE files, preserves original-file backups,
records original and patched CRC-32 values, covers protected caches, and
schedules locked replacements safely. Its verifier classifies each target as
patched, original, changed, or missing. The startup reloader reapplies managed
resources if Windows File Protection or an update restores them. The published
2.4.1 conversion remained applied through the complete post-install update
sequence on a clean retail XP x64 system. Version 3.0.0 separately
passed clean-install Apply, reboot persistence, Revert, repeat Apply, and
uninstall restoration on XP Professional x86 SP3.

Uninstall performs a restoration transaction before removing the program. If
restoration cannot be completed and verified, uninstall stops and retains the
application and backup data rather than destroying the recovery path.

The **Revert** button uses the same complete restoration stages without
uninstalling the application or deleting the immutable baseline.

## Scope and deliberate limits

- Supported by version 3.1.1: English Windows XP Professional x86 SP3 and
  Windows XP Professional x64 Edition SP2.
- Rejected: every other XP edition/service-pack combination and every Windows
  Server edition.
- Not changed: the kernel and boot screen.
- Not replaced: `explorer.exe`.
- Not included: Internet Explorer executable/icon experiments.
- No .NET dependency and no network code.

See the [support matrix](docs/SUPPORT-MATRIX.md) for the exact boundary.

## Release integrity

The sole binary asset attached to release 3.1.1 is:

```text
eXPerience2K-v3.1.1-Setup.exe
SHA-256: D9B4027A3F73104937035381F4E911DCEC75A59C2D23C5DDEBF49ADE2AF4CE7F
Size:    4356311 bytes
```

The executable is unsigned. Security products may warn because the requested
operation modifies protected Windows files and uses an XP-era Windows File
Protection suspension technique. See [SECURITY.md](SECURITY.md) for the exact
behavior and threat boundary.

## Building from source

The project uses native Win32 C/C++, MinGW-w64, NSIS 3, and Windows PowerShell.
It does not require .NET:

```powershell
.\scripts\build.ps1 `
  -GccX86Path C:\path\to\mingw32\bin\gcc.exe `
  -GccX64Path C:\path\to\mingw64\bin\gcc.exe `
  -MakeNsisPath "C:\Program Files (x86)\NSIS\makensis.exe"

.\scripts\verify-release.ps1
```

The expected installer is `dist\eXPerience2K-v3.1.1-Setup.exe`. See
[BUILDING.md](docs/BUILDING.md) and
[REPRODUCIBILITY.md](docs/REPRODUCIBILITY.md) for prerequisites and the exact
published-source attestation.

## Repository layout

```text
src/                    native configuration app, patch engines, Explorer module
installer/              NSIS installer and fail-closed uninstaller
payload/                manifests, feature definitions, artwork, sounds and state
reference-assets/       source-reference material and asset provenance inputs
scripts/                build and verification tooling
tests/                  isolated logon-state and image-conversion tests
tools/resource-hacker/  documented resource-patching fallback
docs/                   architecture, support, testing and release documentation
```

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Support matrix](docs/SUPPORT-MATRIX.md)
- [Building from source](docs/BUILDING.md)
- [Reproducibility and source attestation](docs/REPRODUCIBILITY.md)
- [Feature and lifecycle testing](docs/TESTING.md)
- [Validation record](docs/VALIDATION.md)
- [Experimental Explorer implementation](docs/EXPLORER-EXPERIMENT.md)
- [Release notes](docs/RELEASE-NOTES-3.1.1.md)
- [Security behavior](SECURITY.md)
- [Legal and redistribution notes](docs/LEGAL.md)
- [Contributing](CONTRIBUTING.md)

## License and attribution

Repository-authored code and documentation are available under
[`LICENSE-CODE`](LICENSE-CODE). That license does not grant rights to
third-party tools, Microsoft software, names, marks, visual resources, or
sounds. Read [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) and
[`docs/LEGAL.md`](docs/LEGAL.md) before redistribution.

This is an independent community project and is not affiliated with,
approved by, or supported by Microsoft.
