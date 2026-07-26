# Quickstart: verify consistent SFTP dialog appearance

**Feature**: 009-sftp-dialog-style
**Date**: 2026-07-17

How to build and verify that SFTP dialogs look consistent with the rest of the application.
Verification is manual, side-by-side visual comparison (no automated check — per spec
clarification).

## Build

```bat
:: from repo root; OPENSAL_BUILD_DIR optional (defaults to .\build\)
build.cmd rebuild
```

Use `rebuild` (clean) so no stale plugin binary masks the result. Confirm `BUILD SUCCEEDED`
and that a fresh `sftp.spl` and `english.slg` are produced under
`build\newtcommander\Debug_x64\plugins\sftp\`.

## Verify — primary acceptance (focused text field)

1. Start the built Salamander.
2. Open the SFTP connect dialog (SFTP plugin → connect), tab into a text field so it has
   focus.
3. Open an FTP connect dialog (or a core dialog with a text field, e.g. F2 rename), focus a
   text field there.
4. **Pass condition**: the focused SFTP field and the focused FTP/core field show the same
   frame/focus decoration — no accent underline or modern frame unique to SFTP. Fonts match.

## Verify — order independence (deterministic within session)

1. In a fresh session, open an FTP/core dialog first and note its appearance.
2. Use the SFTP plugin (open its connect dialog), then open the FTP/core dialog again.
3. **Pass condition**: the FTP/core dialog looks identical before and after using SFTP; the
   SFTP dialog looks consistent regardless of what was opened first.

## Verify — all SFTP dialogs

Open each SFTP dialog and confirm the same font and control styling as the rest of the app:
connect, password/passphrase prompt, host-key verification, change permissions, symbolic
link, rename, the configuration page, and the logs window.

## If the difference persists

Follow the empirical spike in `research.md` (§ Empirical spike): confirm the loaded plugin
actually contains the fix, then instrument dialog init to log the thread DPI-awareness
context and the applied theme for an SFTP edit vs an FTP/core edit; the differing factor is
the trigger. Fix by removing that difference (research.md decision D2), not by adding an
SFTP-only theming override.

## Durability check

Confirm the project constitution contains the UI-consistency principle (principle VI) so
future dialogs inherit the house style and deviations are caught in review.
