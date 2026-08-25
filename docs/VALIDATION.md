# Validation record

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
