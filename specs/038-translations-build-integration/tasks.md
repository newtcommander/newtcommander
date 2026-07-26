---

description: "Task list for 038-translations-build-integration"
---

# Tasks: Translations Build Integration

**Input**: Design documents from `/specs/038-translations-build-integration/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md

**Tests**: TDD was not requested, so there are no speculative test suites below.
The verification tasks that *are* present are shipped deliverables required by
the spec — the `.slt` round-trip check (the positional-import constraint, R2),
the `FILEVERSION` verifier (FR-026), and the layout gate (FR-013 / SC-005).

**Organization**: Tasks are grouped by user story so each can be implemented and
verified independently.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1–US5)
- Exact file paths are given in every task

## Path Conventions

Repository root is `E:\Projects\newtcommander`. Build output goes to
`%OPENSAL_BUILD_DIR%salamander\<Config>_<Platform>\`. Committed translation
source lives in `translations/`; offline tooling in `tools/translate/`; build
scripts in `src/vcxproj/`.

## Scope constants (used throughout)

- **20 modules** = `salamand` + the 19 plugins enabled in `plugins.cfg`
- **15 modules** have legacy translation data: 7zip, checksum, dbviewer, diskmap,
  filecomp, ftp, peviewer, pictview, regedt, renamer, tar, uncab, undelete,
  uniso, zip (+ `salamand` = 16 with data)
- **4 modules** have none: folders, mdview, portables, sftp
- **11 non-English languages** = 10 existing + ukrainian; **12 shipped** with English

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Registry, tooling skeleton, and repository hygiene

- [X] T001 Create `translations/languages.cfg` with 11 records (folder, langid, display_name, author, web, comment, helpdir, origin) using the `key=value` + `#` comment style of `plugins.cfg`; read each existing LANGID from the `[TRANSLATION]` header of `translations/<lang>/salamand.slt` (czech 1029, german 1031, russian 1049, …) and add ukrainian=1058 with `origin=machine`
- [X] T002 [P] Create the `tools/translate/` package skeleton — `tools/translate/__init__.py` and `tools/translate/README.md` describing the three-stage workflow from `specs/038-translations-build-integration/quickstart.md`
- [X] T003 [P] Add the `anthropic` dependency and `translate-merge` / `translate-batch` console entry points to `tools/pyproject.toml`, and add `translate` to `[tool.setuptools] packages`
- [X] T004 [P] Add build intermediates to `.gitignore`: generated `.atp` projects, the `translator/templates/` export directory, and `*.bak` under `OPENSAL_BUILD_DIR`

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The `.slt` ↔ `.slg` pipeline. Nothing in any user story can produce
a language module without this.

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T005 Implement the `.slt` reader in `tools/translate/slt.py` — parse `[EXPORTINFO]`, `[TRANSLATION]`, `[DIALOG]`, `[MENU]`, `[STRINGTABLE]`, `[RELAYOUT]` into an ordered model per `contracts/slt-format.md`, preserving section and row order exactly (order is load-bearing: import is positional)
- [X] T006 Implement the `.slt` writer in `tools/translate/slt.py` — UTF-8 BOM, CRLF, four `[EXPORTINFO]` value lines, blank-line separators, quoted-text escaping matching `EncodeString`/`DecodeString`
- [X] T007 Add a round-trip check to `tools/translate/slt.py` (`--verify` mode) that parses and re-serializes all 230 existing files under `translations/` and asserts byte equality; run it and fix any parser gap it exposes
- [X] T008 [P] Create `src/vcxproj/gen_atp.ps1` — emit `<module>.atp` with `[Files] Original=/Translated=/Include=` as absolute paths plus a minimal `[Settings]` block, per `contracts/translator-cli.md`; `Include=` resolves to `src/lang/lang.rh` for salamand and `src/plugins/<name>/lang/lang.rh` for plugins
- [X] T009 Create `src/vcxproj/build_langs.cmd` — parse `translations/languages.cfg` and `plugins.cfg`, enumerate the (module × language) matrix, validate that every `languages.cfg` folder exists under `translations/` and every LANGID is unique, failing the build with a clear message otherwise
- [X] T010 Add target seeding to `src/vcxproj/build_langs.cmd` — copy `english.slg` to `<language>.slg` in the module's `lang` output directory before every import (satisfies the `CopyFile` precondition in `CData::Save()` and gives the target `SLGCRCofImpSLT="none"`, which suppresses the only modal prompt on the quiet-import path)
- [X] T011 Add the guarded `translator.exe` invoker to `src/vcxproj/build_langs.cmd` — treat **`ERRORLEVEL == 1` as success**, run every invocation under a 30 s timeout, kill the process on timeout, and report failures naming the (language, module) pair
- [X] T012 Add `--export-templates` mode to `src/vcxproj/build_langs.cmd` — for each module, seed a scratch `.slg`, emit an `.atp`, and run `-quiet-export-slt` into `%OPENSAL_BUILD_DIR%salamander\translator\templates\<module>.slt`
- [X] T013 [P] Create `src/vcxproj/verify_slg.ps1` — read `VS_FIXEDFILEINFO.dwFileVersionMS/LS` from each produced `.slg` and compare against its owning binary (`newtcommander.exe` or `<name>.spl`), failing the build on mismatch (FR-026)
- [X] T014 Add `.bak` cleanup for the output `lang` directories to `src/vcxproj/build_langs.cmd` (the Translator leaves one per import)

