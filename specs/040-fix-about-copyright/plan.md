# Implementation Plan: Fix About Dialog Copyright Notice

**Branch**: `040-fix-about-copyright` | **Date**: 2026-07-26 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/040-fix-about-copyright/spec.md`

## Summary

The About dialog takes both copyright lines from the active language module, so
every translation has been free to rewrite them — and all 11 did, stripping the
"Open Salamander Authors" attribution and, in 10 of them, dating it 2023.

The fix removes the notice from the translation pipeline entirely: the two
captions become empty in the English resource and in all 11 translation
archives, and `CAboutDialog::DialogProc` fills them at runtime from two
self-contained build-time constants — Newt Commander first, Open Salamander
second. The splash screen already paints the same constants and is reordered to
match. This is the pattern `IDC_ABOUT_WWW` already uses two lines above in the
same dialog.

## Technical Context

**Language/Version**: C++20 (`/std:c++latest`), MSVC v143 (VS2022); Win32
resource scripts (`.rc`); Python 3.13 for the translation-archive edit
**Primary Dependencies**: pure WinAPI — `SetDlgItemText`, existing
`CAboutDialog` / `CSplashScreen` in `src/logo.cpp`. No new dependencies.
**Storage**: N/A — build-time string constants only
**Testing**: manual visual verification of the About dialog and splash screen
per shipped language, plus scripted grep assertions over the translation
archives and a full build
**Target Platform**: Windows 11+ (x64)
**Project Type**: desktop application (two-panel file manager, pure WinAPI)
**Performance Goals**: N/A — two `SetDlgItemText` calls in `WM_INITDIALOG`
**Constraints**: `LegalCopyright` in the file-version resource must stay
byte-identical (FR-012); the `.slt` import is strictly positional, so no row may
be added or removed; UTF-8-BOM + CRLF must be preserved in every archive edited
**Scale/Scope**: 3 source files + 11 translation archives; ~15 changed lines of
C++/resource plus 22 archive rows blanked

## Constitution Check

*GATE: evaluated before Phase 0 and re-evaluated after Phase 1 design.*

| Principle | Assessment | Verdict |
|-----------|-----------|---------|
| I. Build Reproducibility | Change is source-only; no new build step, no manual step. `build.cmd full` remains the single command. | PASS |
| II. Backward Compatibility | User-visible text changes, which is the point of the bug fix; no registry, IPC, plugin-ABI or config surface touched. `LegalCopyright` metadata is explicitly preserved. | PASS |
| III. Incremental Modernization | Smallest change that fixes the defect: two `SetDlgItemText` calls, two blanked captions, one constant rename. Explicitly rejected adjacent refactors (dedicated control IDs, year-atom macros) — see research.md findings 5 and 6. | PASS |
| IV. Windows Platform Commitment | Pure WinAPI; no abstraction layer introduced. | PASS |
| V. Plugin Architecture Preservation | No plugin touched. Plugin `versinfo.rh2` files keep their own `VERSINFO_COPYRIGHT` and are out of scope. | PASS |
| VI. UI Consistency | `IDD_ABOUT` keeps its `DIALOGEX` / `DS_SETFONT \| DS_FIXEDSYS` / `FONT 8, "MS Shell Dlg"` declaration and its standard statics. No process-wide visual behaviour altered. Existing `WM_CTLCOLORSTATIC` theming is inherited rather than bypassed. | PASS |

**Result**: no violations. Complexity Tracking section omitted (nothing to
justify).

**Post-Phase-1 re-check**: the design adds no new components, no new
abstractions and no new files to the product; all six principles still pass.

## Project Structure

### Documentation (this feature)

```text
specs/040-fix-about-copyright/
├── spec.md              # Feature specification (clarified)
├── plan.md              # This file
├── research.md          # Phase 0 output — findings and rejected alternatives
├── data-model.md        # Phase 1 output — the two notice constants and their consumers
├── quickstart.md        # Phase 1 output — how to build and verify
├── contracts/
│   └── copyright-display.md   # UI contract: exact literals, order, sources
├── checklists/
│   └── requirements.md  # Spec quality checklist (16/16)
└── tasks.md             # Phase 2 output (/speckit.tasks)
```

### Source Code (repository root)

```text
src/
├── versinfo.rh2                 # display constants — restructured (FR-006)
├── logo.cpp                     # CAboutDialog::DialogProc  — set both lines (FR-002/003/005)
│                                # CSplashScreen::PrepareBitmap — swap order (FR-011)
└── lang/
    └── lang.rc                  # IDD_ABOUT: blank both captions (FR-009)

