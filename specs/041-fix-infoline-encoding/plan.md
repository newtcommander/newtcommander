# Implementation Plan: Fix Information Line Encoding

**Branch**: `041-fix-infoline-encoding` | **Date**: 2026-07-26 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/041-fix-infoline-encoding/spec.md`

## Summary

The application carries narrow strings as UTF-8, but the number separators and
the date/time formats are still fetched from Windows in the legacy single-byte
form. The Czech thousands separator is a non-breaking space — one `0xA0` byte,
invalid UTF-8. The information line concatenates name, size, date, time and
attributes into a single string and converts it once, so that one byte makes the
conversion fail and the whole line drops to the byte-wise drawing path, where
the UTF-8 file name renders as `NovÃ¡ nadÄ›je`.

Two changes, and they are complementary rather than redundant:

1. **Remove the cause.** Locale-derived text becomes UTF-8 at the producer, like
   every other narrow string in the application. This fixes the information
   line, the selection summary, the Find dialog, and every plugin that asks the
   application to format a number.
2. **Bound the blast radius.** The information line converts leniently, so a
   character that genuinely cannot be represented costs that character and not
   the entire line. Today's failure mode — one bad byte destroys everything —
   stops being possible regardless of what causes it.

## Technical Context

**Language/Version**: C++20 (`/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: pure WinAPI; existing in-repo `src/common/salunicode.*`
(UTF-8 ↔ UTF-16, feature 004). No new external dependencies.
**Storage**: N/A — display-path change only
**Testing**: scripted GUI verification (launch, focus an item, read the rendered
text, capture the window) plus a full build across all shipped languages; no
unit-test harness exists for the main application
**Target Platform**: Windows 11+ (x64)
**Project Type**: desktop application (two-panel file manager, pure WinAPI)
**Performance Goals**: the information line is rebuilt on every focus change, so
panel navigation must stay visually instantaneous; the added work is one
conversion of a ≤1000-byte string per keystroke
**Constraints**: plugin interface shape, version and binary compatibility must
not change (FR-012, Constitution II); the strict `SalU8ToW*` contract and its
existing file-operation callers must not change (Constitution III); no new
configuration option (FR-009)
**Scale/Scope**: 42 locale call sites to classify (6 provisionally exempt, the
rest expected to need conversion), 1 new helper set, 1 lenient display
conversion, 2 co-victim surfaces, 18 plugins reviewed

## Constitution Check

*GATE: evaluated before Phase 0 and re-evaluated after Phase 1 design.*

| Principle | Assessment | Verdict |
|-----------|-----------|---------|
| I. Build Reproducibility | Source-only change; no new build step, no manual step, no new dependency. | PASS |
| II. Backward Compatibility | The plugin interface's shape, version (104) and binary compatibility are untouched — only the encoding of a returned string changes, and the previous bytes were malformed for any non-ASCII locale, so no correct plugin could depend on them. FR-013 makes this checkable by reviewing all 18 affected plugins rather than asserting it. No registry, IPC or config surface touched. | PASS |
| III. Incremental Modernization | The strict conversion helpers and their file-operation callers are deliberately left alone; the lenient variant is added beside them for display only. Group A call sites are converted mechanically, one API family at a time. No adjacent refactoring. | PASS |
| IV. Windows Platform Commitment | Pure WinAPI, moving from the A to the W variants of the same calls. No abstraction layer. | PASS |
| V. Plugin Architecture Preservation | `spl_gen.h` untouched; plugins keep calling the same methods and get correct text instead of malformed text. The review under FR-013 is the safeguard. | PASS |
| VI. UI Consistency | No dialog template, control style or process-wide visual behaviour changes. The information line renders through the path it already uses when its text happens to be valid — this change makes that the normal case. | PASS |

**Result**: no violations. Complexity Tracking section omitted.

**Post-Phase-1 re-check**: the design adds one internal helper pair and one
lenient conversion, no new component, no new abstraction layer, and no change to
any published interface. All six principles still pass.

## Project Structure

### Documentation (this feature)

```text
specs/041-fix-infoline-encoding/
├── spec.md              # Feature specification (clarified)
├── plan.md              # This file
├── research.md          # Phase 0 — mechanism, FR-011 investigation, FR-013 surface
├── data-model.md        # Phase 1 — the strings involved and their invariants
├── quickstart.md        # Phase 1 — build, reproduce, verify
├── contracts/
│   ├── locale-text.md         # internal contract: locale-derived text is UTF-8
│   └── information-line.md    # UI contract: what the line must display
├── checklists/
│   └── requirements.md  # Spec quality checklist (16/16)
└── tasks.md             # Phase 2 output (/speckit.tasks)
```

### Source Code (repository root)

