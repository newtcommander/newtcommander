# Implementation Plan: Fix File Name Encoding in Find Results and Name Notices

**Branch**: `042-fix-find-results-encoding` | **Date**: 2026-07-26 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/042-fix-find-results-encoding/spec.md`

## Summary

Two reported defects, one class: a file name that is correct in storage is
destroyed at the last step before it is drawn. Research established both root
causes conclusively.

**Report 1 (Find results, `🙂` → `??`)** — `CDialog::CDialogProc` attaches the
dialog object only on `WM_INITDIALOG`, but a template list view asks its parent
for a notification format *before* that. `CFindDialog`'s `WM_NOTIFYFORMAT`
handler therefore has never run, the control settled permanently on ANSI
notifications, and the `LVN_GETDISPINFOW` handler added by feature 004 has been
dead code ever since. Fix: send `NF_REQUERY` from `WM_INITDIALOG` so the control
re-asks once the handler can answer.

**Report 2 (duplicate-name notice, `č` → `ÄŤ`)** — an ANSI `LoadStr` template is
spliced with a UTF-8 name; the composed string is not valid UTF-8, so
`CMessageBox` refuses its own wide drawing path and falls back to ANSI. Fix:
`LoadStrU8` at the composing call site, so the whole message is UTF-8 and the
existing wide path is accepted. `SalMessageBox` itself is not touched, which is
what keeps plugin behaviour bit-identical (FR-014/FR-014a).

Both fixes are small. The feature's weight is in the third deliverable: the
FR-009 inventory (~119 candidate sites measured), the repairs it drives, and the
FR-016 guard that makes the next recurrence fail the build instead of reaching a
user.

## Technical Context

**Language/Version**: C++20 (`/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI; internal `src/common/` (`salunicode`, `winlib`); no new external dependencies
**Storage**: N/A — no persisted state changes; file names are read from NTFS/exFAT/FAT and never written back by this feature
**Testing**: `src/saltests/saltests.cpp` (single-file suite, 10 existing test groups) + a new mechanical source check invoked from `build.cmd` + manual runtime verification against the fixture set
**Target Platform**: Windows 11+, x64
**Project Type**: Desktop application (two-panel file manager), single MSBuild solution
**Performance Goals**: No measurable regression; SC-009 requires a 10,000-row Find result set to scroll as it does today. The virtual list converts only visible rows, so per-row work is unchanged or slightly lower.
**Constraints**: Plugin ABI version 104 unchanged; plugin-visible behaviour bit-identical (FR-014/014a); no application-wide encoding conversion (explicitly deferred by feature 041 and out of scope per Clarifications)
**Scale/Scope**: 2 list-view sites (Report 1 class) + ~119 candidate composed-message sites across 30 files (Report 2 class), to be classified before repair per FR-009

## Constitution Check

*GATE: evaluated before Phase 0 and re-evaluated after Phase 1 design.*

| Principle | Assessment | Verdict |
|---|---|---|
| **I. Build Reproducibility** | The FR-016 guard is added to `build.cmd` as an automated step. No manual actions introduced; SC-013 explicitly requires no step a contributor could skip. | ✅ Pass |
| **II. Backward Compatibility** | Baseline Newt Commander 0.1.0. This repairs defects, it does not change intended behaviour. Plugin ABI 104 untouched; FR-014a forbids changing any shared entry point's default, so plugin output is bit-identical. FR-012 enumerates the behaviour that must not regress. | ✅ Pass |
| **III. Incremental Modernization** | Each fix is independently reviewable and revertible: one message at dialog init, one symbol per composed call site. The application-wide encoding conversion — the big-bang option — is explicitly rejected and deferred. Adjacent code is not refactored. New comments in English. | ✅ Pass |
| **IV. Windows Platform Commitment** | Pure WinAPI throughout; `NF_REQUERY`, `LVN_GETDISPINFOW` and `LoadStringW` are all platform primitives. No abstraction layer. VS2022/MSVC only. | ✅ Pass |
| **V. Plugin Architecture Preservation** | No plugin interface modified, no plugin source modified. Plugins are reviewed and their state recorded (FR-011), which *documents* the interface rather than changing it. | ✅ Pass |
| **VI. UI Consistency** | No dialog templates, fonts, control styles or `InitCommonControlsEx` flags are touched. `NF_REQUERY` affects only the notification direction control→parent, never rendering of standard controls. | ✅ Pass |

**Result: PASS, no violations.** Complexity Tracking section omitted as it would
be empty.

Note on Principle III: the tempting central fix — converting `LoadStr`
application-wide, or making `SalMessageBox` UTF-8-aware for everyone — is
precisely the big-bang this principle forbids, and feature 041 already
demonstrated it empirically by attempting it, breaking the Find dialog, and
reverting. The per-call-site approach is the constitutional one *and* the one the
evidence supports.

## Project Structure

### Documentation (this feature)

