# Validation record

## Version 3.1.0 logon-background validation

The release installer SHA-256 is
`68314B8A8AF8F47772E4C4B239708784D6B8E95B6CC03ADA327B30ECB73FC9F8`.
The same installer was installed in isolated XP Professional x86 SP3 and
XP Professional x64 SP2 validation clones. Original VMs and the preserved
3.0.0 source/installer were not changed.

The release-specific checks covered:

- actual blue (`#3A6EA5`), teal (`#008080`) and custom-PNG logon backgrounds
  visually inspected after reboot on both architectures;
- custom images remaining usable after their original files were moved;
- proportional fitting with blue letterboxing and unchanged logon artwork;
- the saved custom selection and managed-image status after reopening;
- actual configuration UI selection, native image chooser, Apply and Revert;
- successful full restoration on x86 through the Revert button and on x64
  through the same complete restore path used by uninstall, including return
  to the original `(None)` logon wallpaper value;
- native XP PNG/JPG/JPEG/BMP conversion on both architectures, including JPEG
  camera orientation;
- host regression checks for 24-bit uncompressed BMP output, exact teal color,
  transparent PNG flattening, Unicode paths, unchanged input hashes, invalid
  input rejection and excessive-dimension rejection;
- the production state functions tested on both XP architectures with
  process-local registry redirection, covering exact type/data/absence
  restoration, repeated Apply/Revert, a pre-existing baseline in the 3.0.0
  schema, and independence from caption choices and the signed-in wallpaper;
- the initial window fitting the desktop work area, including scrolling to
  all controls and buttons at 800x600; and
- the complete release asset, PE, packaging, version and OS-gate verifier.

The resource engine, Explorer module and all payload/reference assets are
unchanged from 3.0.0. Historical full-feature lifecycle results below belong
to their stated versions; this update's focused tests do not claim exhaustive
testing of every possible image, display configuration or third-party tool.

## Confirmed source lineage

The x86 port is based on the final 2.4.1 exact-state-restore checkpoint. That
checkpoint corrected restoration of the full Start-menu/taskbar `ShellState`,
the original taskbar/font/theme values, Control Panel state, all sound-event
branches, Explorer state, caption choices, and registry values that were
originally absent.

## XP x64 lifecycle evidence

Development validation covered clean Windows XP Professional x64 Edition SP2
virtual machines built from both retail and volume-license media. The tested
surfaces included:

- Windows 2000-style resource conversion and icon coverage;
- Classic theme, Tahoma 8 captions, taskbar and title-bar metrics;
- Classic Start Menu and Classic Control Panel;
- sliding/fading menu animation selection;
- classic GINA logon, background and caption presets;
- wallpapers;
- Windows 2000 sounds and independent folder navigation sound;
- experimental native x64 Explorer folder pane and Icons view;
- protected-file verification and reboot persistence; and
- complete feature clearing and uninstall restoration.

## XP x86 SP3 lifecycle evidence

The native x86 port was validated from an authoritative clean Windows XP
Professional x86 SP3 snapshot with Guest Additions. Version 3.0.0 passed:

- strict profile detection as NT 5.1 build 2600 SP3 Professional x86;
- all eleven default feature paths;
- 134 patched resource targets with zero original, changed, or missing targets
  after reboot;
- visual inspection of the desktop, taskbar, Start menu, Winver, System
  Properties, classic logon, Control Panel, Explorer pane, and Folders bar;
- byte-identical installation of all five wallpapers and eight sound files;
- full Revert to 134 original targets, XP sounds, Luna/Welcome behavior, and
  removal of installed wallpapers;
- correct post-Revert checkbox state;
- a second Apply after Revert; and
- uninstall restoration to all 134 originals with program cleanup.

The retail system was patched before applying its post-install updates. The
conversion persisted through the required update/restart sequence and could
also be applied to an already updated installation.

## Negative platform behavior

Version 3.0.0 accepts only XP Professional x86 SP3 and XP Professional x64 SP2.
All other profiles remain fail-closed.

## Release-integrity checks

The release verifier checks:

- expected executable names and version metadata;
- PE32/PE32+ architecture and NT compatibility fields;
- 672 operations, 147 targets, and 671 unique resource files;
- eleven feature definitions and their defaults;
- original branding and icon corpus hashes;
- five wallpaper assets;
- eight Windows 2000 sound assets;
- Explorer WebView/state manifests and native x86/x64 COM exports; and
- strict installer and configuration-application profile guards.

The final release notes and GitHub asset record the verified installer hash.
