# Contributing

Thank you for helping improve eXPerience2K.

Before opening a pull request:

1. Work from a dedicated branch.
2. Preserve Windows XP Professional x64 Edition SP2 and NT 5.2 compatibility.
3. Keep the PE32 configuration front end compatible with XP x64's WoW64
   subsystem.
4. Build with warnings treated as errors and run
   `scripts/verify-release.ps1`.
5. Test installation, Apply, both reboot persistence, state reopening,
   verification, feature clearing, uninstall, and exact restoration from a
   disposable XP x64 snapshot.
6. Test the x86 refusal path and confirm it changes nothing.
7. Do not add a kernel patch, boot-screen modification, product key,
   installation image, telemetry, network dependency, or silent download.
8. Document every new target, resource operation, fallback, state value, and
   compatibility assumption.

Bug reports should include the XP x64 build and update level, whether the
system is English, the sanitized application log, `verification.tsv`, clear
reproduction steps, and useful before/after screenshots. Never upload Windows
system binaries, product keys, installation media, SIDs, account names, or
profile paths.

XP x86 and Windows Server reports are outside the supported 2.4.1 boundary.
Feature requests for those platforms are welcome, but should not be presented
as bugs in the x64 release.
