# Tasks: Fix Plugin Name Encoding in Plugin Manager

**Input**: Design documents from `/specs/052-fix-plugin-name-encoding/`
**Prerequisites**: plan.md, spec.md, research.md (D1–D7), data-model.md, contracts/plugin-metadata-encoding.md, quickstart.md, investigation.md

**Tests**: The regression guard (checker + saltests) is a spec requirement (FR-005), so guard tasks are mandatory implementation, not optional TDD.

**Organization**: Tasks are grouped by user story. US1 is the MVP; US2 is validation-only (its outcome falls out of US1 by design D6); US3 (guard) and US4 (ZIP name) are independent.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 (correct names), US2 (existing installations), US3 (guard), US4 (ZIP name)

## Phase 1: Setup

**Purpose**: Pin down the exact scope of the encoding contract before touching producers.

- [X] T001 Audit the persisted plugin-metadata field list: read `CPlugins::Save` (src/plugins2.cpp:1581 onward, including the region below line 1655 — menu items, BugReport*, thumbnail masks) and `CPlugins::Load` (src/plugins2.cpp:1260 onward); list every REG_SZ value that carries translated/free text and its `CPluginData` field + producer call; update the field table in specs/052-fix-plugin-name-encoding/data-model.md with the confirmed list (mark non-translated ASCII identifiers as "normalize for uniformity" or "out of scope" explicitly)

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The normalization primitive and the documented contract — everything in US1/US3 builds on these.

**⚠️ CRITICAL**: No user story work can begin until this phase is complete (US4 is the exception — translation data only).

- [X] T002 Add legacy-text normalization helper to src/common/salunicode.h and src/common/salunicode.cpp: `char* SalLegacyToU8Alloc(const char* src, int maxBytes)` (name per house style) — returns input copied unchanged when it is valid UTF-8 (probe with MB_ERR_INVALID_CHARS, mirroring src/salamdr6.cpp:2390-2399), otherwise converts CP_ACP → UTF-8; result clamped to `maxBytes` at a UTF-8 sequence boundary; document in the header per salunicode.h conventions (strict vs display rationale)
- [X] T003 [P] Document the encoding contract on the `CPluginData` field declarations in src/plugins.h (around line 2419): one block comment stating the fields listed in data-model.md hold valid UTF-8 from every producer (SetBasicPluginData normalization; registry facade), referencing specs/052-fix-plugin-name-encoding/contracts/plugin-metadata-encoding.md

**Checkpoint**: Helper compiles and is callable from the main app; contract documented.

---

## Phase 3: User Story 1 — Correct plugin names in every UI language (Priority: P1) 🎯 MVP

**Goal**: Plugins Manager shows correct diacritics for every plugin, loaded or not, in every shipped language; `CPluginData` metadata is UTF-8 always (research D1–D3).

**Independent Test**: quickstart.md Scenario 1 — Czech UI, full restart so names come from the registry cache, open Plugins Manager, verify all rows (compare temp/plugins_strings.png); load one plugin and verify its row is unchanged.

### Implementation for User Story 1

- [X] T004 [US1] Normalize plugin-supplied metadata in `CSalamanderPluginEntry::SetBasicPluginData` (src/plugins1.cpp:1602-1633): route pluginName, version, copyright, description, extensions (and regKeyName if T001 marked it in-contract) through the T002 helper instead of bare `DupStr`; keep `MAX_PATH - 1` caps per plugins.h
- [X] T005 [US1] Normalize the remaining persisted-string producers identified by T001 (e.g. the ChDrvMenuFSItemName setter and menu-item name intake in src/plugins1.cpp, if their strings persist per the audit) using the same helper
- [X] T006 [US1] Switch the Plugins Manager name column to the tolerant sink: in `CPlugins::AddNamesToListView` (src/plugins2.cpp:1048) replace `ListView_SetItemText(hListView, i, 0, plugin->Name)` with `SalListViewSetItemTextU8` (declared src/common/winlib.h:346); leave the Loaded/Version/Location columns as-is
- [X] T007 [US1] Convert the 9 mixed-composition sites in src/plugins1.cpp (annotated lines 1582, 2177, 2296, 2465, 2629, 3045, 3103, 3125 — plus the composition each annotation guards, e.g. the sprintf at 1585) to the 042/043 pattern: `LoadStrU8` template + UTF-8-capable sink; delete each `// encoding-check: allow mixed-composition` annotation as its site is converted
- [X] T008 [P] [US1] Convert the 4 mixed-composition sites in src/dialogs5.cpp (annotated lines 788, 832, 858, 1336) the same way; also verify the two already-U8 compositions at src/dialogs5.cpp:325-328 (ShowInBarText) and :1222-1226 use `LoadStrU8` templates now that plugin->Name is UTF-8 by contract, and remove the stale "local copy of the ANSI p->Name" reasoning comment at src/dialogs5.cpp:832-838
- [X] T009 [P] [US1] Convert the 2 mixed-composition sites in src/fileswn7.cpp (annotated lines 1449, 1739) the same way
- [X] T010 [US1] Build (`build.cmd`) and fix any encoding-guard findings the conversions surface; then run quickstart.md Scenario 1 (Czech UI restart test, loaded-vs-not-loaded identity) and record the result in specs/052-fix-plugin-name-encoding/quickstart.md or the PR notes

