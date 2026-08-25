# Changelog

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