translations/
├── czech/salamand.slt           # blank both copyright rows (FR-009, FR-009a)
├── german/salamand.slt          #   "
├── french/salamand.slt          #   "
├── dutch/salamand.slt           #   "
├── hungarian/salamand.slt       #   "
├── romanian/salamand.slt        #   "
├── slovak/salamand.slt          #   "
├── spanish/salamand.slt         #   "
├── chinesesimplified/salamand.slt   # disabled language, still corrected (FR-010)
├── russian/salamand.slt             #   "
└── ukrainian/salamand.slt           #   "
```

**Structure Decision**: no new files and no new directories in the product. The
change is confined to the three source files that already own the notice
(`versinfo.rh2` defines it, `logo.cpp` displays it, `lang/lang.rc` declares the
controls) plus the translation archives that must be cleared of the stale
strings. This mirrors the existing split — `salamand.rc` holds the untranslated
splash template, `lang/lang.rc` holds the translated dialog templates — and adds
nothing to it.

## Phase 0 — Research

Complete. See [research.md](./research.md). Seven findings, no unresolved
NEEDS CLARIFICATION. Key outcomes:

- The defect is that `IDD_ABOUT` is loaded from `HLanguage` and nothing
  overrides the captions.
- The splash screen's empty-caption + build-time-constant pattern is the model,
  but the About dialog keeps real statics so it inherits the existing
  light/dark `WM_CTLCOLORSTATIC` theming.
- The archive format has no "do not translate" state, so an empty *source*
  string is how the notice is removed from the pipeline permanently.
- Clipping (FR-007) is structurally eliminated: geometry always regenerates from
  the English template and `layout.py` never shrinks, so every language inherits
  the English box.

## Phase 1 — Design & Contracts

Complete. Artifacts:

- [data-model.md](./data-model.md) — the two notice constants, their consumers,
  and the invariants that must hold after the change.
- [contracts/copyright-display.md](./contracts/copyright-display.md) — the UI
  contract: exact literals, line order, which surface reads which constant, and
  what must *not* change.
- [quickstart.md](./quickstart.md) — build and verification procedure.

No agent-context update is needed beyond a refresh: the feature introduces no
new technology.

## Implementation Approach

Four independent edits, each verifiable on its own:

1. **`src/versinfo.rh2`** — replace `VERSINFO_COPYRIGHT1/2` with
   `VERSINFO_COPYRIGHT_OPENSAL` / `VERSINFO_COPYRIGHT_NEWT`, both self-contained
   and both carrying `Copyright ©`. `VERSINFO_COPYRIGHT` is left byte-identical.
2. **`src/lang/lang.rc`** — blank the two `LTEXT` captions in `IDD_ABOUT`,
   keeping the controls, their IDs and their geometry.
3. **`src/logo.cpp`** — in `CAboutDialog::DialogProc`'s `WM_INITDIALOG`, set
   `IDC_STATIC_1` to the Newt constant and `IDC_STATIC_2` to the Open Salamander
   constant; in `CSplashScreen::PrepareBitmap`, swap the two `PaintText` calls to
   the same order.
4. **`translations/*/salamand.slt`** — blank the text of rows
   `1150,10,97,196,8` and `1151,10,108,196,8` in all 11 archives, preserving
   UTF-8-BOM, CRLF, row count and state flag.

Order matters only in that (1) must precede (3); the rest are independent.

## Risks

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| `LegalCopyright` accidentally altered by the constant restructuring | Low | FR-012 verified by comparing the built `.exe` version resource before/after; kept as a plain literal specifically to avoid this (research finding 5) |
| A `.slt` edit corrupts encoding or line endings and breaks the positional import | Low | Scripted byte-level edit; row count and BOM/CRLF asserted after the edit; full build of all 8 enabled languages is the real check |
| Blanked English caption confuses a future translator into "restoring" it | Low | Comment added at the `lang.rc` controls pointing at the runtime source |
| Text clipped in some language | Very low | Structurally prevented (research finding 4); still visually verified |