**Checkpoint**: US1 fully functional — mojibake gone, names identical before/after plugin load.

---

## Phase 4: User Story 2 — Existing installations are correct immediately after update (Priority: P2)

**Goal**: Prove the fix reaches affected installations with zero user action and zero data rewrite (research D6). No code — US1 delivers the behavior; this phase verifies the spec guarantees (FR-004).

**Independent Test**: quickstart.md Scenario 2 — registry export → run fixed build → names correct → registry export diff is byte-identical (before any plugin load).

### Validation for User Story 2

- [X] T011 [US2] Execute quickstart.md Scenario 2 on this machine (it carries pre-fix cached state): `reg export "HKCU\Software\Tandem Commander\0.1\Plugins" before.reg`, run the fixed build, verify Plugins Manager display, export again before any plugin loads, `fc /b` the two exports; document the byte-identical result (values may legitimately change only after plugins.ver-triggered re-registration, which is pre-existing behavior)

**Checkpoint**: FR-004/SC-002 evidenced.

---

## Phase 5: User Story 3 — The defect class cannot silently return (Priority: P3)

**Goal**: Three-layer regression guard (research D4): contract-tracked checker identifiers, runtime tests, and a build that fails loudly when the checker cannot run.

**Independent Test**: quickstart.md Scenarios 3–4 — seeded defect fails the checker; python removed from PATH fails the build; saltests pass on healthy tree.

**Dependency note**: T012 depends on the US1 sweep (T007–T009) being merged — extending the identifier list before the conversions would fail the build on the not-yet-converted sites.

### Implementation for User Story 3

- [X] T012 [US3] Extend `UTF8_IDENT` in tools/check_encoding.py (lines 112-117) with the plugin-metadata identifiers by contract: `plugin->Name`, `p->Name`, `pluginData->Name`, `->Description`, `->Copyright`, `->Extensions`, `->ChDrvMenuFSItemName` plus any local-copy spellings encountered during T007–T009; run `python tools/check_encoding.py --strict` — must exit 0 on the converted tree
- [X] T013 [P] [US3] Add `TestPluginMetadataEncoding` to src/saltests/saltests.cpp (pattern: lines 802-927 from features 042/043): (a) registry-facade round-trip via `SalRegSetValueExW8`/`SalRegQueryValueExW8` — Czech UTF-8 in → identical UTF-8 out, and CP1250 bytes in → UTF-8 out (documents the transitional tolerance as contract); (b) T002 helper properties — ASCII unchanged, valid UTF-8 unchanged, CP1250 converted, cap respected at sequence boundary; (c) `SalListViewSetItemTextU8` accepts both valid-UTF-8 and legacy input (tolerant fallback pinned)
- [X] T014 [US3] Make the encoding guard mandatory in build.cmd (lines 212-225): replace the `Encoding guard: SKIPPED (python not on PATH)` branch with an error message naming python as a build prerequisite (pointing at quickstart/README) and `exit /b 1`; no bypass variable
- [X] T015 [US3] Execute quickstart.md Scenarios 3 and 4: seeded `utf8-to-legacy-sink` defect → checker fails; seeded mixed composition → checker fails; python off PATH → build fails; saltests suite passes; revert all seeds

**Checkpoint**: SC-003 evidenced — the class, not the instance, is guarded.

---

## Phase 6: User Story 4 — Plugin identifier names are not mistranslated (Priority: P4)