```text
src/
├── common/
│   ├── salunicode.h             # + lenient display conversion (declaration)
│   └── salunicode.cpp           # + lenient display conversion (implementation)
├── sallocale.h / .cpp           # NEW (or folded into salunicode): UTF-8 wrappers
│                                #   over GetLocaleInfoW / GetDateFormatW / GetTimeFormatW
├── salamdr1.cpp                 # separators read as UTF-8; buffers widened
├── consts.h                     # separator declarations widened (not plugin-visible)
├── salamdr4.cpp                 # column date/time sites; ExpandPluralBytesFilesDirs
├── execute.cpp                  # information-line date/time expansion
├── fileswn2.cpp                 # panel date/time columns (assumption corrected)
├── worker.cpp                   # progress/error date/time text
├── finddlg1.cpp                 # Find dialog date fields
├── stswnd.cpp                   # information line converts leniently; truncation check
└── plugins/                     # reviewed, not expected to change (FR-013)
```

**Structure Decision**: the fix stays inside the main application. The locale
wrappers sit next to the existing Unicode helpers so there is one obvious place
for "text crossing the OS boundary", which is exactly what
`src/common/salunicode.h` already documents itself as. Whether they become a new
`sallocale` pair or additions to `salunicode` is a small judgement call left to
implementation; the contract in `contracts/locale-text.md` is the same either
way. No new directory, no new project, no change to any plugin build.

## Phase 0 — Research

Complete. See [research.md](./research.md). Eight findings, no unresolved
NEEDS CLARIFICATION. The load-bearing ones:

- The mechanism is confirmed end to end, with the offending byte measured on
  this machine (`cs-CZ`, CP1250, `LOCALE_STHOUSAND` = U+00A0 → `0xA0`).
- The panel survives because it converts per column; the information line does
  not because it converts once. That asymmetry is why the fix has two parts.
- No lenient conversion exists — the existing helpers are strict by design, for
  good reasons that still hold for their current callers.
- The FR-011 investigation found **42** locale call sites across 13 files. Six
  are provisionally exempt (the crash-report writer, a language-name lookup);
  the remaining 36 need individual classification, and the ones traced so far
  all need conversion. It also found a confirmed second victim: the selection
  summary combines localized text with the malformed separator, and it reaches
  the **Find dialog** as well as the information line — a surface the report
  never mentioned.
- The FR-013 surface is **18** plugins; the interface itself needs no change.

## Phase 1 — Design & Contracts

Complete. Artifacts:

- [data-model.md](./data-model.md) — the strings involved, who produces and
  consumes each, and the invariants that must hold afterwards.
- [contracts/locale-text.md](./contracts/locale-text.md) — the internal
  contract: everything the application obtains from regional settings is UTF-8
  from the moment it is obtained.
- [contracts/information-line.md](./contracts/information-line.md) — the UI
  contract the line must satisfy, in terms a reviewer can check on screen.
- [quickstart.md](./quickstart.md) — build, reproduce the defect, verify the fix.

## Implementation Approach

Five groups of work, in dependency order.

1. **Locale wrappers.** Add UTF-8 wrappers over `GetLocaleInfoW`,
   `GetDateFormatW`, `GetTimeFormatW`. Each calls the W API and converts the
   result to UTF-8. This is the single place the encoding boundary is crossed.
2. **Producers.** Read `DecimalSeparator` and `ThousandsSeparator` through the
   wrapper and widen both buffers to 16 bytes. Convert the Group A date/time
   sites. After this step the information line is valid UTF-8 and the reported
   defect is gone.
3. **Lenient display conversion.** Add it beside the strict helpers, documented
   as display-only, and use it for the information line's wide mirror. Handle
   the surrogate-pair truncation gap while in that code.
4. **Co-victims.** Confirm the selection summary and the Find dialog status are
   fixed by step 2; fix anything the FR-011 classification still shows broken,
   and record Group C as unaffected.
5. **Plugin review.** Walk the 18 plugins that call the shared formatting,
   record an outcome for each, fix any that depended on the old bytes, and
   confirm the interface version is unchanged and every plugin still loads.

Steps 1→2 are strictly ordered. Step 3 is independent of step 2 and could be
done first, but is sequenced after so its value — graceful degradation — is
demonstrated against a line that is otherwise already correct. Steps 4 and 5 both
depend on step 2.

## Risks

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| A Group A consumer feeds the now-UTF-8 separator into an ANSI-only API, trading one mojibake for another | Medium | This is FR-008 and the reason FR-011's classification exists. Every Group A consumer is traced to its display path before conversion, and the panel columns, directory line and size dialogs are checked on screen afterwards. |
| A plugin depended on the old bytes | Low | FR-013 review of all 18, with a recorded outcome each; a plugin could only be harmed by passing the value straight to an ANSI API, which is already broken today for non-ASCII locales. |
| Lenient conversion masks a real encoding bug instead of surfacing it | Medium | Restricted to the display path and named so; the strict helpers stay strict, so file operations still refuse malformed names rather than silently mangling them. |
| Widening the separator buffers breaks a caller assuming 5 bytes | Low | Both are declared in `src/consts.h`, which no plugin includes (verified); all callers are in-tree and compile-checked. |
| Per-keystroke conversion makes panel navigation sluggish | Low | One conversion of a ≤1000-byte string per focus change, replacing work already being done; checked by holding an arrow key through a large directory. |
| Locale-specific behaviour cannot be reproduced on this machine | Medium | `cs-CZ` reproduces the reported defect. Locales whose date/time formats are non-ASCII are covered by converting the sites regardless, and verified by temporarily switching the regional settings during validation. |
