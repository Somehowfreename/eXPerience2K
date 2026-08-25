# Experimental Windows 2000 Explorer folder interface

The optional Explorer feature is a native 64-bit integration for Windows XP
Professional x64 Edition SP2. It aims to make ordinary Explorer folder views
look and behave like the Windows 2000 folder interface while preserving XP
x64's shell implementation.

## Implementation

The feature installs `eXPerience2KExplorerBand64.dll`, a native x64 in-process
COM module, and a hash-verified Windows 2000 WebView asset tree. The module
inserts a headerless, 200-pixel Windows 2000-style information pane into native
`SHELLDLL_DefView` folder windows.

It does **not** replace or patch `explorer.exe`. XP continues to provide:

- folder namespace and navigation;
- context menus;
- drag and drop;
- copy, move, delete, and rename operations;
- shell APIs used by applications; and
- the Win+E folder tree.

The associated state selects Icons instead of XP's Tiles view and installs the
reference-derived cabinet, toolbar, address-bar, and per-folder WebView state.

## Assets

The Explorer payload includes the exact reference `wvleft.bmp`, 74 Web files,
eight state assets, and hash manifests. The separate resource conversion
continues to supply the Windows 2000-style shell animations and icons.

## Restoration

Explorer machine registration, user settings, Web files, and saved-folder-view
state are captured separately. Clearing the Explorer option restores the
original values and files without affecting unrelated selected features.

## Status

This feature is marked **experimental** in the user interface because it is an
in-process shell extension and because Explorer folder-state behavior can vary
with existing ShellBags and third-party shell extensions. It is restricted to
the supported XP x64 platform.