**Checkpoint**: `build_langs.cmd --export-templates` produces an English template for every module, and re-importing that template yields a `.slg` the product loads. The pipeline is proven before any translation content is involved.

---

## Phase 3: User Story 1 - Run the application in my own language (Priority: P1) 🎯 MVP

**Goal**: The main application is selectable and fully usable in all 10 existing
languages.

**Independent Test**: Launch the built product, open the language chooser,
confirm it lists 11 languages, select a non-English one, restart, and confirm the
main window and dialogs are in that language.

### Implementation for User Story 1

- [X] T015 [US1] Implement legacy merge in `tools/translate/merge.py` — load the stage-1 template as the authoritative structure and fill each entry from the legacy `.slt` by (section, ID), marking filled entries `human`; entries with no legacy counterpart stay empty for now, legacy entries with no template counterpart are dropped and counted as `discarded` (FR-014)
- [X] T016 [P] [US1] Implement `tools/translate/rebrand.py` — replace predecessor product and vendor names with the Newt Commander identity, strip `altap.cz` forum and support URLs rather than repointing them, and replace translator contact links with the project address while preserving translator names (FR-018, FR-019, FR-020)
- [X] T017 [US1] Wire `rebrand.py` and the `languages.cfg` record into `tools/translate/merge.py` so each emitted `.slt` carries the correct `[TRANSLATION]` block (LANGID, AUTHOR, WEB, COMMENT, HELPDIR, SLGINCOMPLETE) and rebranded body text
- [X] T018 [US1] Add the origin sidecar writer to `tools/translate/merge.py` — emit `translations/<language>/<module>.origin` mapping entry key → `human` | `machine` | `english_fallback` (FR-011); the sidecar exists because the `.slt` grammar is fixed by the parser and has no comment syntax
- [X] T019 [US1] Run `python -m translate.merge --module salamand` for all 10 existing languages, review the diff, and commit the regenerated `translations/<language>/salamand.slt` files
- [X] T020 [US1] Build with `build.cmd full` then `src\vcxproj\build_langs.cmd --module salamand`, and confirm 10 `salamand`-module `.slg` files are produced and pass `verify_slg.ps1`
- [ ] T021 [US1] Run the product with no saved configuration and verify the first-run chooser lists 11 languages with readable names and author credits; capture a screenshot
- [ ] T022 [US1] Select Czech, restart, and verify the main window, menus, and a representative set of dialogs render in Czech and that the choice persists; capture screenshots
- [ ] T023 [US1] Verify the Windows-display-language preselection path (`GetPreferredLanguageIndex`) picks the matching language on first run, and that the configuration dialog shows and can change the active language

**Checkpoint**: The application itself is fully localized in 10 languages. Plugins are still English — that is US2.

---

## Phase 4: User Story 2 - Plugins follow the language I chose (Priority: P2)

**Goal**: The 15 enabled plugins that have legacy translation data come up in the
chosen language with no per-plugin prompt.

**Independent Test**: With a non-English language active, load each of the 15
plugins and confirm its UI is in that language and no language-selection prompt
appears.

### Implementation for User Story 2

