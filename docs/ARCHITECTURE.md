# Architecture

eXPerience2K supports Windows XP Professional x86 SP3 and Windows XP
Professional x64 Edition SP2. It is divided into a 32-bit configuration front
end, native x86/x64 resource engines, architecture-matched Explorer extensions,
an NSIS installer, and data-driven payload manifests.

## Components

### Configuration application

`src/eXPerience2KConfig.c` builds as a PE32/NT 5.1 GUI program so it runs under
XP x64's WoW64 subsystem without .NET. It:

- detects the native processor architecture and exact supported XP profile
  before any feature logic runs;
- discovers the interactive desktop user independently of the administrator
  account supplied through **Run As**;
- detects each feature's current state;
- captures the immutable pre-Apply baseline;
- applies and restores user-scoped settings;
- invokes the architecture-matched resource engine for protected files;
- installs or removes the architecture-matched Explorer integration;
- keeps privacy-safe diagnostics in memory; and
- provides unattended maintenance entry points used by reload and uninstall.

### Resource engine

`src/eXPerience2KCore.c` builds as native PE32/NT 5.1 and PE32+/NT 5.2
executables.
It consumes `payload/operations.tsv`, `payload/targets.tsv`, and the resource
tree. Its responsibilities include:

- resolving native System32, WoW64, WinSxS, and Windows File Protection cache
  targets;
- backing up original files and recording CRC-32 state;
- staging and validating PE resource changes;
- applying icon groups natively and using the documented Resource Hacker
  fallback where needed;
- scheduling locked replacements at restart;
- reporting patched/original/changed/missing state; and
- reload, repair, inventory, verification, and complete restoration modes.

The installer includes both engines and the configuration application chooses
the one matching the detected supported operating system.

### Explorer integration

`src/eXPerience2KExplorerBand.cpp` builds as native x86 and x64 in-process COM
modules. The matching module adds the optional Windows 2000-style folder
information pane while retaining XP's original `explorer.exe`, namespace,
context menus, drag/drop, shell APIs, file operations, and Win+E folder tree.

The integration is explicitly marked experimental and has its own apply and
restore state. See [EXPLORER-EXPERIMENT.md](EXPLORER-EXPERIMENT.md).

### Installer and uninstaller

`installer/eXPerience2K.nsi` builds the NSIS installer. Its `.onInit` handler
checks the architecture, workstation edition, NT build, service pack, and
specialized-edition markers before displaying install pages. Only the two
documented Professional profiles proceed.

The uninstaller first invokes the configuration application's complete
restore path. It removes program files and recovery metadata only after that
transaction succeeds. Failure is fail-closed: the installation and backups
remain available for repair.

## Data model

- `payload/features.tsv` defines the eleven user-facing features and their
  first-launch defaults, scope, privilege requirements, and maturity.
- `payload/profiles.tsv` defines the exact XP Professional x86 SP3 and x64 SP2
  profiles accepted by version 3.0.0.
- `payload/targets.tsv` describes 147 logical protected-file targets.
- `payload/operations.tsv` describes 672 resource operations.
- `payload/Resources/` contains the icon, bitmap, animation, string, and
  branding payload.
- `payload/Sounds/` and `payload/Assets/Wallpapers/` hold the optional sound
  and wallpaper assets.
- `payload/ExplorerWeb/` contains the reference-derived WebView and pane state.

## Exact-state restoration

The first Apply transaction captures every managed setting before any selected
feature changes it. Baseline records preserve:

- whether a value existed;
- its registry type;
- its exact bytes or string data;
- the full shell-state binary block where applicable; and
- file/feature state needed for protected-resource and Explorer restoration.

Current-state detection is kept separate. Later Apply operations may update
the selected configuration, but they never replace the original baseline.

## Security boundary

The application has no network code and does not collect credentials. It does
modify protected operating-system files and uses an XP-era Windows File
Protection suspension routine requiring `SeDebugPrivilege` and Winlogon
interaction. Those techniques are invasive and can resemble malware behavior;
they are documented in [SECURITY.md](../SECURITY.md).

No boot-screen or kernel modification is included.
