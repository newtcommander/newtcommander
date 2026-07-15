# Implementation Plan: Correct Display of Unicode File Names in Dialogs and Text Fields

**Branch**: `005-fix-unicode-display` | **Date**: 2026-07-15 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/005-fix-unicode-display/spec.md`

## Summary

Feature 004 made internal narrow name/path buffers UTF-8 and converted
the hot UI paths (panels, titles, `CTransferInfo::EditLine`) to W
APIs, but a long tail of UI crossings still pushes raw UTF-8 through
ANSI text APIs — most visibly the F2 Rename dialog's history-combo
branch, which mojibakes `č-dir` into `ÄŤ-dir` (NFC) / `cĚŚ-dir` (NFD)
and corrupts read-back. This feature completes the 004 contract at the
UI boundary: a systematic audit (done in Phase 0, [research.md](research.md))
classified every name-bearing text surface; the fix repairs the shared
choke points (`CMessageBox`, `CStatusWindow`, `CStaticText`, custom
menus/toolbars/tooltips, history-combo helper, `CTruncatedString`,
command line, plugin-shared `winliblt`/`HistoryComboBox`), converts
the remaining direct message sites via new central `…U8` helpers, and
removes the signed-char name-validation defect that would otherwise
reject valid Unicode names on input (D1–D8 in research.md).

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI; feature-004 conversion layer (`src/common/salunicode.h` `SalU8ToW*`/`SalWToU8*`; plugin SDK `src/plugins/shared/splunicode.h` `SplU8ToW*`/`SplWToU8*`); no new external dependencies
**Storage**: Windows Registry (history rings as `REG_SZ`, UTF-8 contract from 004 R9; invalid-UTF-8 entries dropped at load per D6)
**Testing**: No automated UI test infrastructure; manual verification matrix in [quickstart.md](quickstart.md) against live fixtures (`%TEMP%\salamander-test`, NFC/NFD `č-dir` pair verified 2026-07-15); `PostMessageW`-driven panel automation where UIPI permits (004 precedent); full-solution build check
**Target Platform**: Windows 11+ x64 (x86 shell extension unaffected — no UI text code there)
**Project Type**: Native desktop application — 90-project MSBuild solution; this feature touches the core app + shared plugin sources (winliblt/lukas) + individual plugin dialogs
**Performance Goals**: No perceptible UI change for ASCII names; per-crossing UTF-8→UTF-16 conversion is O(len) on user-interaction paths (dialogs, menus, status line) — negligible; status-line/menu paint paths convert per paint, same as the 004 panel-draw approach that met SC-009
**Constraints**: No plugin ABI change (winliblt/lukas are compiled-in shared sources; interface 104 semantics unchanged); ANSI fallback for invalid UTF-8 preserved (004 transitional rule); no process-ACP manifest change (004 R3)
**Scale/Scope**: Audit inventory: 9 shared framework surfaces (A1–A9), 17 core dialog groups (B1–B17), 3 signed-char validation sites (C1–C3), 2 plugin-shared helpers + SDK doc (D1–D3), ~10 plugin-specific groups (E1–E10) — full inventory in research.md §Audit

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| # | Principle | Evaluation | Status |
|---|-----------|------------|--------|
| I | Build Reproducibility | Pure source changes; no new projects, manifests, or manual steps; `build.cmd full` stays the single-command build | PASS |
| II | Backward Compatibility | Defect fix: previously *garbled/corrupting* inputs start working; ASCII behavior is regression-guarded (SC-004, quickstart #20 + reference surfaces research.md §F). No plugin ABI change; `CSalamanderGeneral` signatures unchanged; third-party binaries unaffected | PASS |
| III | Incremental Modernization | Program-wide tail, but delivered as independently buildable increments (see Increment Sequencing); each touches only the audited defective sites; no adjacent refactoring (per-plugin ad-hoc `SetDlgItemTextU8` copies are left working; only new call sites use the central helpers) | PASS (justified) |
| IV | Windows Platform Commitment | Pure WinAPI (`W` messages/APIs, `MultiByteToWideChar` via existing helpers); no new dependencies | PASS |
| V | Plugin Architecture Preservation | Shared plugin sources fixed once (winliblt, lukas, SDK-exposed history helper); SDK contract documented before modification ([contracts/ui-text-contract.md](contracts/ui-text-contract.md)); bundled plugins rebuilt in-solution | PASS |

**Post-Phase-1 re-check**: design artifacts introduce no new
violations; the single justified item (III) stands with its increment
sequencing mitigation. GATE: PASS.

## Architecture Decisions (summary)

Authoritative rationale in [research.md](research.md):

- **D1** Fix shared choke points first; per-site conversion only for direct messages
- **D2** Central `…U8` window-text helpers (core `Sal*`, plugin SDK `Spl*`-based)
- **D3** Read-back always wide + `SalWToU8` (3× sizing, EditLine fallback semantics)
- **D4** Unsigned-byte rule for name-validation loops (`*s >= 32` defect)
- **D5** `CTruncatedString` measures/truncates on UTF-16 (no torn UTF-8)
- **D6** History rings UTF-8; invalid entries dropped at registry load
- **D7** Browse common dialogs switch to W structs/calls
- **D8** Plugin shared UI layer fixed once; SDK contract note in spl_gen.h

### Increment Sequencing (constitution III)

1. **Helpers + history plumbing** — `SalSetWindowTextU8`/`SalGetWindowTextU8`/`SalSetDlgItemTextU8`/`SalGetDlgItemTextU8`/`SalComboAddStringU8` (src/common); fix `LoadComboFromStdHistoryValues` (A7), `CTruncatedString` (A8), history load-drop rule (D6). Build-green, no visible change yet.
2. **US1 — the reported bug** — `CCopyMoveDialog::Transfer` history branch (B1), `RenameFileInternal` validation loop (C1). Verifies SC-001/SC-003 on the live fixtures (quickstart #1–#6).
3. **Core dialogs** — B2–B14 transfer/message sites + B16 browse-W (D7) + B17 UNCERTAIN resolution; `CMessageBox` (A1); list controls B15. Verifies quickstart #7–#11, #16.
4. **Core chrome** — `CStatusWindow` (A2), `CStaticText` (A3), menus (A4), toolbars (A5), tooltips (A6), command line (A9). Verifies quickstart #14–#15, #17–#18.
5. **Plugin shared layer** — winliblt `EditLine` (D1), lukas `HistoryComboBox` (D2), zip.cpp cache-name loops (C2/C3), SDK doc note (D3), shared `SetDlgItemTextU8`/`GetDlgItemTextU8` for plugins (D2). Full-solution rebuild (35 plugins).
6. **Plugin-specific sites** — E1–E8 fixes + E9 UNCERTAIN sweep (ftp listbox, regedt path fields, wmobile, pictview loop, demoplug sample, archiver tail). Verifies quickstart #12–#13, #19.
7. **Hardening & closure** — audit-inventory closure pass (every DEFECTIVE row → fix ref + ✓ with the sample-name set), full quickstart matrix incl. ASCII regression (#20), Release build check.

Each increment leaves the app fully functional and buildable; 2–7 are
independently revertible.

## Project Structure

### Documentation (this feature)

```text
specs/005-fix-unicode-display/
├── plan.md              # This file
├── spec.md              # Feature specification
├── research.md          # Phase 0 — root cause, decisions D1–D8, audit inventory
├── data-model.md        # Phase 1 — encoding contract, surface/audit model
├── quickstart.md        # Phase 1 — build, fixtures, verification matrix
├── contracts/
│   └── ui-text-contract.md   # UI text boundary rule + SDK compatibility note
└── tasks.md             # Phase 2 (/speckit.tasks — not created here)
```

### Source Code (repository root)

```text
src/
├── common/
│   ├── winlib.{h,cpp}         # MODIFIED: new Sal*U8 window-text helpers (D2)
│   └── salunicode.h           # unchanged (conversion primitives from 004)
├── salamdr6.cpp               # MODIFIED: LoadComboFromStdHistoryValues → W (A7)
├── salamdr4.cpp               # MODIFIED: CTruncatedString wide-safe (A8)
├── msgbox.cpp                 # MODIFIED: CMessageBox W text (A1)
├── stswnd.cpp, stswnd.h       # MODIFIED: directory/info line wide draw+measure (A2)
├── gui.cpp, gui.h             # MODIFIED: CStaticText wide draw+measure (A3)
├── menu1.cpp, menu2.cpp, menu3.cpp, menubar.cpp  # MODIFIED: menu text W (A4)
├── toolbar2.cpp, toolbar3.cpp, toolbar4.cpp      # MODIFIED: toolbar text W (A5)
├── tooltip.cpp                # MODIFIED: tooltip draw W (A6)
├── editwnd.cpp                # MODIFIED: command-line combo W (A9)
├── dialogs.cpp, dialogs2-6.cpp, dialogsp.cpp     # MODIFIED: B1–B17 sites
├── finddlg1.cpp               # MODIFIED: Find field read-backs (B7)
├── fileswn5.cpp               # MODIFIED: RenameFileInternal loop (C1)
├── zip.cpp                    # MODIFIED: cache-name loops (C2, C3)
├── execute.cpp                # MODIFIED: browse helpers → W (B16/D7)
├── plugins/
│   ├── shared/winliblt.cpp    # MODIFIED: EditLine(char*) → U8⇄W (D1)
│   ├── shared/lukas/utildlg.cpp  # MODIFIED: HistoryComboBox → U8⇄W (D2)
│   ├── shared/spl_gen.h       # MODIFIED: doc comments (D3)
│   └── ftp/, regedt/, wmobile/, pictview/, undelete/, filecomp/,
│       renamer/, dbviewer/, demoplug/, …          # MODIFIED: E1–E9 sites
└── (no .vcxproj changes — no new files planned; helpers live in
    existing winlib/salamdr translation units)
```

**Structure Decision**: single native-app codebase; changes are
confined to the audited defective sites listed above plus the new
helpers in existing `src/common/winlib.{h,cpp}`. No new projects or
build inputs.

## Complexity Tracking

> Constitution Check has no violations requiring justification beyond
> the sequencing note recorded inline (Principle III). No entries.
