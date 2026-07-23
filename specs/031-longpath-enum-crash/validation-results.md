# Validation Results: 031 — Directory-Listing Crash on Long Multi-Byte Names

**Date**: 2026-07-23 | **Branch**: `031-longpath-enum-crash`
**Machine**: Windows 11 Pro 26200, VS2022 17.10, Windows SDK 10.0.26100

## SC-001 — The reported crash is gone

- Root cause proven from the user's own WER dumps (5 dumps, 2026-07-23
  10:40–10:48, all `FAST_FAIL_STACK_COOKIE_CHECK_FAILURE` in
  `CFilesWindow::DrawIcon`, analyzed with DbgEng against the matching PDB —
  full stack in research.md R0).
- After the fix, automated live smoke (start app with `-l`/`-r` panel paths,
  drive with `WM_COMMAND`, verify liveness + WER dump count + `PrintWindow`
  screenshots):
  - Debug x64: `D:\Temp` in both panels — PASS (previously an instant crash);
    entering the 223-char path inside the long-named directory — PASS;
    view modes Icons/Thumbnails/Tiles/Detailed cycled — PASS.
  - Release x64 (the user's configuration): `D:\Temp` both panels + all five
    view-mode switches — PASS; panel opened inside the long directory — PASS.
  - WER dump count unchanged (7 before = 7 after) across every run.
- Screenshots (session artifacts): Detailed both panels, Thumbnails and
  Tiles with correct diacritics + ellipsis after the R2c fix (before it, the
  labels rendered as mojibake).

## SC-002 — Operation surface

- `saltests` on-disk coverage: create/enumerate/remove the exact 215-char
  repro directory; create/write/attributes/enumerate/rename/copy/delete at
  a > 300-char deep Unicode path (existing `TestFileIO`); byte-exact name
  retrieval (330 B) asserted.
- Fail-safe path proven: an undersized conversion target yields an empty
  string, never a truncated name (`TestLongComponentNames`).
- Interactive dialog-driven matrix (F5 copy dialogs, clipboard paste into
  Explorer, …) cannot be keyboard-driven in this environment (`SendInput`
  is blocked for the session); the underlying engine routes were validated
  by features 012/027 and their saltests coverage remains green. The manual
  GUI walkthrough is left to the user, consistent with 027 precedent.

## SC-003 — Regression fence proves itself

- Revert experiment executed: `fileswn4.cpp` `fileName` buffer temporarily
  set back to `MAX_PATH + 4` → build **fails**:
  `error C2338: static_assert failed: '031: buffer must hold a worst-case
  UTF-8 name component'` — then restored. Every fixed site carries the same
  `static_assert` fence.
- `saltests` gained 31 checks (`TestLongComponentNames` + repro-name
  on-disk block): byte-length invariants (215 diacritics chars = 330 B >
  MAX_PATH+4; 255 × 3-byte chars = 765 B fits `SAL_FIND_NAME_U8`; surrogate
  pairs), `SalConvertFindDataW` max-length round-trip + small-buffer
  fail-safe, and physical enumeration of the repro name.

## SC-004 — No regressions

- `saltests`: **1106 checks, 0 failed** (baseline before changes: 1075/0;
  all 1075 pre-existing checks still pass).
- Debug x64: **BUILD SUCCEEDED**; Release x64 (`build.cmd full release`):
  **BUILD SUCCEEDED** — including `exif.dll`, which failed to link on this
  machine even at HEAD (pre-existing SDK 26100 issue, fixed en route —
  research.md R7).
- clang-format 17.0.3 (repo-standard binary): changed line ranges formatted;
  violation count on touched files equals the HEAD baseline (all remaining
  hits are pre-existing, none introduced).
- No new compiler warnings at the touched sites.

## SC-005 — Review closure

- research.md R2 (8 defect sites: 6 CRASH, 2 CORRUPTION) — all fixed.
- R2b: 8 flagged/neighbouring sites re-verified with recorded verdicts
  (1 hardened, 6 safe with evidence, 1 deferred out-of-scope route).
- R2c: follow-on mojibake defect found by live verification — fixed
  (UTF-8-boundary snapping in `SplitText`/`TruncateSringToFitWidth` +
  2 latent OOB reads), with one documented cosmetic limitation (byte-based
  width measurement in icon-mode labels).
- Plugin exposure bounded by `CSalamanderDirectory::AddFile` (≤ 255-byte
  names) — added guards are defense-in-depth (R2b).

## Known limitations / follow-ups

- Icon/Thumbnails/Tiles label *widths* are still measured with byte-wise
  ANSI metrics — multi-byte labels may sit slightly off-centre (glyphs are
  correct). Proper fix: wide-unit measurement in `SplitText`/
  `TruncateSringToFitWidth`.
- The 027 §Bounded backlog (truncating, non-crashing MAX_PATH sites outside
  the paint path, e.g. `fileswn5.cpp` quick-rename/selection buffers)
  remains open as before.
- Interactive GUI operation matrix on the repro entry — user walkthrough
  recommended (F5/F6 copy dialogs, Ctrl+C/Ctrl+V, rename), engine routes
  already covered by 012/027 + saltests.
