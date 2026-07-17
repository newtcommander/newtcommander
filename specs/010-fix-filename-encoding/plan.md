# Implementation Plan: Complete Revision of File Name and Path Display Encoding

**Branch**: `010-fix-filename-encoding` | **Date**: 2026-07-17 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/010-fix-filename-encoding/spec.md`

## Summary

Close the remaining mojibake surfaces left after features 004 (UTF-8
in `char*`, convert at OS boundaries) and 005 (dialog text fields).
The two reported defects are precisely located: the Directory Line /
Info Line (`CStatusWindow`, `src/stswnd.cpp`) draws raw UTF-8 through
`ExtTextOutA` and does all ellipsis/hot-track math in bytes (005's
deferred audit item A2 — explains both the garbled accents and the
dropped trailing component); the Alt+F5 Pack dialog fills its packer
combo with ANSI `CB_ADDSTRING` on UTF-8 titles
(`src/dialogs3.cpp:1845`), a site the 005 audit missed. The fix
strategy is to finish the 005 deferred tail with the same proven
boundary-conversion pattern: rebase `CStatusWindow` text handling on
UTF-16 (convert once, measure/index/draw wide), convert the Pack/
Unpack/config packer surfaces via the `Sal*U8` helpers, convert the
remaining owner-draw chrome (menus, toolbars, tooltips), fix the
partial plugins (ftp, sftp, regedt), reset legacy-version packer
config sections to defaults behind a `THIS_CONFIG_VERSION` gate (per
clarification), and verify everything through a documented
manual-walkthrough audit inventory (~80–100 candidate sites in
~15–18 files). Full evidence and decisions in
[research.md](research.md).

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI (ANSI build — no `UNICODE`/`_UNICODE`); internal helpers `src/common/salunicode.{h,cpp}`, `src/common/winlib.{h,cpp}` (`Sal*U8` window-text helpers), `src/plugins/shared/splunicode.h` for plugins; no new external dependencies
**Storage**: Windows Registry (REG_SZ; UTF-8↔UTF-16 via `SalRegSetValueExW8`/`SalRegQueryValueExW8`, `src/salamdr6.cpp:2306,2379`); config schema gate `THIS_CONFIG_VERSION` (`src/mainwnd2.cpp:143`, currently 104)
**Testing**: Build verification via `build.cmd` (Debug x64); manual walkthrough protocol with the feature-005 sample-name set, verdicts recorded in `surface-inventory.md` (per clarification); existing ASCII flows as regression guard
**Target Platform**: Windows 11+ (x64)
**Project Type**: Desktop application (WinAPI file manager) — existing monolithic `src/` + per-plugin trees
**Performance Goals**: No perceptible UI change; conversions O(length) executed on text *change*, not per paint (cache wide text) — bounded empirically by the panel drawing, which has paid this cost per item since 004
**Constraints**: ANSI build stays (004 decision — no `UNICODE` build, no UTF-8 ACP manifest); strict conversions with legacy-ANSI fallback on invalid UTF-8; no silent normalization (FR-006); plugin ABI unchanged
**Scale/Scope**: ~80–100 candidate display sites in ~15–18 files (core chrome `stswnd/menu*/toolbar*/tooltip`, pack family `dialogs3/dialogsp`, plugins ftp/sftp/regedt partial); audit inventory covers core app + 18 enabled plugins

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| # | Principle | Verdict | Evidence |
|---|-----------|---------|----------|
| I | Build Reproducibility | PASS | No build-system changes; code-only fixes inside existing projects; `build.cmd` remains the single entry point. |
| II | Backward Compatibility | PASS | Display fixes restore intended behavior (no functional change for ASCII, FR-008/SC-005). The one user-visible behavior change — resetting legacy packer config entries to defaults — is **gated behind a version check** (`ConfigVersion` < UTF-8 baseline → rebuild from defaults), exactly the mechanism the principle prescribes, and was decided in the spec clarification 2026-07-17. Healthy post-004 configs are not reset. |
| III | Incremental Modernization | PASS | Each surface converted independently with the established pattern; `CStatusWindow` rework confined to that class; no refactoring of adjacent untouched code; changes are reviewable/revertible per surface. |
| IV | Windows Platform Commitment | PASS | Pure WinAPI (`-W` API variants + existing helpers); no new dependencies, no abstraction layers. |
| V | Plugin Architecture Preservation | PASS | Plugin fixes (ftp/sftp/regedt) use the shared `splunicode.h` helpers; no plugin interface changes; core helper additions (e.g. a listbox `U8` helper) are additive. |
| VI | UI Consistency | PASS | No visual restyling — identical fonts/metrics, only correct glyphs; no `InitCommonControlsEx(ICC_STANDARD_CLASSES)`, no manifests, no control subclassing for style. |

**Post-Phase-1 re-check (2026-07-17)**: All six principles still PASS —
the design (data-model.md, contracts/display-conversion-contract.md)
introduces no new projects, dependencies, or process-wide visual
changes.

## Project Structure

### Documentation (this feature)

```text
specs/010-fix-filename-encoding/
├── plan.md                 # This file
├── research.md             # Phase 0: root causes, decisions R1–R9
├── data-model.md           # Phase 1: entities (surfaces, inventory, config gate)
├── quickstart.md           # Phase 1: build + walkthrough verification protocol
├── contracts/
│   └── display-conversion-contract.md  # Rules every display site must satisfy
├── surface-inventory.md    # Audit inventory (created/maintained during implementation)
└── tasks.md                # Phase 2 output (/speckit.tasks — NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
src/
├── stswnd.{h,cpp}          # CStatusWindow: Directory Line + Info Line (P1) — UTF-16 rebase
├── tooltip.cpp             # CToolTip: wide measure/draw (A6)
├── dialogs3.cpp            # CPackDialog/CUnpackDialog combos (P2)
├── dialogsp.cpp            # Config → Pack/Unpack/External Archivers pages (B11)
├── mainwnd2.cpp            # THIS_CONFIG_VERSION bump + legacy packer-section reset gate
├── menu1.cpp, menu2.cpp, menu3.cpp, menubar.cpp   # Owner-draw menus (A4)
├── toolbar2.cpp, toolbar3.cpp, toolbar4.cpp        # Toolbars / hot-path bar (A5)
├── editwnd.cpp             # PathCompactPath(A) remnant in combo custom draw
├── salamdr2.cpp, salamdr3.cpp, dialogs4.cpp, dialogs5.cpp,
│   shellsup.cpp, codetbl.cpp, packac.cpp           # B17 / deferred misc — per-site verdicts
├── common/
│   ├── salunicode.{h,cpp}  # Existing primitives (unchanged)
│   └── winlib.{h,cpp}      # Sal*U8 helpers; add listbox variant if needed
└── plugins/
    ├── shared/splunicode.h # Plugin-side conversion helpers (existing)
    ├── ftp/                # Bookmark listbox (E1), log/operation windows
    ├── sftp/               # Log window, connect dialogs
    └── regedt/             # On-disk path fields (E5)
```

**Structure Decision**: Existing monolithic WinAPI application layout —
all changes are in-place conversions inside the files listed above; no
new projects, directories, or build targets. Reference (do-not-touch)
surfaces for the pattern: `src/fileswn4.cpp` (panel drawing),
`src/mainwnd1.cpp:1899-1904` (title), `src/salamdr4.cpp` (`CTruncatedString`).

## Complexity Tracking

> No Constitution Check violations — table intentionally empty.
