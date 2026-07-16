# Tasks: Plugin Build Policy — Remove Obsolete Plugins and Introduce a Build-Time Plugin Configuration

**Input**: Design documents from `/specs/007-plugin-build-policy/`
**Prerequisites**: plan.md, spec.md, research.md (R1–R8), data-model.md, contracts/plugins-cfg.md, contracts/build-cmd.md, quickstart.md

**Tests**: No automated test framework exists in this repository — no test tasks. Every story ends with verification tasks executing the relevant rows of the `quickstart.md` verification matrix.

**Organization**: Tasks are grouped by user story. Note the documented ordering exception: US2–US4 build on the post-removal plugin set, so US1 must complete first (see Dependencies).

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 = removal, US2 = config mechanism, US3 = default policy, US4 = validation feedback

## Phase 1: Setup

**Purpose**: Confirm a healthy starting point so every later diff is attributable to this feature.

- [x] T001 Run baseline `build.cmd rebuild full` on a clean working tree; confirm BUILD SUCCEEDED and record the reported plugin count (expected 35 registered in plugins.ver) and output layout under `%OPENSAL_BUILD_DIR%salamander\Debug_x64\` for later comparison

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: None required — this feature modifies existing build infrastructure and deletes code; there is no shared scaffolding to build first. Phase intentionally empty.

**Checkpoint**: Proceed directly to User Story 1.

---

## Phase 3: User Story 1 - Obsolete plugins are gone from the product (Priority: P1) 🎯 MVP

**Goal**: The 8 plugins (pak, unarj, unlha, unfat, wmobile, ieviewer, splitcbn, winscp) are completely removed: sources, solution entries, build steps, core-code references, help, translations, tooling lists. Build succeeds; upgrades stay silent.

**Independent Test**: quickstart.md rows 2 and 13 — repo-wide grep finds no functional references; `build.cmd rebuild full` succeeds with 28 plugins; app over an old profile starts with no dialogs.

### Implementation for User Story 1

- [x] T002 [US1] Delete plugin source trees: `git rm -r src/plugins/pak src/plugins/unarj src/plugins/unlha src/plugins/unfat src/plugins/wmobile src/plugins/ieviewer src/plugins/splitcbn src/plugins/winscp`
- [x] T003 [US1] Remove the 14 removed-plugin project entries (7 × `<name>.vcxproj` + 7 × `lang_<name>.vcxproj`, winscp has none) from `src/vcxproj/salamand.sln`: the `Project(...)` blocks, every `GlobalSection(ProjectConfigurationPlatforms)` line for their GUIDs, and any `GlobalSection(NestedProjects)` / dependency lines referencing those GUIDs
- [x] T004 [US1] In `build.cmd`: delete the ieviewer CSS robocopy step (lines ~238–243) and update the trailing NOTE text (lines ~297–299) that mentions winscp not being part of salamand.sln
- [x] T005 [P] [US1] In `src/plugins2.cpp`: remove `AddPlugin("PAK", "pak\\pak.spl", ...)` (~line 1554) and `AddPlugin("Internet Explorer Viewer", "ieviewer\\ieviewer.spl", ...)` (~line 1559) from the standard-plugins table; remove the `winscp\\winscp.spl` special case (~line 2878); extend the silent-uninstall suppress list (~lines 3245–3248, fsearch/eroiica/unace/diskcopy block — appears in two places, ~3245 and the parallel block near ~3352 if present) with the 8 removed plugins' `<name>\\<name>.spl` paths (research R6)
- [x] T006 [US1] Investigate persistence semantics of the packer/format tables (research R7): read `src/packers.cpp` (`CPackerConfig::AddDefault`, Save/Load), `src/pack3.cpp` (`SetFormat` table, `CArchiverConfig`), and their registry save/load paths; determine whether stored user configs reference table entries by index or by value; append findings as an addendum to `specs/007-plugin-build-policy/research.md` R7
- [x] T007 [US1] Per T006 findings, remove the `pak` default additions — `SetPacker(index, 3, "PAK (Plugin)", "pak", TRUE)` in `src/packers.cpp` (~line 219) and `SetFormat(index, "pak", TRUE, -3, -3, TRUE)` in `src/pack3.cpp` (~line 459) — without renumbering any surviving entry; if T006 shows index-coupled persistence prevents safe removal, keep the entries, mark them with an explanatory comment, and document the deviation in research.md R7
- [x] T008 [P] [US1] Remove dead WinSCP x86 special-casing: `AddX86OnlyPlugins` logic in `src/dialogs4.cpp` (~line 553), `#ifdef _WIN64` FIXME_X64_WINSCP block in `src/dialogs5.cpp` (~line 870), `#ifndef _WIN64` blocks in `src/mainwnd2.cpp` (~lines 1907, 3438), and the FIXME_X64_WINSCP comments in `src/cfgdlg.h` (~lines 411, 421) and `src/pwdmngr.h`; simplify surrounding code only as far as the removed guards require (constitution III)
- [x] T009 [P] [US1] In `src/lang/lang.rc`: reword the two Master Password LTEXT strings (~lines 1996, 2025) from "…like FTP or WinSCP passwords…" to FTP-only wording
- [x] T010 [P] [US1] Help cleanup in `help/src/`: remove the 8 plugin names from the compile list in `compileall.bat` (lines 25–26); remove their `.chm` MERGE/file entries from `salamand.hhp` (~lines 368–386); remove their TOC sections from `salamand.hhc` (~lines 985–1180); grep `salamand.hhk`, `copy_to_salbin.bat`, and `hh/salamand/*.htm` (known: `appendix_fspaths.htm`, `introduction_news.htm`, `othertask_pwdmanager.htm`) and remove/reword remaining references
- [x] T011 [P] [US1] Delete translation sources for the 8 removed plugins in every `translations/<language>/` directory (`pak.slt`, `unarj.slt`, `unlha.slt`, `unfat.slt`, `wmobile.slt`, `ieviewer.slt`, `splitcbn.slt`, plus `winscp.slt` where present) and remove their names from `translations/!update_langs_from_translator.bat` if listed
- [x] T012 [P] [US1] Remove the 8 plugin names from the plugin lists in `tools/comments/word_counter.py` (~line 124), `tools/comments/translation_status.py`, and `tools/comments/code_guard.py`
- [x] T013 [US1] Final reference audit: `git grep -i` each of the 8 names across the repo (excluding `.git`, `specs/`, `features/`, `architecture/`, git history); fix any functional straggler found (candidates: `src/setup/`, `.github/`, `src/salmon/`, `doc/`); historical changelog mentions are acceptable (SC-002)
- [x] T014 [US1] Verify US1: `build.cmd rebuild full` succeeds; `%OPENSAL_BUILD_DIR%...\plugins\` contains no trace of the 8; plugins.ver reports 28 plugins; launch built Salamander over a profile that previously registered the removed plugins — silent cleanup, no error dialogs (quickstart rows 2, 13)

**Checkpoint**: Repository and product are free of the 8 plugins — deliverable even if nothing else ships.

---

## Phase 4: User Story 2 - One configuration file controls which plugins are built (Priority: P2)

**Goal**: `plugins.cfg` + PowerShell helper + `build.cmd` integration: only enabled plugins are compiled and shipped, every flavor, with pre-build output reconciliation.

**Independent Test**: quickstart rows 3–5 — toggle `renamer` off/on and confirm compile/output/plugins.ver follow the config in every build flavor.

### Implementation for User Story 2

- [x] T015 [P] [US2] Create `plugins.cfg` in the repository root with the normative content from `contracts/plugins-cfg.md` (28 entries, 18 on / 10 off, header comment explaining syntax and purpose)
- [x] T016 [P] [US2] Add `/src/vcxproj/salamand.gen.slnf` to the root `.gitignore`
- [x] T017 [US2] Implement `src/vcxproj/gen_plugins_filter.ps1` per `contracts/build-cmd.md`: parameters `-Config`, `-Solution`, `-OutSlnf`, `-PluginsRoot`, optional `-OutputPluginsDir`; parse + validate V1–V5 from `contracts/plugins-cfg.md` (case-insensitive names, `shared` exempt, `ERROR:` messages naming file/line/entry, exit 1); generate deterministic `salamand.gen.slnf` including every solution project except those under `..\plugins\<name>\` of disabled plugins; when `-OutputPluginsDir` given, delete output dirs of disabled plugins and of dirs matching no current plugin (preserve `plugins.ver` file itself and `Intermediate` artifacts), then filter `plugins.ver` entry lines to enabled plugins only; print `Plugins: N enabled, M disabled` (PowerShell 5.1 compatible, ASCII output)
- [x] T018 [US2] Integrate into `build.cmd`: after prerequisite checks and OUT_DIR computation, invoke the helper (`powershell -NoProfile -ExecutionPolicy Bypass -File ...` with `-OutputPluginsDir "%OUT_DIR%\plugins"`); abort with non-zero exit before MSBuild on helper failure; retarget the MSBuild invocation from `salamand.sln` to `salamand.gen.slnf`; capture the `Plugins: N enabled` count and show it in the final summary block
- [x] T019 [US2] In `build.cmd` `:populate_runtime`: make per-plugin runtime data copies conditional on the plugin being built — guard the automation sample-scripts robocopy and the zip2sfx robocopy with `if exist "%OUT_DIR%\plugins\<name>\"` checks (post-MSBuild, only enabled plugins have output dirs)
- [x] T020 [US2] Verify US2: quickstart rows 3–5 — flip `renamer=off`, run incremental `build.cmd`: not compiled, output dir gone, no plugins.ver line; flip back on: returns; repeat the off-toggle for `rebuild`, `full`, and `full release` flavors

