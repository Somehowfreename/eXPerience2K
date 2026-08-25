# Branding and visual resources

Version 2.4.1 uses the exact Windows 2002 Professional branding artwork from
the original conversion resource set. Branding is intrinsic to the visual
resource conversion and is not a selectable or customizable field in the
configuration window.

The same visual family is used wherever the original resource conversion
provided branding artwork, including the Start-menu side banner, About
Windows/Winver surfaces, System Properties resources, and the native classic
logon presentation where the corresponding resources are used.

## Why it says 2002

The project currently favors fidelity to the proven original artwork rather
than shipping the discarded hand-edited 2005 variants. Earlier development
experiments attempted year-specific replacements, but those assets are not in
the v2.4.1 release path.

## Resource scope

The manifest contains 672 operations across 147 logical targets:

- 503 icon-group operations;
- 140 bitmap operations;
- 18 AVI shell-animation operations; and
- 11 string-resource operations.

This includes desktop and shell surfaces, Start-menu and Control Panel assets,
Explorer and common-dialog resources, system-properties artwork, networking
surfaces, common controls, and the original file-operation animations covered
by the resource corpus.

The resource conversion does not replace the XP shell architecture, Luna
theme engine, kernel, boot screen, or `explorer.exe`.

## Rights

Many visual resources are derived from historical Microsoft Windows artwork.
The repository code license does not grant rights to those assets. See
[LEGAL.md](LEGAL.md) and [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md).