- [X] T024 [US2] Extend `tools/translate/merge.py` to iterate the module matrix so `--all` covers every enabled module rather than one at a time, skipping modules with no legacy data for the requested language
- [X] T025 [US2] Run `python -m translate.merge --all` for the 15 legacy plugin modules across the 10 existing languages, review, and commit the regenerated `translations/<language>/<module>.slt` files
- [X] T026 [US2] Run `src\vcxproj\build_langs.cmd` for all modules and confirm 150 plugin `.slg` files are produced and pass `verify_slg.ps1`
- [ ] T027 [US2] With a non-English language active, load each of the 15 plugins (archive open, file compare, FTP connect, picture view, registry edit, rename, undelete, …) and verify no per-plugin language prompt appears and each UI is translated
- [ ] T028 [US2] Open the plugin manager and verify every listed plugin reports the same active language; verify that changing the application language and restarting propagates to all plugins

**Checkpoint**: A full working session in a non-English language is possible except for SFTP, MDView, Folders, and Portables.

---

## Phase 5: User Story 3 - New plugins are translated too (Priority: P2)

**Goal**: SFTP, MDView, Folders, and Portables are machine-translated into the 10
existing languages, and post-4.0 drift in the legacy modules is filled.

**Independent Test**: With a non-English language active, open the SFTP connect
dialog and the MDView viewer; both are in that language, with no prompt and no
layout damage.

### Implementation for User Story 3

- [X] T029 [P] [US3] Implement `tools/translate/validate.py` — verify a translated string against its English source: identical multiset of `printf` placeholders (`%s %d %u %c %x %ld` and positional forms), identical `&` accelerator count, verbatim `\t`-suffix shortcut label, and preserved `\n` `\r` `\t` `\\` `\"` escapes (FR-012)
- [X] T030 [US3] Implement the batch driver in `tools/translate/deepl.py` (DeepL, not Anthropic -- the available key) — `anthropic` SDK with `claude-opus-5` over `client.messages.batches.create`, one request per translation unit keyed by `custom_id`, structured outputs for the reply shape, and a `cache_control` breakpoint on the shared prefix (glossary, style rules, placeholder rules, target language); **key results by `custom_id`, never by position**
- [ ] T031 [US3] Add a per-string length budget to the prompt in `tools/translate/translate.py`, derived from the English source width, to reduce dialog overflow before it happens (mitigation for FR-013)
- [X] T032 [US3] Integrate machine translation into `tools/translate/merge.py` — batch every entry left empty after the legacy pass, validate each result with `validate.py`, retry failures once with a corrective prompt, fall back to the English source on persistent failure and record it as `english_fallback` in the sidecar and coverage report
- [X] T033 [US3] Add a post-assembly accelerator-uniqueness check to `tools/translate/validate.py` — no duplicate `&`-letter within a single dialog or menu (SC-006)
- [X] T034 [US3] Run machine translation for folders, mdview, portables, and sftp across the 10 existing languages; review the output and commit the new `translations/<language>/<module>.slt` files and sidecars
- [X] T035 [US3] Run machine translation to fill post-4.0 drift in the 15 legacy plugin modules and in `salamand` across the 10 existing languages; review and commit
- [ ] T036 [US3] Rebuild and verify: with a non-English language active, open the SFTP connect dialog and the MDView viewer — translated, no prompt, every control readable and inside the dialog; capture screenshots

**Checkpoint**: All 10 existing languages are complete across all 20 modules.

---

## Phase 6: User Story 4 - Ukrainian, produced from scratch (Priority: P3)

**Goal**: Ukrainian ships as a fully machine-translated 12th language.

**Independent Test**: Select Ukrainian in the chooser and run a full working
session across the application and every enabled plugin.

### Implementation for User Story 4

- [X] T037 [US4] Create the `translations/ukrainian/` directory and confirm its `languages.cfg` record (langid 1058, `origin=machine`, `helpdir=ENGLISH`, project web address, `SLGINCOMPLETE` set to the project URL)
- [X] T038 [US4] Run `python -m translate.merge --language ukrainian` across all 20 modules — every entry is a gap, so the whole language is machine-produced; review and commit the `.slt` files and sidecars
- [ ] T039 [US4] Build and verify Ukrainian appears in the chooser, is selectable, and that a full session across the application and every enabled plugin runs in Ukrainian with no per-plugin prompt; capture screenshots
- [ ] T040 [US4] Verify the Ukrainian `.slg` files carry a non-empty `SLGIncomplete` so the product surfaces the "not fully human-reviewed" notice, and that Cyrillic renders correctly in menus, list columns, and message boxes (FR-016)

