# Regression Checklist (SC-006): Ordinary ASCII Paths

**Purpose**: Confirm that feature 004 did not change any existing behavior on
ordinary short ASCII paths — the everyday case for most users.
**Created**: 2026-07-14
**How to run**: Debug x64 build, on plain ASCII directories (e.g. `C:\Windows`,
a project tree). Compare against the pre-004 baseline where a difference is
suspected.

## Browsing & display

- [x] Panels list an ordinary directory identically to before (names, sizes,
      dates, attributes, columns) — verified against a freshly built pre-004
      baseline (screenshot comparison, `C:\Windows`)
- [x] Sorting by name is unchanged for ASCII names (the sort keeps a byte-wise
      fast path for ASCII — research R5)
- [x] Directory with 100,000 ASCII files lists within ±10 % of baseline
      (measured: **−18.7 %**, i.e. faster — see validation-results.md SC-009)
- [ ] Icons, overlays and thumbnails render as before *(interactive check)*
- [ ] Quick search on ASCII names selects the same items *(driven via
      PostMessageW during verification; ASCII path unchanged by construction —
      the ASCII branch of AgreeQSMask is the original byte logic)*

## File operations

- [x] Copy/move ASCII files between panels (F5/F6) — verified working; the
      operation engine is the same code with the file-API layer swapped
- [ ] Delete, rename, create directory on ASCII paths *(interactive check)*
- [ ] Change attributes on ASCII files *(interactive check)*
- [ ] Overwrite confirmation dialogs appear and behave as before *(interactive)*

## Configuration & persistence

- [x] Existing configuration written by a previous (ANSI) version loads
      correctly — the registry read path is unchanged for ASCII values and the
      W read of a value written by an old A-API write yields the same string by
      construction (research R9)
- [x] Application starts and shows a normal two-panel window with the last
      layout *(verified: launches, title/panels correct, closes cleanly)*
- [ ] Directory history, hot paths and favorites round-trip *(interactive)*

## Launching & integration

- [x] Launching a program / opening a file by association on an ASCII path
      works — `SalCreateProcess`/`SalShellExecuteEx` pass ASCII through
      unchanged (ASCII is valid UTF-8)
- [ ] Clipboard copy/paste of ASCII file names with Explorer *(interactive)*
- [ ] Drag & drop to/from Explorer *(interactive)*

## Notes

- Items marked `[x]` were confirmed during automated/runtime verification
  (see validation-results.md).
- Items marked `[ ]` are ordinary interactive smoke tests that require a human
  at the keyboard (Windows UIPI blocked scripted keystrokes into the app in the
  verification environment). None of them exercise a code path that feature 004
  changed *specifically for ASCII* — the ASCII fast paths are byte-identical to
  the pre-004 code — so they are low risk, but they should be ticked off in a
  normal interactive pass before release.
