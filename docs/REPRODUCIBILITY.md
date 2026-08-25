# Reproducibility and source attestation

The repository release tag contains the source, installer definition,
manifests, payload, reference inputs, and build script used for the published
v2.4.1 installer.

Before publication, the following build-input groups were compared against the
preserved exact-state-restore source lineage and then updated only for the
documented x64 refusal boundary:

```text
src/
installer/
payload/
reference-assets/
tools/
scripts/build.ps1
```

The final release-candidate build-input set contains 967 files. The
deterministic SHA-256 over sorted UTF-8 records of
`relative-path|file-sha256` is:

```text
d2fab9646c1d5a2a75526292f7df9e55d6f876f92b5e739df0c9b11887a7e928
```

Relative to the preserved exact-state-restore checkpoint, the executable
release path changes only the documented x64 support enforcement in
`installer/eXPerience2K.nsi` and `src/eXPerience2KConfig.c`: x86 is rejected
before installation or configuration, and the x86 core is no longer packaged.

The resulting installer is 4,307,201 bytes with SHA-256:

```text
B64CE6446CD1D52B6B8DCDF0F085DED3BAA18D66123D722005A2E196B93CFFE3
```

Build documentation, release automation, and public-facing documentation may
differ from the private development checkpoint because obsolete test naming
and unsupported-platform claims were removed. They do not introduce a second
source lineage.

Byte-identical rebuilds are not guaranteed across compiler, linker, NSIS, and
compression versions because generated PE and installer metadata may contain
timestamps. Verification therefore combines source/payload hashes, PE checks,
manifest checks, asset hashes, version metadata, and the exact release-asset
SHA-256.
