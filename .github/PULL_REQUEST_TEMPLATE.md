## Summary

Describe the user-visible and implementation changes.

## Compatibility

- [ ] The release path still targets Windows XP Professional x64 Edition SP2 / NT 5.2.
- [ ] Windows XP x86 is still rejected before installation changes anything.
- [ ] Build succeeds with warnings treated as errors.
- [ ] Resource, operation, and target counts are intentional and documented.
- [ ] No Windows binaries, product keys, installation media, or secrets were added.
- [ ] No kernel or boot-screen modification was added.

## Testing

- [ ] `scripts/verify-release.ps1` passes.
- [ ] Installation tested from a clean XP x64 SP2 snapshot.
- [ ] Both persistence reboots completed on the supported profile.
- [ ] The x86 refusal dialog was tested and left the system unchanged.
- [ ] Visual surfaces inspected.
- [ ] Persistence tested after an additional reboot.
- [ ] Uninstall and complete user/system restoration tested on XP x64.

Attach safe logs and screenshots. Do not attach Microsoft system binaries.
