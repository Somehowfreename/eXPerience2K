# Reproducibility and source attestation

The `v3.0.0` tag contains the source, installer definition, manifests, payload,
reference inputs, and build scripts used for the published 3.0.0 installer.

The release build-input groups are:

```text
src/
installer/
payload/
reference-assets/
tools/
scripts/
```

This set contains 969 files. The deterministic SHA-256 over sorted UTF-8
records of `relative-path|file-sha256`, with forward-slash paths and one LF
after every record, is:

```text
392ab8a0b7fb2fbef192c2529bafc77057baacd12fbc83d0f7605d31208628a2
```

Version 3.0.0 descends directly from the tested 2.4.1 exact-state-restore
source. Its functional changes add the native XP Professional x86 SP3 engine,
native x86 Explorer module, strict dual-profile selection, and shared
architecture-aware configuration paths while retaining the existing XP
Professional x64 Edition SP2 implementation.

The final installer is 4,334,250 bytes with SHA-256:

```text
5160B4B101528F1AF544BD9B00C7AEC99968391F98FEE4A65553123CD868958D
```

Byte-identical rebuilds are not guaranteed across compiler, linker, NSIS, and
compression versions because generated PE and installer metadata may contain
timestamps. Verification therefore combines source/payload hashes, PE checks,
manifest checks, asset hashes, version metadata, exact x86 COM export names,
strict operating-system gates, and the published installer SHA-256.
