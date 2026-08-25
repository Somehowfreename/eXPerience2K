# Support matrix

## Supported release platform

| Operating system | Architecture | Service pack | Status |
| --- | --- | --- | --- |
| Windows XP Professional | x86 | SP3 | Supported and tested |
| Windows XP Professional x64 Edition | AMD64/Intel 64 | SP2 | Supported and tested |

Version 3.0.0 was validated against English Windows XP Professional
x86 SP3, NT 5.1 build 2600. Its clean-snapshot lifecycle covered Apply, reboot
persistence, full Revert, repeat Apply, and uninstall restoration. The x64
release was validated against English Windows XP Professional x64 Edition SP2,
NT 5.2 build 3790, on clean retail and volume-license installations.

## Explicitly unsupported or untested

| Family | Status | Release behavior |
| --- | --- | --- |
| Windows XP x86 other than Professional SP3 | Unsupported | Installer exits before changing anything |
| Windows Server 2003, all editions | Untested and unsupported | Do not install |
| Windows Server 2003 R2, all editions | Untested and unsupported | Do not install |
| Windows XP IA-64 editions | Unsupported | Do not install |
| Other XP service-pack levels or specialized editions | Unsupported | Do not install |

The installer and configuration application both enforce the exact x86 SP3
Professional and x64 SP2 Professional profiles. There is no unsupported-mode
override.

## Language boundary

Only English media has been validated. The resource engine is data driven and
may encounter matching binaries on other languages, but that has not been
tested and is not supported for this release.

## Deliberate non-features

- no boot-screen or kernel patching;
- no replacement `explorer.exe`;
- no Internet Explorer executable/icon modification;
- no Server-specific branding; and
- no unsupported-mode override.
