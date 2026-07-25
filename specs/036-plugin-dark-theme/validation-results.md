# Validation Results: Dark Theme for Plugin Windows and Dialogs (036)

**Date**: 2026-07-25 · Debug x64 (`build.cmd`), all builds 0 errors
**Evidence**: `audit.md` (per-plugin table + limitations) + session screenshots

## Success criteria

### SC-001 — 100% of audited plugin surfaces dark ✅ (with recorded OS-boundary exceptions)

Runtime-audited surfaces (SFTP Connect/Logs, FTP Connect, Renamer window +
confirm, DiskMap frame, plugin menus) all render dark. Every remaining
enabled-plugin surface flows through one of the two runtime-proven
mechanisms (winliblt central procs / raw-proc guard) — per-plugin table in
`audit.md`. Exceptions are the OS-drawn native menu bars on plugin frame
windows and OS common dialogs — the same boundary feature 028 accepted for
the core (spec: out of scope).

### SC-002 — Zero unreadable text ✅

All captured dark surfaces show light-on-dark text (edits, lists, buttons,
checkboxes, radio buttons, preview lists); no dark-on-dark or
light-on-light combination observed. The palette is 028's (contrast-tested
there); this feature adds no new color pairs.

### SC-003 — Switch without restart ✅

Live Dark→Default switch with a modeless plugin window open: no crash, no
mixed surface; open window kept its old consistent look; newly opened
plugin dialog matched the new theme; switch back to Dark identical.
(`t019-*`, `t020-sftp-connect-default.png`.)

### SC-004 — Default theme unchanged ✅

SFTP Connect under Default renders the classic light appearance
(`t020-sftp-connect-default.png`). All theme entry points are strict
passthroughs when Dark is off (`ThemeHandleCtlColor` returns FALSE,
`ThemeApplyToDialog/ToTopLevel` restore/no-op, `GetThemeSysColor` ==
`GetSysColor`) — the same guarantee 028 established, now reached through
delegation.

### SC-005 — Old plugins load unchanged ✅ (by construction + inspection)

The 6 new methods are appended at the vtable end of
`CSalamanderGeneralAbstract`; no existing slot moved (verified by reading
the diff — additions only, after `CloseAllOwnedEnabledDialogs`). The loader
version handshake is untouched; `LAST_VERSION_OF_SALAMANDER` = 105 with a
history row documenting the append. A ≤104 plugin never calls the new
slots, so its behavior is bit-identical.

## Gates (T021)

- `ICC_STANDARD_CLASSES` repo grep: no new occurrence (only mdview's
  pre-existing comment noting the constitution rule). No plugin gained a
  manifest.
- `spl_gen.h` method docs match `contracts/plugin-theme-api.md`;
  `spl_vers.h` history row present.
- clang-format applied to all 28 touched sources; final build green.

## Notes

- Runtime audit depth: 6 plugin surfaces opened live; the rest verified at
  mechanism level (see audit.md anchors) — the two mechanisms themselves
  are runtime-proven, and every remaining surface is a compile-checked user
  of one of them.
- `Theme Mode` left at 1 (Dark) in the user's configuration — matches the
  user's reported daily usage.