**Checkpoint**: All 12 languages ship.

---

## Phase 7: User Story 5 - One command rebuilds every language (Priority: P3)

**Goal**: `build.cmd full` regenerates all 240 language modules and reports
coverage.

**Independent Test**: Run two clean builds and confirm equivalent output; change
one English string, rebuild, and confirm the affected modules are regenerated and
the coverage report reflects it.

### Implementation for User Story 5

- [X] T041 [US5] Invoke `src\vcxproj\build_langs.cmd` from `build.cmd` in `full` mode, after the language projects link and before the plugin-output cleanup step
- [X] T042 [US5] Emit the coverage report from `src/vcxproj/build_langs.cmd` — per (language, module): total, human, machine, english_fallback, discarded, layout_errors (FR-015)
- [X] T043 [US5] Replace the hard-coded `language modules built: %LANG_COUNT% (english)` summary in `build.cmd` (around the `.slg` counting block) with per-language counts and a coverage line
- [X] T044 [US5] Add incremental skipping to `src/vcxproj/build_langs.cmd` — skip a (module, language) pair whose `.slt` and `english.slg` are both older than the existing `<language>.slg`
- [X] T045 [US5] Wire `translator.exe -quiet-validate-layout` into `src/vcxproj/build_langs.cmd` after each import and surface the error count as `layout_errors` in the coverage report (FR-013 / SC-005 gate)
- [ ] T046 [US5] Verify reproducibility: run two clean full builds and confirm equivalent language modules; then change one English string in `src/lang/texts.rc2`, re-run stage 1 + merge + build, and confirm only the affected modules regenerate and the new string appears in every language

**Checkpoint**: A maintainer produces the complete 12-language product with one command.

---

## Phase 8: Polish & Cross-Cutting Concerns

- [ ] T047 Include all 12 languages in the installable product — update the file lists in `src/setup/` so the `lang` directories ship with every `.slg`, not just `english.slg` (FR-027)
- [ ] T048 Fix the dialogs reported by the layout gate — adjust control `cx` (and dialog geometry where needed) in the offending `translations/<language>/<module>.slt` rows, or shorten the text, until `layout_errors` is zero across all 12 languages (SC-005)
- [ ] T049 Review and correct declined product names — scan the Czech, Russian, Ukrainian, Slovak, and Polish-style inflected languages for grammatically wrong `Newt Commander` substitutions produced by `rebrand.py` and fix the surrounding phrase (FR-021, the known residue from research R13)
- [ ] T050 [P] Scan all shipped `.slt` files for predecessor product name, vendor name, and `altap.cz` / predecessor web addresses; confirm zero user-visible occurrences (SC-007)
- [ ] T051 [P] Verify Simplified Chinese and Russian render correctly in menus, dialogs, list columns, and message boxes on a Latin-script Windows installation
- [ ] T052 [P] Rewrite `translations/readme.txt` — it currently documents the dead Salamander-4.0 translator workflow; replace with the three-stage workflow and a pointer to `tools/translate/README.md`
- [ ] T053 [P] Retire or rewrite `translations/!update_langs_from_translator.bat`, which is already self-marked *"outdated = needs to be fixed before using again"* and describes a Mercurial-era import that no longer applies
- [ ] T054 [P] Update `CLAUDE.md` — document the 12 shipped languages, the `translations/languages.cfg` policy file, and the fact that machine translation is a hand-run content step (like `tools/brand/gen_icons.py`), never part of the build
- [ ] T055 Run the full `specs/038-translations-build-integration/quickstart.md` verification sequence end to end
- [ ] T056 Write `specs/038-translations-build-integration/validation-results.md` recording per-language coverage, layout-gate results, screenshots, and the SC-001…SC-011 outcomes, following the pattern of `specs/036-plugin-dark-theme/validation-results.md`

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependencies
- **Foundational (Phase 2)**: depends on Setup — **blocks every user story**
- **US1 (Phase 3)**: depends on Foundational
- **US2 (Phase 4)**: depends on US1 (reuses `merge.py` built there)
- **US3 (Phase 5)**: depends on US2 (fills the gaps its merge left)
- **US4 (Phase 6)**: depends on US3 (reuses the machine-translation driver)
- **US5 (Phase 7)**: depends on Foundational only — can be built in parallel with US1–US4 by a second person
- **Polish (Phase 8)**: depends on all desired stories