```text
specs/042-fix-find-results-encoding/
├── plan.md                     # This file
├── research.md                 # Phase 0 — root causes, inventory sizing, guard design
├── data-model.md               # Phase 1 — the text-encoding state model
├── quickstart.md               # Phase 1 — how to reproduce and verify
├── contracts/
│   ├── notification-format.md  # Contract for list views that render names
│   └── composed-message.md     # Contract for messages combining localized text with a name
├── checklists/
│   └── requirements.md         # Spec quality checklist (from /speckit.specify + /speckit.clarify)
├── inventory.md                # FR-009 deliverable — produced during implementation
└── validation-results.md       # FR-015 deliverable — produced during implementation
```

### Source Code (repository root)

```text
src/
├── finddlg1.cpp        # Report 1: NF_REQUERY from WM_INITDIALOG; retire the lossy ANSI path
├── packac.cpp          # Same dead-handler defect (R2) — same repair
├── fileswnb.cpp        # Report 2: the duplicate-name notice composition (line 815)
├── common/
│   ├── winlib.cpp/.h   # Read-only in this feature: explains why the handler never ran (R1)
│   └── salunicode.h    # Existing helpers; no new API expected
├── salamdr2.cpp        # LoadStr / LoadStrU8 / GetErrorText — reference, not modified
├── msgbox.cpp          # CMessageBox wide path — reference, NOT modified (FR-014)
├── saltests/
│   └── saltests.cpp    # FR-016(a) regression suite
└── <~30 files>         # FR-009 inventory-driven composed-message repairs

tools/
└── check_encoding.py   # FR-016(b) mechanical guard, invoked from build.cmd

build.cmd               # Wires the guard into the ordinary build (SC-013)
```

**Structure Decision**: The existing repository layout is used unchanged. This is
a defect repair inside a mature single-solution WinAPI application; no new
projects, directories or build targets are introduced beyond one guard script in
the existing `tools/` directory. Source, function and project names keep their
upstream identifiers per the project's standing rule.

## Implementation Phasing

Ordered so that the two reported defects are fixed and verifiable early, and so
that the inventory — the one open-ended item — is sized before any bulk work is
committed to.

| Stage | Content | Gate to proceed |
|---|---|---|
| **1** | Report 1: `NF_REQUERY` in `CFindDialog`; delete the `CP_ACP` conversion; extend `NF_REQUERY` to `packac.cpp`. Verify against the reported search. | Reported search shows all four names correctly; Path column, sorting, long paths unregressed |
| **2** | Report 1's second symptom: type-to-search (`LVN_ODFINDITEMW` becomes live; retire the ANSI comparison against UTF-8 names). | Type-to-search selects non-ASCII fixture names |
| **3** | Report 2: `LoadStrU8` at `fileswnb.cpp:815`. Verify in all 9 languages. | Notice reads `č-dir` in every shipped language, surrounding text still correct |
| **4** | **FR-009 inventory** — classify the ~119 candidates by both axes (mechanical + UI walk), record verdict and language class per entry. Write `inventory.md`. | Inventory complete and its size known **before** stage 5 begins |
| **5** | Repair every same-defect site the inventory identifies; verify per FR-009c's language matrix. | SC-007: every identified surface demonstrated correct in the running app |
| **6** | FR-016 guard: `saltests` suite + `tools/check_encoding.py` wired into `build.cmd`; demonstrate it failing on each reverted fix (FR-017). | Guard observed failing for both reported defects, then passing |
| **7** | FR-011 plugin review (record only, no modification); FR-013 re-verification of feature 041's surfaces; `validation-results.md`. | SC-005, SC-008, SC-008a satisfied |

Stage 4 is the explicit scope checkpoint: per the Assumptions, if the classified
work list comes back materially larger than the ~119 candidates suggest, scope is
re-cut with that real number rather than absorbed mid-implementation.

## Risks

| Risk | Mitigation |
|---|---|
| The Find dialog is a known-fragile neighbour — feature 041 broke it once with a comparable change and reverted | `NF_REQUERY` changes only the control→parent notification direction; messages *to* the control (ANSI column headers, item counts) are untouched. Verified explicitly against the Path column, sorting, long paths and the status bar. |
| A composed site converted to `LoadStrU8` still mixes in another ANSI ingredient | R4 audited every ingredient: names, `GetErrorText` and locale text are already UTF-8. `LoadStr` is the last ANSI source. 85 sites already mix `LoadStr` + `GetErrorText` and are already defective today. |
| The mechanical pass misses sites — it already missed the reported one | Measured and recorded in R5. FR-009a makes the UI walk load-bearing, and requires any pattern that missed a surface to be written down. |
| Converting a site whose message is *not* displayed through a UTF-8-capable route | Stage 4 classification records the display route per entry; only message-box routes take `LoadStrU8`. |
| Plugin output changes without any plugin being modified | No shared entry point is modified (FR-014a). SC-008a requires observing a plugin's dialogs before and after, rather than assuming. |
| English-only verification hides composed-message defects | Proven in R3: English templates are ASCII and therefore always work. FR-009c pins composed surfaces to all 9 languages. |

## Post-Design Constitution Re-check

Re-evaluated after Phase 1 artifacts (`data-model.md`, `contracts/`,
`quickstart.md`) were produced. The design introduces no new projects, no new
external dependencies, no plugin interface change, and no UI style change. The
only new build artifact is a guard script whose purpose is to *enforce* the
constitution's compatibility principle. **Still PASS, no violations, Complexity
Tracking not required.**
