# eXPerience2K 3.0.0

Version 3.0.0 adds complete Windows XP Professional x86 SP3 support while
retaining the existing Windows XP Professional x64 Edition SP2 implementation.
The same configuration application, eleven selectable features, state
detection, exact-state restoration, and fail-closed uninstall behavior are now
available on both supported architectures.

## Highlights

- Native XP Professional x86 SP3 protected-resource engine.
- Architecture-aware selection of the x86 or x64 engine at runtime.
- Native x86 and x64 Windows 2000-style Explorer integrations without
  replacing `explorer.exe`.
- Exact Windows 2002 Professional branding artwork on both supported systems.
- Classic theme with Tahoma 8 metrics and independent signed-in/logon caption
  presets.
- Classic Start menu/taskbar and Classic Control Panel defaults.
- Mutually exclusive sliding and fading Start-menu animations.
- Windows 2000-style classic logon window.
- Five optional Windows 2000-style wallpapers.
- Exact Windows 2000 sound mappings and an independently selectable folder
  navigation sound.
- Reopenable configuration that reflects the actual current state.
- Full Revert to the immutable pre-Apply baseline.

## Supported operating systems

- English Windows XP Professional x86 Service Pack 3, NT 5.1 build 2600.
- English Windows XP Professional x64 Edition Service Pack 2, NT 5.2 build
  3790.

The installer rejects Home, Starter, Media Center, Tablet PC, Embedded,
IA-64, every Windows Server edition, and unsupported service-pack levels before
extracting files or changing settings.

## Validation

The x86 release path was tested from a clean XP Professional x86 SP3 snapshot.
All eleven default features applied successfully, all 134 protected targets
were patched after reboot, the complete visual surface was inspected, and
Revert restored all 134 original targets with zero changed or missing files.
A second Apply and uninstall restoration were also verified.

The identical 3.0.0 installer was then tested successfully on Windows XP
Professional x64 Edition SP2, confirming that the existing x64 implementation
remained intact.

## Download

The release contains one binary asset:

```text
eXPerience2K-v3.0.0-Setup.exe
SHA-256: 5160B4B101528F1AF544BD9B00C7AEC99968391F98FEE4A65553123CD868958D
Size:    4,334,250 bytes
```

The executable is unsigned. Antivirus software may warn because eXPerience2K
modifies protected operating-system resources and temporarily suspends Windows
File Protection while applying requested changes. The project contains no
network code or telemetry.