### Story Dependency Notes

This feature is more sequential than a typical one, and it is worth being honest
about why: US2–US4 each *extend the same merge tool* rather than adding a
parallel component. They remain independently **testable** (each has its own
verification tasks and delivers a distinct user-visible outcome), but they are
not independently **implementable** from scratch — US2 without US1's `merge.py`
has nothing to run.

US5 is the exception: it wraps the Phase 2 pipeline in build integration and
touches only `build.cmd` and `build_langs.cmd`, so it genuinely parallelises.

### Within Each Story

- Tooling before data generation (write `merge.py` before running it)
- Data generation before build
- Build before run-verification

### Parallel Opportunities

- T002, T003, T004 — three different files, no ordering
- T008 and T013 — `gen_atp.ps1` and `verify_slg.ps1` are independent of the `build_langs.cmd` work
- T016 — `rebrand.py` is independent of `merge.py`'s legacy path
- T029 — `validate.py` is independent of `translate.py`
- T050, T051, T052, T053, T054 — five independent polish tasks
- **US5 (Phase 7) can run alongside Phases 3–6** with a second person

---

## Parallel Example: Phase 2 Foundational

```bash
# Independent files — launch together:
Task: "Create src/vcxproj/gen_atp.ps1 (.atp project generator)"
Task: "Create src/vcxproj/verify_slg.ps1 (FILEVERSION verifier)"

# Then the slt.py chain (T005 → T006 → T007) and the
# build_langs.cmd chain (T009 → T010 → T011 → T012 → T014) run as two
# independent sequences.
```

## Parallel Example: Phase 8 Polish

```bash
Task: "Scan shipped .slt files for predecessor brand strings"
Task: "Verify Chinese and Russian rendering"
Task: "Rewrite translations/readme.txt"
Task: "Retire translations/!update_langs_from_translator.bat"
Task: "Update CLAUDE.md"
```

---

## Implementation Strategy

### MVP First (US1 only)

1. Phase 1 Setup (T001–T004)
2. Phase 2 Foundational (T005–T014) — **critical; proves the pipeline before any translation content exists**
3. Phase 3 US1 (T015–T023)
4. **STOP and VALIDATE**: the application runs in 10 languages
5. Demo — this alone is the headline outcome of the feature

### Incremental Delivery

1. Setup + Foundational → pipeline proven with an English round-trip
2. + US1 → app localized (**MVP**)
3. + US2 → 15 plugins follow along; a full session works except the 4 new plugins
4. + US3 → SFTP/MDView/Folders/Portables localized; all 10 languages complete
5. + US4 → Ukrainian; 12 languages ship
6. + US5 → one-command rebuild with coverage reporting
7. + Polish → installer, layout fixes, brand scan, docs

Each step leaves the product shippable.

### Suggested Parallel Split (two people)

- Person A: Phase 2 → US1 → US2 → US3 → US4 (the content pipeline)
- Person B: Phase 2 assist (T008, T013) → US5 (build integration) → Phase 8 prep

---

## Notes

- **Exit code 1 means success** for `translator.exe`. A conventional
  `if errorlevel 1` check rejects every successful run — see
  `contracts/translator-cli.md`.
- **Every `translator.exe` invocation must be timeout-guarded.** Its error paths
  call `MessageBox`, so an unguarded failure hangs the build instead of failing it.
- **`.slt` import is positional and all-or-nothing.** Never hand-assemble a
  `.slt`; always start from a stage-1 template and replace only text and control
  geometry.
- **The build never calls the Anthropic API.** Machine translation is a hand-run
  content step whose output is committed (FR-023) — this is what makes build
  reproducibility (FR-024) true by construction.
- Human-authored translations always win over machine ones for the same entry
  (FR-017); the merge tool must never overwrite committed human text.
- Commit after each task or logical group; stop at any checkpoint to validate.