**Goal**: ZIP plugin is named exactly "ZIP" in all languages (user decision 2026-08-06; research D5) and machine re-translation cannot undo it. Independent of Phases 2–5 (translation data + tooling config only).

**Independent Test**: quickstart.md Scenario 5 — `grep "^1007,1," translations/*/zip.slt` all read `"ZIP"`; Czech/German/French UI spot-check; merge.py coverage reports the overrides as `human`.

### Implementation for User Story 4

- [X] T016 [P] [US4] Set `1007,1,"ZIP"` at line 311 of the six affected files: translations/czech/zip.slt ("PSČ"), translations/slovak/zip.slt ("PSČ"), translations/french/zip.slt ("Code postal"), translations/spanish/zip.slt ("Código postal"), translations/german/zip.slt ("ZIP-Archiv"), translations/chinesesimplified/zip.slt ("邮编"); preserve UTF-8-BOM encoding and positional format exactly (only the quoted text changes)
- [X] T017 [P] [US4] Add pins to translations/ui-overrides.json: `"zip": { "<language>": { "IDS_PLUGINNAME": "ZIP" } }` for all 10 non-English language folders (czech, slovak, french, spanish, german, dutch, hungarian, romanian, russian, ukrainian, chinesesimplified — 11 folders exist, add all), following the documented shape in the file's `_readme`
- [X] T018 [US4] Run `build.cmd full`, verify the rebuilt zip language modules carry "ZIP" (quickstart.md Scenario 5 steps 1-2), and confirm `tools/translate/merge.py` reports the new overrides as `human` in its coverage output (Scenario 5 step 3)

**Checkpoint**: SC-005 evidenced.

---

## Phase 7: Polish & Cross-Cutting Concerns

- [X] T019 Add CHANGELOG.md entries under the next unreleased version heading, following the file's existing convention (constitution §Release Documentation): **Fixed** — plugin names no longer garbled in Plugins Manager for non-English UI (names of not-loaded plugins were rendered with wrong encoding); **Changed** — the ZIP plugin is named "ZIP" in every language (was mistranslated as "postal code" in Czech, Slovak, French, Spanish, Chinese; "ZIP-Archiv" in German)
- [X] T020 [P] Run clang-format on every touched C++ file (src/plugins1.cpp, src/plugins2.cpp, src/dialogs5.cpp, src/fileswn7.cpp, src/plugins.h, src/common/salunicode.h, src/common/salunicode.cpp, src/saltests/saltests.cpp) per repo config (note: pwsh7/normalize.ps1 unavailable on this machine — invoke clang-format directly)
- [X] T021 Full regression pass: `build.cmd rebuild` (clean Debug) and `build.cmd full release` complete cleanly; run the complete quickstart.md end-to-end and check every spec Success Criterion (SC-001…SC-005) off

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: none — start immediately
- **Foundational (Phase 2)**: T002 independent; T003 independent [P]
- **US1 (Phase 3)**: needs T001 (scope) + T002 (helper); T003 can land in parallel
- **US2 (Phase 4)**: needs US1 complete (validates its outcome)
- **US3 (Phase 5)**: T012 needs T007–T009 merged; T013 needs T002; T014 independent
- **US4 (Phase 6)**: fully independent — can run any time, even first
- **Polish (Phase 7)**: T019 anytime after scope is final; T020/T021 last

### User Story Dependencies

- US1 → US2 (validation of US1's effect on existing data)
- US1 → US3 (checker extension presupposes the sweep; saltests/build-guard parts independent)
- US4 independent of all

### Parallel Opportunities

- T003 ∥ T002 (different files)
- T008 ∥ T009 (different files) after T004–T007 started; T007/T008/T009 touch disjoint files and can run concurrently once T002+T004 exist
- T013 ∥ T012/T014 (different files)
- T016 ∥ T017 (different files); whole US4 ∥ everything else
- T020 parallel with T019

## Implementation Strategy

**MVP first**: Phase 1 → 2 → 3 (US1), then validate quickstart Scenario 1 — that alone removes the user-visible defect. US2 is a validation pass. US3 locks the door. US4 is an independent quick win that can be interleaved anywhere (it only needs a `build.cmd full` to verify).

**Suggested single-developer order**: T001 → T002+T003 → T004–T010 (US1) → T011 (US2) → T012–T015 (US3) → T016–T018 (US4) → T019–T021. Commit per task or logical group; every commit must keep `build.cmd` green.
