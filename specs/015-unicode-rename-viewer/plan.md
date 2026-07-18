# Implementation Plan: Unicode Rename Field + Viewer Encoding

**Branch**: `015-unicode-rename-viewer` | **Date**: 2026-07-18 | **Spec**: [spec.md](spec.md)

## Summary

Two Unicode-at-the-ANSI-boundary fixes:

**Part A — Rename field `?` (tractable).** The Rename/Copy/Move/Edit-New dialogs
are ANSI windows (`CDialog::Execute` calls `DialogBoxParam` unless
`UnicodeWnd` is set), so the wide `SalSetWindowTextU8`/`SalComboAddStringU8`
calls added in features 004/005 are lossily down-converted to the ANSI code
page → `?`. Fix: make the shared `CCopyMoveDialog` family Unicode windows
(`DialogBoxParamW`) so those wide helpers work. Contained: thread a
`unicodeWnd` flag through `CCommonDialog` to `CDialog` and set it TRUE for
`CCopyMoveDialog`, `CCopyMoveMoreDialog`, `CEditNewFileDialog`.

**Part B — Viewer encoding (`viewer.md`, large).** The viewer paints raw bytes
1-per-column via `TextOutA` with an optional 256-byte code table; no
UTF-8/UTF-16/BOM notion. Keep the byte-offset sliding-buffer model and add:
(1) encoding detection on open (BOM → UTF-8 validation → legacy fallback);
(2) a decode-to-UTF-16 step for the *visible* lines; (3) wide rendering
(`ExtTextOutW`) + column/hit-test mapping over the decoded line; (4) two new
first-class encodings UTF-8 and UTF-16 LE/BE alongside the existing tables;
(5) report the actually-used encoding; (6) manual switch re-decodes.

## Technical Context

**Language/Version**: C++20, MSVC v143, ANSI build (no `UNICODE`)
**Primary Dependencies**: Pure WinAPI; `Sal*` unicode helpers; `MultiByteToWideChar`/`WideCharToMultiByte`; existing `CCodeTables`. No new deps.
**Storage**: n/a (viewer default-encoding config already exists)
**Testing**: `build.cmd` Debug + `build.cmd release`; static analysis; generated reference files; human walkthrough (headless)
**Target Platform**: Windows 11+ x64
**Project Type**: Desktop app, in-place changes
**Constraints**: keep byte-offset seek model (large files, search, long lines); no full extra in-memory copy; binary detection intact; zero ASCII regressions; dialog change limited to the CCopyMove family (avoid flipping every dialog); preserve feature-005 NFC-on-no-op rename.
**Scale/Scope**: Part A ~4 files (winlib.h forward, salamand.h CCommonDialog, dialogs.h/dialogs3.cpp ctors). Part B ~3 files (viewer2.cpp load/detect, viewer.cpp Paint render, viewer3.cpp/codetbl.cpp encoding menu/state) + test-file generator.

## Constitution Check

| # | Principle | Verdict |
|---|-----------|---------|
| I | Build Reproducibility | PASS — code-only, no dep/toolchain change |
| II | Backward Compatibility | PASS — ASCII identical; adds Unicode where it was lossy; legacy code tables kept |
| III | Incremental Modernization | PASS — reuse Sal helpers / CDialog Unicode support; viewer keeps its model + a decode layer |
| IV | Windows Platform Commitment | PASS — pure WinAPI wide text |
| V | Plugin Architecture Preservation | PASS — plugin dialog/viewer ABI unchanged (CCopyMoveDialog ctor gains an optional defaulted param; SalSplit/plugin exports untouched) |
| VI | UI Consistency | PASS — removes `?`/mojibake; encoding shown consistently with current design |

**Post-design re-check**: to confirm after Part A build (watch for any control that relied on ANSI behavior in the now-Unicode dialog).

## Project Structure

```text
specs/015-unicode-rename-viewer/
├── spec.md, plan.md, research.md, quickstart.md, tasks.md, checklists/requirements.md

src/  (Part A)
├── common/winlib.h        # CDialog ctor already has unicodeWnd param (verify) 
├── salamand.h             # CCommonDialog ctors: add unicodeWnd passthrough
├── dialogs.h              # CCopyMoveDialog/CEditNewFileDialog/CCopyMoveMoreDialog ctor decls
├── dialogs3.cpp           # those ctors: pass unicodeWnd=TRUE to CCommonDialog

src/  (Part B)
├── viewer.h               # encoding state fields, decode helpers
├── viewer2.cpp            # detect on open (BOM/UTF-8/legacy); decode; buffer notes
├── viewer.cpp             # Paint: wide render of visible lines (ExtTextOutW), column map
├── viewer3.cpp            # encoding menu items (UTF-8/UTF-16), manual switch, caption
├── codetbl.cpp/.h         # register UTF-8/UTF-16 as selectable encodings + detection
tools/ or a script         # generate reference test files (viewer.md)
```

**Structure Decision**: In-place. `research.md` carries the two root-cause
analyses (dialog ANSI-window; viewer byte pipeline map) as the audit
deliverable. No data-model/contracts phase.

## Phasing (implementation order)

1. **Part A** (rename `?`) — smallest, directly reported, build-testable. Ship first.
2. **Reference test files** — generate the `viewer.md` variants (also aids B).
3. **Part B viewer** — staged: (a) detection + state + caption/menu with
   decode of visible lines to wide + `ExtTextOutW`; (b) column/hit-test mapping;
   (c) manual switch + UTF-16; (d) binary-detection cooperation + perf guard.

## Complexity Tracking

> Part B is inherently complex (variable-width rendering over a byte model).
> Mitigation: decode only visible lines; keep byte offsets as the seek/anchor
> unit; build after each sub-step. If full variable-width hit-testing proves
> too risky to land safely without interactive testing, land detection + wide
> rendering + reported encoding + manual switch first (the core of `viewer.md`),
> and treat pixel-exact caret/selection on multi-byte lines as a follow-up,
> documented explicitly — never silently.