**Checkpoint**: Config mechanism fully governs the scripted build.

---

## Phase 5: User Story 3 - The agreed default policy ships with the repository (Priority: P3)

**Goal**: Out-of-the-box build = exactly the 18 enabled plugins; app starts clean.

**Independent Test**: quickstart rows 1 and 12 — clean checkout, `build.cmd full`, count 18 everywhere, Plugin Manager lists exactly 18.

### Implementation for User Story 3

- [x] T021 [P] [US3] Verify committed `plugins.cfg` against `data-model.md` initial content: exactly 28 entries, the 10 named plugins `off` (automation, checkver, demomenu, demoplug, demoview, mmviewer, nethood, unchm, unmime, unole), remaining 18 `on`, every entry matching a `src/plugins/` directory
- [x] T022 [US3] Verify default full build (quickstart row 1): from clean state `build.cmd rebuild full` succeeds; `<out>\plugins\` contains exactly the 18 enabled plugin dirs with `.spl` + `lang\english.slg`; `plugins.ver` has exactly 18 entry lines; summary reports 18
- [x] T023 [US3] Verify runtime (quickstart row 12): launch built Salamander — zero plugin-related dialogs; Plugins Manager lists exactly the 18 enabled plugins

**Checkpoint**: Shipped policy verified end to end.

---

## Phase 6: User Story 4 - Configuration problems are reported clearly (Priority: P4)

**Goal**: All five config error classes stop the build before compilation with messages naming the file and offending entry; case-insensitive matching works.

**Independent Test**: quickstart rows 6–11.

### Implementation for User Story 4

- [x] T024 [US4] Review and harden the V1–V5 error paths in `src/vcxproj/gen_plugins_filter.ps1` against `contracts/plugins-cfg.md`: each message is `ERROR: plugins.cfg ...` naming the expected path (V1), file+line+text (V2), unknown entry name (V3), duplicated name (V4), or unlisted plugin directory (V5); confirm `build.cmd` propagates the failure before any MSBuild start and with a non-zero exit code
- [x] T025 [US4] Verify US4 (quickstart rows 6–11): temporarily (a) rename plugins.cfg away, (b) add `tar=maybe`, (c) add `bogus=on`, (d) duplicate `zip=on`, (e) delete the `tar=` line — each aborts pre-MSBuild with the contracted message; (f) `ZIP=ON` case variant builds normally; restore `plugins.cfg` via `git checkout -- plugins.cfg`

**Checkpoint**: All user stories complete.

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Documentation truth, formatting, and the full acceptance sweep (FR-012, SC-001…SC-006).

- [x] T026 [P] Update `CLAUDE.md`: plugin counts (35→28 plugins in repo tree, 27 buildable→shown counts per new reality), Key Facts project totals (90→76 projects), remove winscp from missing-deps note, document `plugins.cfg` in Build Quick Start and Key Facts, mention plugins.ver now lists enabled plugins only
- [x] T027 [P] Update architecture docs to the new plugin disposition: `architecture/02-solution-structure.md` (76 projects), `architecture/03-build-pipeline.md` (policy stage, .slnf, reconciliation), `architecture/04-dependencies.md` (drop winscp/Embarcadero and ieviewer/MSHTML rows), `architecture/06-plugin-architecture.md` (plugins.cfg mention), `architecture/09-plugin-catalog.md` and `architecture/10-plugin-maintenance-outlook.md` (mark 8 removed, 10 disabled-by-default)
- [x] T028 Run formatting verification on all touched C++ sources (`clang-format` per repo config / `normalize.ps1` if present); confirm UTF-8-BOM preserved on edited files
- [x] T029 Execute the complete `quickstart.md` verification matrix rows 1–14 (including row 14: all 28 entries `off` → build succeeds, core app runs with empty plugin set) and record results in `specs/007-plugin-build-policy/quickstart.md` or a validation notes file under the feature dir
- [x] T030 Final acceptance sweep against spec Success Criteria SC-001…SC-006; confirm `specs/007-plugin-build-policy/checklists/requirements.md` items remain satisfied; fix any gap found

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependencies.
- **Foundational (Phase 2)**: empty — skip.
- **US1 (Phase 3)**: starts after T001. **Blocks US2/US3/US4** — documented exception to story independence: `plugins.cfg` enumerates the post-removal 28-plugin set (V5 would fail against 36 dirs), and T004/T018/T019 edit the same `build.cmd`.
- **US2 (Phase 4)**: after US1 completes.
- **US3 (Phase 5)** and **US4 (Phase 6)**: after US2 completes; US3 and US4 are mutually independent and can run in parallel.
- **Polish (Phase 7)**: after all stories.

### Within-Story Dependencies

- US1: T002 → T003 (sln edit assumes dirs gone for verification); T006 → T007 (investigation before packer edits); T005, T008–T012 mutually parallel [P]; T013 after T002–T012; T014 last.
- US2: T015, T016 parallel [P]; T017 after T015 (needs config to test against); T018–T019 after T017 (same file `build.cmd`, sequential); T020 last.
- US3: T021 parallel with nothing pending; T022 → T023.
- US4: T024 → T025.

### Parallel Opportunities

```text
US1 wave after T002+T003:  T005 | T008 | T009 | T010 | T011 | T012   (6 tasks, disjoint files)
US2 start:                 T015 | T016
After US2:                 Phase 5 (US3) | Phase 6 (US4)
Polish:                    T026 | T027
```

---

## Implementation Strategy

**MVP = US1** (Phase 1 + Phase 3): the removal alone is a complete,
valuable, shippable increment (quickstart rows 2, 13 prove it).

Incremental delivery: US1 → verify → US2 (mechanism) → verify toggles
→ US3 + US4 (verification-heavy, parallelizable) → Polish. Commit
after each task or logical group; every checkpoint leaves the build
green.

**Total**: 30 tasks (US1: 13, US2: 6, US3: 3, US4: 2, Setup: 1, Polish: 5).
