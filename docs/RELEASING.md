# Release process

1. Confirm the support matrix and version constants.
2. Run the clean XP x64 lifecycle and x86 refusal tests in `TESTING.md`.
3. Build with `scripts/build.ps1`.
4. Run `scripts/verify-release.ps1`.
5. Record the installer size and SHA-256 in the README, release notes, and
   release body.
6. Commit the exact source and documentation.
7. Tag the verified commit `v2.4.1`.
8. Create a non-draft, non-prerelease GitHub release and mark it Latest.
9. Attach only `eXPerience2K-v2.4.1-Setup.exe`.
10. Verify the repository visibility remains private until the project owner
    explicitly chooses to publish it.

Do not attach an independently generated source archive: GitHub automatically
provides source archives for the tagged commit. Do not attach checksums as a
second asset; place the SHA-256 in the release body and README so the Assets
section contains exactly the installer.
