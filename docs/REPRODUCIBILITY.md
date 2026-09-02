# Reproducibility and source attestation

The `v3.1.1` tag contains the source, installer definition, manifests, payload,
reference inputs, and build scripts used for the published 3.1.1 installer.

The release build and verification input groups are:

```text
src/
installer/
payload/
reference-assets/
tools/
scripts/
tests/
```

This set contains 975 files. The deterministic SHA-256 over sorted UTF-8
records of `relative-path|file-sha256`, with forward-slash paths and one LF
after every record, is shown below. File hashes use the committed Git blob
bytes (text normalized by `.gitattributes`), so checkout line-ending settings
do not change the attestation. Both per-file and aggregate hashes are lowercase.

```text
c78dc1a33257344f2109ea223abc7bbf55364dc37690103274eb9657e9bf11ca
```

Version 3.1.1 continues the published 3.1.0 source and the subsequent README
commit `5e5d93cac88650468f5e3ecfc31ecd430a35ba1d`, without rewriting that history.
The prior source and installer were preserved before development. Changes cover
the Explorer navigation/preview fixes, native preferences, configuration and
restoration fixes, loaded-component upgrade handling, tests, and documentation.
The resource engines, original artwork, wallpapers, sounds, and reference
assets are unchanged.

The final installer is 4,356,311 bytes with SHA-256:

```text
D9B4027A3F73104937035381F4E911DCEC75A59C2D23C5DDEBF49ADE2AF4CE7F
```

Byte-identical rebuilds are not guaranteed across compiler, linker, NSIS, and
compression versions because generated PE and installer metadata may contain
timestamps. Verification therefore combines source/payload hashes, PE checks,
manifest checks, asset hashes, version metadata, exact x86 COM export names,
strict operating-system gates, and the published installer SHA-256.

A separate build directory containing the same inputs passed the documented
full build and release verifier. Its independently generated installer is not
substituted for the installer tested in the XP VMs. Resource Hacker's generated
local preferences file is neither a build input nor part of the attestation.
