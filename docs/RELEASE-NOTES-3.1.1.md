Version 3.1.1 fixes Explorer navigation, previews, toolbar defaults, and
Start-menu choice while retaining all eleven configuration options.

### Fixes

- The information pane's **My Network Places** link opens the native network
  folder instead of an Internet Explorer error.
- Selected images show proportional thumbnails. Supported audio and video
  files have the real Windows 2000-style legacy Media Player controls, without
  automatically starting playback. Explorer remains native x86 or x64; an
  isolated 32-bit helper hosts the legacy player on both systems.
- Explorer initially uses **Small icons** in the native Customize Toolbar
  dialog. Users can still change that preference.
- Native folder and toolbar preferences, including hiding file extensions,
  no longer make the active Explorer interface appear disabled in the app.
- Taskbar Properties retains both **Start menu** and **Classic Start menu**
  choices. The patcher no longer hides XP's choice with a policy, and removes
  its previously managed restriction without replacing the original backup.
- Applying Explorer preserves XP's native shell-state format and the separate
  Start-menu selection. Sign-in resource repair preserves saved folder views
  and user choices instead of reapplying initial Explorer preferences.
- Exact font/window-metric restoration no longer normalizes the saved raw
  registry values using measurements from the active theme.
- The installer can replace loaded application components at the next reboot
  when upgrading an existing installation.
- Uninstall removes the Add/Remove Programs entry from the correct registry
  view on XP x64 instead of leaving a stale entry after the files are removed.
- A new account no longer mistakes retained backups after Revert for active
  resource conversion or unnecessarily blocks current-user-only changes.
- Run As can identify the signed-in user even when XP blocks cross-account
  process-token access. An unverified account stops Apply/restoration safely.
  Menu animation choices are saved to that user's profile, not the Run As
  administrator's profile.

### Updating

Keep a system backup, close the configuration application, and run the new
installer. Restart if setup requests it, then open eXPerience2K and click
**Apply** with your desired options selected. Existing restoration backups
are retained; do not delete them to perform an upgrade.

Supported systems remain English **Windows XP Professional x86 SP3** and
**Windows XP Professional x64 Edition SP2**. Media formats depend on the
legacy player and codecs present on the system. No boot-screen or kernel
modification is included.

### Download

The only attached binary is `eXPerience2K-v3.1.1-Setup.exe`.

```text
SHA-256: D9B4027A3F73104937035381F4E911DCEC75A59C2D23C5DDEBF49ADE2AF4CE7F
Size: 4356311 bytes
```

The installer is unsigned. See the README for features, restoration, security
considerations, and supported-system limitations.
