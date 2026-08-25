# Testing guide

Because eXPerience2K modifies protected system resources, full lifecycle
testing belongs in a disposable virtual machine snapshot.

## Required platform

- English Windows XP Professional x64 Edition SP2;
- administrator account available;
- 32-bit color display driver for gradient-caption validation; and
- audio enabled for sound validation.

Server editions, other XP x64 editions, IA-64, and XP x86 are negative-test
platforms only. Each must receive the support-status message before the
installer UI and must remain unchanged.

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

## Revert button

1. Before the first Apply, click **Revert** and confirm that the application
   reports that no saved pre-Apply setup exists.
2. Apply all features and restart.
3. Click **Revert**, choose **No**, and confirm that nothing changes.
4. Click **Revert** again, choose **Yes**, and confirm that the progress dialog
   remains responsive until the complete restoration finishes.
5. Restart and compare every managed resource and setting with the original
   baseline.
6. Reopen eXPerience2K and confirm the controls reflect the restored current
   state; first-launch defaults must not silently reappear.
7. Apply a subset of features, Revert again, and verify the same baseline is
   restored rather than a state captured after the earlier Revert.
8. Run Revert without administrator privileges and confirm it makes no changes
   and gives the Run As guidance.

## Resizable configuration window

1. Drag every edge and corner and confirm the native resize cursor and sizing
   behavior work without clipped or overlapping controls.
2. Confirm the window will not become narrower or shorter than its usable
   minimum size.
3. Make the client area shorter than the full interface and confirm a native
   vertical scrollbar appears on the right.
4. Exercise the scrollbar arrows, track, thumb, Page Up/Page Down, and mouse
   wheel; every control must remain reachable.
5. Increase the height until all visible content fits and confirm the main
   scrollbar disappears and returns to the top position.
6. Widen and maximize the window and confirm status text, checkboxes, caption
   controls, bottom buttons, and the diagnostic log resize without overlap.
7. Click **Open log** in the compact window and confirm it changes to **Hide
   log**, the log becomes visible by scrolling into view, and hiding it returns
   the interface to the top.

## Independent option tests

- Slide and Fade must automatically clear one another.
- Logon and signed-in caption presets must remain independent.
- Folder double-click sound must work with the broad sound conversion both on
  and off.
- Clearing one feature must not restore or disable another selected feature.
- Full Revert must restore all features without deleting the saved baseline.
- First-launch defaults must not reassert after the user saves a different
  configuration.
- User-scoped changes under **Run As** must target the interactive account.
- Limited-account Apply must stop cleanly before machine changes when an
  administrator-only feature is requested.

## Strict operating-system refusal tests

Test at least Windows XP Professional x86, Server 2003 x86, Server 2003 x64,
and Server 2003 R2 x64. Launch the installer and confirm on each platform:

- the message states that only Windows XP Professional x64 Edition SP2 is
  currently supported;
- it explains that other XP x64 editions, every Windows Server edition, and
  all XP x86 editions are unsupported;
- it states that Windows XP Professional x86 support is planned for the next
  major update;
- it includes `https://github.com/Somehowfreename/eXPerience2K` and advises the
  user to keep checking for an update;
- the installer exits after the dialog; and
- no installation directory, uninstall entry, shortcut, file replacement, or
  managed registry state is created.

## Update persistence

The retail validation path applied eXPerience2K immediately after a clean XP
x64 installation, installed the available operating-system updates with all
required restarts, and confirmed that the conversion remained present. Repeat
this scenario when changing protected-target logic or the reloader.
