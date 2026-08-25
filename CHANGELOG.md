# Changelog

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
