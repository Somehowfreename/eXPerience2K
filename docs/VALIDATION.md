# Validation record

## Version 3.1.1 regression and release validation

The tested final installer is `eXPerience2K-v3.1.1-Setup.exe`, 4,356,311 bytes,
SHA-256:

```text
D9B4027A3F73104937035381F4E911DCEC75A59C2D23C5DDEBF49ADE2AF4CE7F
```

Validation used isolated, clean English Windows XP Professional x86 SP3 and
Windows XP Professional x64 SP2 installations with Guest Additions and audio.
The same installer was used on both. No kernel or boot-screen changes were
included. The original VM snapshots and earlier source/releases were preserved.

### Clean lifecycle and restoration

- All default selections completed through the actual configuration UI.
- After reboot, the native verifier reported all 134 x86 and all 208 x64
  applicable protected files patched, with no original, changed, or missing
  targets.
- The desktop, resource icons, taskbar, captions, Start menu, Winver, System
  Properties, classic logon, Control Panel and Explorer were visually checked.
- Full Revert on x86 and clearing every option on x64 completed successfully.
  After reboot, all 276 captured managed registry values/runtime structures
  matched their pre-Apply baselines exactly on each architecture.
- An independent read-only resource audit matched all 134/208 restored files
  to their original CRCs, with zero mismatches.
- The final candidate also passed direct uninstall while applied on x64,
  including restored state/resources and removal of both registry-view
  uninstall entries, the application, shortcuts and startup hooks.

### Explorer and Start-menu fixes

- Actual information-pane clicks opened native My Network Places on both
  systems, rather than an Internet Explorer error page.
- Native Customize Toolbar dialogs showed **Small icons**. Native Taskbar
  Properties showed both Start-menu choices, with Classic initially selected.
- BMP/JPEG/PNG selection produced proportional previews, including after
  hiding filename extensions through native Folder Options. Directory,
  multiple-item and empty selections cleared the previous image preview.
- WAV/MP3 selection showed the real legacy player controls. Play, Pause,
  Stop, selection changes, Folders-bar navigation and window-close cleanup
  were exercised. Selection alone did not start playback. Audio recordings
  corroborated playback and silence transitions for the unchanged final
  Explorer/media binaries.
- Native folder/toolbar preferences did not make the active Explorer option
  appear disabled. Reopening the final configuration application and applying
  an unrelated option preserved hidden-extension preferences on both systems.
- Native XP Start-menu and Large-toolbar overrides survived sign-in without
  the reloader reasserting initial preferences. Slide and Fade were mutually
  exclusive; recorded native menu output distinguished movement from fading.

### Existing features and account handling

The regression sequence covered all eleven options, including alternatives
that cannot be selected together. Individual clearing covered theme/font/
metrics, Start menu, Control Panel, Explorer, sounds and navigation sound;
clearing sounds retained the separately selected navigation event, and clearing
navigation restored its own original values. Wallpaper and sound payload bytes
matched all five JPEGs and eight original Windows 2000 WAVs.

Both caption presets, blue/teal/custom logon backgrounds, saved selections,
custom-image persistence after removing the source, resizing/scrolling to all
controls, Revert/No, repeated subset Apply/Revert, and uninstall before Apply
were exercised during this release's regression sequence. Restoration compared
actual type/data/value absence with captured baselines, not assumed defaults.

On both architectures, actual limited-user and XP Run As sessions verified
privilege refusal before machine changes, interactive-user targeting, separate
administrator-profile preservation, navigation-only Apply/undo, and complete
restoration after signing out/in. The final session-resolution and animation
code passed these checks. Final Explorer enablement changes additionally passed
native UI regressions and isolated production-function tests.

### Final published-version upgrade

Both pristine guests installed and applied the actual published 3.1.0
installer, then rebooted. The final 3.1.1 installer upgraded each with the
previous native Explorer module loaded. After the replacement reboot, the
installed configuration executable and architecture-matched Explorer module
hashes matched the final build, and Apply succeeded on both.

All 804 pre-existing immutable backup records remained byte-for-byte
unchanged. Only two new Small-icons backup markers were added. Both native
Start-menu choices were visible, all 134/208 resources verified as patched,
and final MP3/WAV preview recordings confirmed no autoplay, Play/Pause/resume,
Stop and window-close cleanup. The closed preview helper was absent from the
process list. An unrelated XP Security Center notification in the x64 capture
was distinguished from preview playback.

Full Revert after upgrade succeeded on both. After reboot, all 276 managed
records again matched the pristine baselines exactly, and independent CRC
checks confirmed all 134/208 original resources with zero mismatches. All six
source files embedded in the tested installer matched the release source.

### Build and test evidence

The documented build succeeded independently in a separate directory with the
same build inputs. The release verifier checks 672 resource operations, 147
manifest targets, 671 resource files, nineteen original branding assets, 501
icon files/503 mappings, five wallpapers, eight sounds, 74 Web files, eight
Explorer-state inputs, both native architectures, legacy media hosting, strict
OS gates and installer metadata.

The image/state harness passed format conversion, 24-bit BMP output, exact
colors, transparency, EXIF orientation, Unicode, invalid/oversized input,
immutable baseline/repeated restoration, saved images without source files,
privacy-safe diagnostics, retained-backup detection, Run As animation bits,
and Explorer enablement independent of mutable preferences.

These are results for the stated supported systems and test configurations,
not a guarantee for every codec, third-party shell extension, hardware driver
or modified XP installation. Raw guest registry captures contain private
profile information and are retained locally, not distributed as support logs
or release assets. Older-version results below remain identified separately.

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
