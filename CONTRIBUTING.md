# Contributing

Thank you for helping improve eXPerience2K.

Before opening a pull request:

1. Work from a dedicated branch.
2. Preserve Windows XP Professional x86 SP3/NT 5.1 and Windows XP Professional
   x64 Edition SP2/NT 5.2 compatibility.
3. Keep the PE32 configuration front end compatible with both native XP x86
   and XP x64's WoW64 subsystem.
4. Build with warnings treated as errors and run
   `scripts/verify-release.ps1`.
5. Test installation, Apply, reboot persistence, state reopening,
   verification, feature clearing, uninstall, and exact restoration from a
   disposable x86 and x64 snapshots.
6. Test unsupported-edition and unsupported-service-pack refusal paths and
   confirm they change nothing.
7. Do not add a kernel patch, boot-screen modification, product key,
   installation image, telemetry, network dependency, or silent download.
8. Document every new target, resource operation, fallback, state value, and
   compatibility assumption.

Bug reports should include the XP architecture, build and update level, whether the
system is English, the sanitized application log, `verification.tsv`, clear
reproduction steps, and useful before/after screenshots. Never upload Windows
system binaries, product keys, installation media, SIDs, account names, or
profile paths.

Windows Server, specialized XP edition, and unsupported service-pack reports
remain outside the supported 3.0.0 boundary.
