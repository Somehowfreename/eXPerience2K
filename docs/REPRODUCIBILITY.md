# Reproducibility and source attestation

The `v3.1.0` tag contains the source, installer definition, manifests, payload,
reference inputs, and build scripts used for the published 3.1.0 installer.

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

This set contains 972 files. The deterministic SHA-256 over sorted UTF-8
records of `relative-path|file-sha256`, with forward-slash paths and one LF
after every record, is shown below. File hashes use the committed Git blob
bytes (text normalized by `.gitattributes`), so checkout line-ending settings
do not change the attestation. Both per-file and aggregate hashes are lowercase.

```text
2e86642c68c0f4c480d87e5bdb7ca62a6d95db2337aff7132039d8829659498f
```

Version 3.1.0 descends directly from the published 3.0.0 source at commit
`568188916cc2580f66ded4935f2646e631f6df13`. The prior source and installer were
preserved before development. The new version adds logon-background selection,
image conversion and corresponding UI, tests and documentation. The resource
engine, Explorer source, payload and reference assets are unchanged.

The final installer is 4,342,170 bytes with SHA-256:

```text
68314B8A8AF8F47772E4C4B239708784D6B8E95B6CC03ADA327B30ECB73FC9F8
```

Byte-identical rebuilds are not guaranteed across compiler, linker, NSIS, and
compression versions because generated PE and installer metadata may contain
timestamps. Verification therefore combines source/payload hashes, PE checks,
manifest checks, asset hashes, version metadata, exact x86 COM export names,
strict operating-system gates, and the published installer SHA-256.
