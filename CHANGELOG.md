# Changelog

## 3.1.1

- Fixes the My Network Places link in the Windows 2000-style Explorer pane
  so it opens the native network folder instead of an Internet Explorer error.
- Restores selected-image thumbnails and Windows 2000-style media-preview
  controls in the Explorer pane. The legacy media control runs in an isolated
  32-bit helper on both architectures; Explorer itself remains native x86/x64.
- Selects Small toolbar icons through XP's native preference and preserves
  its original value for restoration.
- Keeps the Explorer interface selected when users change native preferences
  such as hiding extensions, folder visibility, or toolbar size. Those choices
  no longer cause an unrelated Apply to remove the active interface.
- Keeps the XP Start-menu choice visible in Taskbar Properties while selecting
  Classic Start through the normal shell state. Migrates the restrictive
  policy previously set by eXPerience2K without overwriting the original
  restoration baseline.
- Preserves XP's ShellState format and the independent Start-menu selection
  when applying or clearing the Explorer interface.
- Restores saved font/window-metric registry values without overwriting them
  with normalized runtime measurements from the active theme.
- Preserves saved Explorer folder views and user preferences during sign-in
  resource repair, and safely replaces loaded application components on reboot
  when upgrading an existing installation.
- Removes the Add/Remove Programs entry from the correct registry view when
  uninstalling on XP x64, rather than leaving a stale entry after file cleanup.
- Avoids treating retained resource backups as an active conversion for a new
  account after Revert, so current-user-only changes do not wrongly require
  administrator access.
- Identifies the signed-in account through XP's native session information
  when Run As cannot read the shell process token. Apply and restoration stop
  without changes if the interactive account cannot be verified.
- Saves menu-animation preferences to the interactive user's profile when
  running under another administrator account, leaving that administrator's
  preferences unchanged.

## 3.1.0

- Adds a logon-background selector under the classic login-window option:
  current blue (`#3A6EA5`), Windows 95 teal (`#008080`), or a custom image.
- Converts PNG, JPG/JPEG, and BMP images locally into an opaque, uncompressed
  24-bit BMP that XP's secure logon desktop can display without Active Desktop.
- Fits custom images to the display without distorting their proportions;
  unused space and transparency use the current blue background. JPEG camera
  orientation is respected.
- Keeps the original image untouched and stores a converted copy in the
  application's Windows installation folder, available before sign-in.
- Remembers the selection and retains the converted image across launches and
  in-place upgrades. The original source file is not needed after Apply.
- Restores the original logon wallpaper, tiling, style, pattern and background
  color when classic logon is cleared, Revert is used, or the app is uninstalled.
  Existing 3.0.0 first-Apply baselines are reused rather than overwritten.
- Keeps logon-background choices independent of both caption presets and the
  signed-in desktop wallpaper. The current blue remains the default.
- Rejects unreadable or unsupported images before changing settings and keeps
  image/profile paths out of diagnostic logs.
- Fits the initial configuration window inside the desktop work area, using
  its native scrollbar on smaller displays.
- Retains all eleven 3.0.0 options, original branding, and XP Professional x86
  SP3 / x64 SP2 support; no boot-screen or additional OS-edition changes.

## 3.0.0

Windows XP Professional x86 SP3 is now fully supported alongside Windows XP
Professional x64 Edition SP2.

- Ports the complete eleven-option eXPerience2K configuration experience to
  native XP Professional x86 SP3 with no feature omissions.
- Adds a native x86 protected-resource engine and architecture-aware selection
  between the x86 and x64 engines.
- Adds a native x86 Windows 2000-style Explorer integration while retaining
  the existing native x64 implementation and XP's original `explorer.exe`.
- Applies the exact Windows 2002 Professional branding artwork on both
  supported architectures.
- Retains Classic theme, Classic Start menu/taskbar, Classic Control Panel,
  sliding/fading animation, classic logon, wallpaper, sound, independent
  folder sound, Explorer pane, caption presets, and exact-state restoration.
- Adds strict installer and application gates for the two supported profiles:
  XP Professional x86 SP3 and XP Professional x64 Edition SP2.
- Rejects Home, Starter, Media Center, Tablet PC, Embedded, IA-64, Server
  editions, and unsupported service-pack levels before changing anything.
- Fixes native x86 Explorer COM exports and makes the Windows 2000 information
  pane coexist correctly with Explorer's optional Folders bar.
- Corrects Revert state reporting so reopened controls reflect the restored
  pre-Apply configuration.
- Verifies the x86 lifecycle from a clean snapshot: all 134 protected targets
  patched after reboot, all 134 originals restored by Revert, repeat Apply,
  and fail-closed uninstall restoration.
- Revalidates the same 3.0.0 dual-architecture build on XP Professional x64
  Edition SP2.

## 2.4.1

Initial public release of eXPerience2K.

- Supports Windows XP Professional x64 Edition SP2.
- Reproduces the Windows 2000-style resource conversion across native x64,
  WoW64, WinSxS common-controls, and Windows File Protection cache targets.
- Provides eleven individually configurable visual, shell, logon, wallpaper,
  Explorer, sound, and animation options.
- Adds independent Solid Navy and Blue Gradient caption presets for the secure
  logon prompt and signed-in desktop.
- Adds an experimental native x64 Windows 2000-style Explorer folder pane
  without replacing `explorer.exe`.
- Captures one immutable pre-Apply baseline and restores the exact original
  value, type, data, or absence for every managed option.
- Preserves protected-file backups, CRC verification, startup repair, and
  fail-closed uninstall restoration.
- Retains the exact Windows 2002 Professional branding artwork used by the
  original conversion assets.
- Excludes all Internet Explorer executable/icon and boot-screen experiments.
- Refuses installation unless the host is AMD64 Windows XP Professional x64
  Edition SP2 (NT 5.2 build 3790, workstation product type, SP2), with a clear
  support-status dialog before any files or settings are changed.
- Adds a bottom-row **Revert** button with a default-No confirmation dialog.
  Revert restores every managed change to the immutable pre-Apply baseline
  while keeping the application and baseline available for later use.
- Makes the configuration window natively resizable from its edges and
  corners, with responsive control widths and a right-side vertical scrollbar
  whenever the full interface does not fit the available height.

All Windows XP x86 editions, other XP x64 editions, and all Windows Server
editions are rejected by this release. Windows XP Professional x86 support is
planned for the next major update.
