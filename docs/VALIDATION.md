# Validation record

## Confirmed release lineage

The 2.4.1 source is based on the final exact-state-restore checkpoint. That
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

The retail system was patched before applying its post-install updates. The
conversion persisted through the required update/restart sequence and could
also be applied to an already updated installation.

## Negative platform behavior

The publication candidate adds an installer and configuration-app guard for
32-bit Windows. The release boundary requires a negative x86 test confirming
the dialog appears and no files or settings are changed.

## Release-integrity checks

The release verifier checks:

- expected executable names and version metadata;
- PE32/PE32+ architecture and NT compatibility fields;
- 672 operations, 147 targets, and 671 unique resource files;
- eleven feature definitions and their defaults;
- original branding and icon corpus hashes;
- five wallpaper assets;
- eight Windows 2000 sound assets;
- Explorer WebView/state manifests and native x64 COM exports; and
- presence of both x86 refusal guards.

The final release notes and GitHub asset record the verified installer hash.
