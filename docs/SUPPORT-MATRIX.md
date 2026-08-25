# Support matrix

## Supported release platform

| Operating system | Architecture | Service pack | Status |
| --- | --- | --- | --- |
| Windows XP Professional x64 Edition | AMD64/Intel 64 | SP2 | Supported and tested |

The release was developed and validated against English Windows XP
Professional x64 Edition SP2, NT 5.2 build 3790. It was tested on clean retail
and volume-license installations and remained applied across the complete
post-install update sequence on the retail test system.

## Explicitly unsupported or untested

| Family | Status | Release behavior |
| --- | --- | --- |
| Windows XP x86, all editions | Unsupported in 2.4.1 | Installer displays an x86 support-status message and exits before changing anything |
| Windows Server 2003, all editions | Untested and unsupported | Do not install |
| Windows Server 2003 R2, all editions | Untested and unsupported | Do not install |
| Windows XP IA-64 editions | Unsupported | Do not install |
| Other XP service-pack levels or specialized editions | Unsupported | Do not install |

Windows XP x86 support is planned for a future release. It is not enabled in
2.4.1 because the original x86 conversion already exists and the priority for
this project is the previously unsupported XP x64 platform.

## Important source-tree note

The repository contains x86 core source and profile material retained from
development and future-support work. The installer does not ship the x86 core,
and both the installer and configuration application enforce the x64 release
boundary. Source presence is not a support claim.

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
