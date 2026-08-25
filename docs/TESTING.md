# Testing guide

Because eXPerience2K64 modifies protected system resources, full lifecycle
testing belongs in a disposable virtual machine snapshot.

## Required platform

- English Windows XP Professional x64 Edition SP2;
- administrator account available;
- 32-bit color display driver for gradient-caption validation; and
- audio enabled for sound validation.

Server editions and XP x86 are not release-test platforms. The x86 negative
test verifies only that the installer displays the support-status message and
changes nothing.

## Clean-install lifecycle

1. Restore a clean XP x64 snapshot.
2. Record the original Classic/Luna theme, Start-menu/taskbar layout, Control
   Panel view, menu animation, Winlogon mode, captions, Explorer view, sound
   assignments, fonts/metrics, and relevant value absence.
3. Install the release candidate.
4. Open the configuration app with all default-on features selected.
5. Apply and restart.
6. Run the installed verifier and inspect the privacy-safe log.
7. Visually inspect the desktop, Start menu, All Programs, taskbar, title bars,
   Winver, System Properties, classic logon, Control Panel, Explorer folder
   views, common dialogs, and copy/move/delete animation surfaces.
8. Exercise logon, logoff, shutdown, alerts, notifications, folder navigation,
   and the double-click sound.
9. Restart again and repeat the verifier and visual checks to confirm
   persistence.
10. Reopen the configuration app and confirm it reflects current state.
11. Clear every feature, Apply, and restart.
12. Compare every managed setting and sound branch with the original baseline.
13. Uninstall and verify that no managed program or reloader state remains.

## Independent option tests

- Slide and Fade must automatically clear one another.
- Logon and signed-in caption presets must remain independent.
- Folder double-click sound must work with the broad sound conversion both on
  and off.
- Clearing one feature must not restore or disable another selected feature.
- First-launch defaults must not reassert after the user saves a different
  configuration.
- User-scoped changes under **Run As** must target the interactive account.
- Limited-account Apply must stop cleanly before machine changes when an
  administrator-only feature is requested.

## x86 refusal test

On Windows XP Professional x86, launch the installer and confirm:

- the message explicitly states that x86 is not currently supported and is in
  development;
- the message points users to the GitHub repository for updates;
- the installer exits after the dialog; and
- no installation directory, uninstall entry, shortcut, file replacement, or
  managed registry state is created.

## Update persistence

The retail validation path applied eXPerience2K64 immediately after a clean XP
x64 installation, installed the available operating-system updates with all
required restarts, and confirmed that the conversion remained present. Repeat
this scenario when changing protected-target logic or the reloader.
