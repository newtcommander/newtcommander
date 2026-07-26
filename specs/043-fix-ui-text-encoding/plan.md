# Implementation Plan: Fix UI Text Encoding in Language Selection and Rename Captions

**Branch**: `043-fix-ui-text-encoding` | **Date**: 2026-07-26 | **Spec**: [spec.md](./spec.md)

## Summary

Three reported surfaces, plus everything three independent surveys found sharing
their cause: a UTF-8 value handed to a legacy single-byte display call.

The repair splits into three mechanical shapes, each with a helper that
**already exists** in the codebase — no new abstraction is needed for the bulk of
the work:

| Shape | Repair |
| --- | --- |
| Composed caption: ANSI `LoadStr` template + UTF-8 name | `LoadStrU8` for the template (and `u8=TRUE` for the plural helpers), so the wide path already present in `dialogs3.cpp` / `msgbox.cpp` is finally taken |
| UTF-8 value → legacy list view | `SalListViewSetItemTextU8` |
| UTF-8 value → legacy window text / combo | `SalSetWindowTextU8`, `SalSetDlgItemTextU8`, `SalComboAddStringU8` |

Two gaps need new code: a status-bar helper (`SB_SETTEXT` has no U8 counterpart)
and a wide path for the drag image (`fileswn9.cpp`, which has none at all).

The feature also reverts one regression feature 042 introduced, and widens the
042 build guard so it describes the defect rather than the two examples it was
written around — that guard passed cleanly while all three reported defects were
present, which is precise evidence that it is too narrow.

## Technical Context

**Language/Version**: C++20 (`/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI; `src/common/` (`salunicode`, `winlib`)
**Storage**: N/A — no persisted format changes
**Testing**: `src/saltests/saltests.cpp` + `tools/check_encoding.py` (widened) + runtime verification in all 9 shipped languages
**Target Platform**: Windows 11+, x64
**Project Type**: Desktop application, single MSBuild solution
**Performance Goals**: No measurable change — every repair is on a dialog-construction path, not a hot loop. The single drawing site (drag image) runs once per drag.
**Constraints**: Plugin ABI 104 unchanged; plugin-visible behaviour bit-identical; no application-wide encoding conversion
**Scale/Scope**: 10 composed-caption sites, 1 list-view site, ~25 window-text/number sites, 1 drawing site, 1 regression revert, 1 guard widening

## Constitution Check

| Principle | Assessment | Verdict |
|---|---|---|
| **I. Build Reproducibility** | Guard runs from `build.cmd`; no manual steps added | PASS |
| **II. Backward Compatibility** | Repairs defects only. Plugin ABI untouched; `plugins3.cpp` gains a wide path without any interface change | PASS |
| **III. Incremental Modernization** | Each repair swaps one call for its existing U8 counterpart, independently revertible. No adjacent refactoring. The wholesale conversion stays rejected | PASS |
| **IV. Windows Platform Commitment** | Pure WinAPI throughout | PASS |
| **V. Plugin Architecture Preservation** | No plugin source or interface modified; plugin-metadata sites explicitly left alone | PASS |
| **VI. UI Consistency** | No dialog templates, fonts or control styles touched | PASS |

**Result: PASS, no violations.** Complexity Tracking omitted.

## Project Structure

```text
specs/043-fix-ui-text-encoding/
├── plan.md · spec.md · tasks.md
├── inventory.md            # FR-009 deliverable
├── validation-results.md   # FR-015 deliverable
└── checklists/requirements.md

src/
├── dialogs2.cpp      # language selector list      (reported)
├── dialogs4.cpp      # configuration language field (reported)
├── fileswn5.cpp      # F2 rename caption (reported) + attributes confirm
├── fileswn8.cpp      # F5/F6/F8 captions (reported)
├── fileswna.cpp      # F5/F6/delete on a plugin file system
├── fileswn7.cpp      # archive copy / pack / unpack captions
├── finddlg1/2.cpp    # Find caption, status bar, options fields, log ignore
├── plugins3.cpp      # plugin subject label — no wide path at all
├── dialogs.cpp       # overwrite-confirmation size/date/time
├── dialogs2/3/6.cpp  # number and size fields (UTF-8 locale separator)
├── fileswn9.cpp      # drag image — needs a wide path
├── dialogs5.cpp      # revert the feature 042 regression
├── common/winlib.*   # new SalStatusSetTextU8 helper
└── saltests/         # per-surface automated tests (FR-016)

tools/check_encoding.py   # widened guard (FR-011)
```

**Structure Decision**: Existing layout unchanged; one new helper alongside the
existing `Sal*U8` family.

## Implementation Phasing

| Stage | Content | Gate |
|---|---|---|
| **0** | Revert the feature 042 regression at `dialogs5.cpp` | plugin-name message correct again |
| **1** | Widen the guard (FR-011); run it against the pre-fix tree to prove it detects all three reported defects (SC-007) | guard reports the three sites |
| **2** | Reported surfaces: language list, configuration field, F2/F5/F6 captions | all reproduce zero defects, 9/9 languages |
| **3** | The rest of the composed-caption family (7 further sites) | each verified |
| **4** | Legacy window-text / combo / status-bar / number sites | each verified |
| **5** | Drag image wide path; `plugins3.cpp` subject | verified |
| **6** | Per-surface automated tests (FR-016); full regression matrix for features 004/005/010/041/042 in 9 languages | zero regressions |

Stage 1 precedes Stage 2 deliberately: a guard written *after* the fixes cannot
be shown to detect the defects, and that is exactly the weakness which let these
three through in the first place.

## Risks

| Risk | Mitigation |
|---|---|
| Converting a template whose substituted value is **not** UTF-8 mixes the message the other way — the mistake 042 made at `dialogs5.cpp` | Every conversion is preceded by tracing the substituted value to its origin. Plugin metadata is never converted; those sites carry an annotation. |
| `ExpandPluralFilesDirs` and friends return ANSI and feed the same captions | They already take a `u8` flag (added by 041); pass `TRUE` wherever the template becomes `LoadStrU8`. |
| A caption becomes valid UTF-8 and takes the wide path for the first time, changing truncation behaviour | `CTruncatedString::TruncateText` already has a surrogate-safe wide path; taking it *is* the fix, and truncation is explicitly re-tested (FR-008). |
| English verification hides everything | All 9 languages for every composed or localized surface (FR-015). |
| Breaking something 041/042 repaired | The full regression matrix is a P1 user story, not a final sanity check. |

## Post-Design Constitution Re-check

No new projects, dependencies, plugin interface changes or UI style changes. One
new helper in the existing `Sal*U8` family, whose purpose is to make the
compatibility principle enforceable. **Still PASS.**
