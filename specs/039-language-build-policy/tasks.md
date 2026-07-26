---

description: "Task list for feature 039 — Language Build Policy"
---

# Tasks: Language Build Policy

**Input**: Design documents from `/specs/039-language-build-policy/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md

**Tests**: No automated test tasks. This feature has no test harness to write
against — the build *is* the harness, and every criterion is checked by running
it and asserting on the output tree or the running product. Verification tasks
are therefore explicit, numbered, and carry their expected result.

**Organization**: Grouped by user story so each can be implemented and validated
independently.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 / US2 / US3 from spec.md
- Exact file paths are given in every task

## Path Conventions

Repository root is `E:\Projects\newtcommander`. Build output is
`%OPENSAL_BUILD_DIR%newtcommander\<Config>_x64` (defaults to `.\build\` when the
variable is unset). Paths below are repository-relative unless stated otherwise.

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Capture the "before" state. This must happen *first* — SC-004 asks
whether re-enabling restores byte-identical modules, and the current build output
is the only place where the pre-disable bytes exist.

- [X] T001 Capture baseline hashes of every built language module into `%TEMP%\newtc-039-baseline-slg.txt` by running `Get-ChildItem "$out" -Recurse -Filter '*.slg' | Get-FileHash | Sort-Object Path`, where `$out` is the current build output tree — this is the reference SC-004 is checked against in T022
- [X] T002 Record the current output inventory (expect 20 `lang` directories × 12 `.slg` = 240 files, 1 app + 19 plugins) in the same scratch file so T013's counts have a documented starting point

**Checkpoint**: the pre-change state is recorded and cannot be lost by a rebuild.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Introduce the policy field and teach both readers about it. Nothing
honours the value yet — that is deliberate, so a parse regression is caught on
its own rather than tangled with behaviour changes.

**⚠️ CRITICAL**: No user story work can begin until this phase is complete.

- [X] T003 Add `enabled = on|off` as the last field of all 11 sections in `translations/languages.cfg`, setting `off` for `[chinesesimplified]`, `[russian]`, `[ukrainian]` and `on` for the other eight (FR-006); update the field documentation block at the top of the file to describe `enabled` alongside `origin`
- [X] T004 [P] Extend `src/vcxproj/read_languages.ps1`: add `enabled` to the `$required` list, validate the value is `on` or `off` with the message from `contracts/languages-cfg.md`, and emit the record as `<folder>|<langid>|<origin>|<enabled>` with `1`/`0` — keep emitting every registered language, enabled or not (research.md D7)
- [X] T005 [P] Extend `tools/translate/config.py`: add `enabled: bool` to `Language`, parse and validate `enabled` in `load_languages()`'s `flush()`, add the `include_disabled: bool = False` parameter, and filter the returned list to enabled languages by default while running `_validate_languages()` over **all** records regardless (research.md D3)
- [X] T006 Run `build.cmd` and `python -m translate.config` to confirm both readers still parse: the build must reach MSBuild, and the Python printer must run without error — this is the gate for the "4th field breaks positional parsing" risk in plan.md before any behaviour depends on it

**Checkpoint**: the policy field exists and validates; behaviour is unchanged.

---

## Phase 3: User Story 1 - Stop shipping a broken language (Priority: P1) 🎯 MVP

**Goal**: A language marked `off` disappears from the product — no module in the
output tree, not offered in the chooser — and it does so on the next build,
without a clean build.

**Independent Test**: Mark a language off, run `build.cmd`, launch the product,
confirm the chooser does not list it and no `.slg` for it remains anywhere in the
output tree.

### Implementation for User Story 1

- [X] T007 [US1] Create `src/vcxproj/lang_policy.ps1` per `contracts/build-scripts.md`: parameters `-Config`, `-TranslationsRoot`, optional `-OutputRoot`; delegate parsing to `read_languages.ps1` and exit 1 printing every error on failure; when `-OutputRoot` exists, reconcile `<OutputRoot>\lang` and `<OutputRoot>\plugins\*\lang` by keeping `english.slg` plus one `.slg` per enabled language and deleting every other `.slg`, printing `Reconcile: removed stale language module <path>` per removal; finish with `Languages: <N> enabled, <M> disabled`
- [X] T008 [US1] Add the language policy stage to `build.cmd` immediately after the plugin policy stage (currently lines 110-134): invoke `lang_policy.ps1` into a log the same way `gen_plugins_filter.ps1` is invoked, echo the log, parse `Languages: <N>` into `ENABLED_LANGS`, delete the log, exit 1 with `Language policy check FAILED. Fix translations\languages.cfg and try again.` on non-zero, and add ` Language policy : %ENABLED_LANGS% of 11 languages enabled ^(languages.cfg^)` to the configuration banner under the existing plugin policy line (FR-009)
- [X] T009 [US1] Update `src/vcxproj/build_langs.ps1` language selection to consume the new fourth record field and iterate **enabled languages only**, and add `  languages skipped (off)  : <n>  -- <names>` to the run summary when any language is disabled
- [X] T010 [US1] Make `src/vcxproj/build_langs.ps1` reject `-Language <folder>` naming a disabled language with `ERROR: language '<folder>' is disabled in languages.cfg -- enable it there to build it`, keeping the existing unregistered-language error unchanged (research.md D6)

### Verification for User Story 1

- [X] T011 [US1] Verify SC-002: run `build.cmd full`, then assert `(Get-ChildItem "$out\lang" -Filter '*.slg').Count` is 9 and that `Get-ChildItem "$out" -Recurse -Include russian.slg,ukrainian.slg,chinesesimplified.slg` returns nothing across all 20 `lang` directories
- [X] T012 [US1] Verify SC-003 (no clean build needed): starting from an output tree that still contains all 12 languages, run a plain `build.cmd` (not `full`) and confirm the three disabled modules are removed **in that same build**, with the `Reconcile: removed stale language module` lines naming all 60 files (3 languages × 20 modules)
- [X] T013 [US1] Verify FR-010 in the running product: launch `newtcommander.exe`, open the language chooser, confirm it lists exactly 9 entries (8 languages + English) and that none of the three disabled ones appear; capture a screenshot into `specs/039-language-build-policy/`
- [X] T014 [US1] Verify FR-011 in the running product: with the product configured for Russian (`HKCU\Software\Newt Commander\0.1` → language value), disable Russian, rebuild, start the product, and confirm it shows the "was not found or is not valid language file … will try to search for some other language file" message and then the chooser — not a crash and not a silent English fallback
- [X] T015 [US1] Verify the plugin side of FR-002 explicitly: confirm each of the 19 `plugins/<name>/lang` directories holds 9 `.slg` files, so the policy is uniform across the app and every plugin

**Checkpoint**: US1 is complete — the requested outcome (stop shipping the three
broken languages) is delivered and verified. This is the MVP; everything after
this makes the decision reversible and the policy file trustworthy.

---

## Phase 4: User Story 2 - Re-enable a language without redoing work (Priority: P2)

**Goal**: Disabling is a reversible policy decision, not a destructive one — the
committed translation source survives untouched, the authoring tools do not
quietly spend budget on a language nobody ships, and re-enabling restores
byte-identical modules.

**Independent Test**: Disable, build, re-enable, build again, and hash-compare the
restored modules against the T001 baseline.

**Note on scope**: FR-012 / FR-013 / SC-008 (authoring-tool behaviour) live in this
phase because they exist to protect exactly this story's promise — a maintainer
must be able to prepare or correct a disabled translation *before* enabling it,
and must not have spent DeepL characters on it in the meantime.

### Implementation for User Story 2

- [X] T016 [US2] Update `tools/translate/merge.py`: take the enabled-only default from `load_languages()`, and when `--language <folder>` names a disabled language, process it after printing `note: '<folder>' is disabled in languages.cfg -- processing it because you named it explicitly` (research.md D4; the explicit name *is* the FR-013 opt-in, no new flag)
- [X] T017 [US2] Update `tools/translate/rebrand.py:149` to call `load_languages(include_disabled=True)` with a comment stating why: rebrand spends no budget, and skipping disabled languages would let brand residue accumulate in their committed `.slt` and surface on re-enable, making FR-005 false in practice (research.md D5)
- [X] T018 [US2] Update `tools/translate/config.py`'s `matrix()` and `main()` to pass `include_disabled=True`, add a `state` column showing `on`/`off` per language, and change the header count to `<N> of <M> languages enabled x <K> modules = …` so `python -m translate.config` shows the whole registry and what ships

### Verification for User Story 2

- [X] T019 [US2] Verify FR-004: after the US1 builds, run `git status --short translations/` and confirm it reports nothing — no `.slt` and no `.origin` sidecar was modified by disabling three languages
- [X] T020 [US2] Verify SC-008: `python -m translate.merge --all --dry-run` reports work for 8 languages only; `python -m translate.merge --language ukrainian --dry-run` reports work for Ukrainian and prints the disabled notice; `python -m translate.merge --language klingon` still errors as unknown
- [X] T021 [US2] Verify the rebrand asymmetry is real: run `python -m translate.rebrand` (residue scan, no `--apply`) and confirm it covers all 11 languages, including the three disabled ones
- [X] T022 [US2] Verify SC-004 (the round trip): set the three languages back to `enabled = on`, run `build.cmd full`, hash every restored `.slg` and compare against `%TEMP%\newtc-039-baseline-slg.txt` from T001 — all 60 must match byte-for-byte
- [X] T023 [US2] Restore the shipped policy after the round trip: set `chinesesimplified`, `russian`, `ukrainian` back to `enabled = off` in `translations/languages.cfg` and rebuild, so the committed state matches FR-006

**Checkpoint**: US1 and US2 both work — the three languages are off, and turning
them back on is proven to cost nothing.

---

## Phase 5: User Story 3 - The policy file is honest about mistakes (Priority: P3)

**Goal**: Every way of getting the policy file wrong fails the build with a
message naming the offending entry, before MSBuild runs — so what shipped can be
trusted.

**Independent Test**: Introduce each error in turn and confirm the build stops
with the specific message.

### Implementation for User Story 3

- [X] T024 [US3] Confirm `src/vcxproj/lang_policy.ps1` collects and prints **all** validation errors before exiting rather than stopping at the first, matching `read_languages.ps1`'s existing `$errors` accumulation, and that it deletes nothing when validation fails

### Verification for User Story 3

- [X] T025 [US3] Verify the FR-007 error set one case at a time against `translations/languages.cfg`, each expected to fail `build.cmd` **before MSBuild starts** with the message from `contracts/languages-cfg.md`: (a) `enabled = maybe`; (b) the `enabled` line deleted; (c) a section renamed to a folder that does not exist — which must report both the missing directory and the now-unregistered one; (d) a duplicate `langid`; (e) a `translations/<x>/` directory with no record
- [X] T026 [US3] Verify SC-006 / FR-008: set every language to `enabled = off`, run `build.cmd full`, confirm it succeeds, that every `lang` directory contains only `english.slg`, and that the product starts and runs in English; then restore the eight-on policy
- [X] T027 [US3] Verify FR-009 / SC-007: confirm the build banner shows `Language policy : 8 of 11 languages enabled (languages.cfg)` and that the `build_langs` summary names the three skipped languages, so the shipped set is visible without inspecting the output tree

**Checkpoint**: all three user stories are independently functional and verified.

---

## Phase 6: Polish & Cross-Cutting Concerns

- [X] T028 [P] Document the policy in `tools/translate/README.md`: the `enabled` field, the enabled-only default, that naming a disabled language is the opt-in, and the deliberate `rebrand` exception with its reason
- [X] T029 [P] Update `architecture/03-build-pipeline.md` so the policy stage description covers languages as well as plugins, including that reconciliation runs on every build while production runs only on `full`
- [X] T030 [P] Add the language build policy to `CLAUDE.md` next to the existing plugin build policy paragraph, and add feature 039 to the Active Technologies / Recent Changes sections — `update-agent-context.sh claude` reported success during planning but left the file unchanged, so this is a manual edit
- [X] T031 [P] Add a note to `translations/ukrainian/README.md` that the language is currently `enabled = off` pending the menu rendering defect, and that its source is retained and complete
- [X] T032 [P] Add a forward pointer in `specs/038-translations-build-integration/quickstart.md` noting that `languages.cfg` gained a required `enabled` field in feature 039, so the format documented there is no longer complete on its own
- [X] T033 Run the full `quickstart.md` procedure end to end as written, correcting any step that does not match the delivered behaviour
- [X] T034 Write `specs/039-language-build-policy/validation-results.md` recording SC-001 … SC-008 with the actual observed evidence (counts, hashes, screenshots, error text), and mark any criterion not run as not run rather than assumed

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependencies — but must run *before* T003 changes anything, or the SC-004 baseline is lost
- **Foundational (Phase 2)**: depends on Setup — **blocks all user stories**
- **US1 (Phase 3)**: depends on Foundational. Delivers the requested outcome
- **US2 (Phase 4)**: depends on Foundational. Independent of US1 in code, but its T022 round-trip is most meaningful after US1 has actually removed the modules
- **US3 (Phase 5)**: depends on Foundational and on T007/T008 existing (the errors must surface through the build stage)
- **Polish (Phase 6)**: depends on the stories it documents

### Within-file constraints (no [P])

- `translations/languages.cfg` — T003, T023, T025, T026 all edit it; strictly sequential
- `src/vcxproj/build_langs.ps1` — T009 then T010
- `src/vcxproj/lang_policy.ps1` — T007 then T024
- `build.cmd` — T008 only
- `tools/translate/config.py` — T005 (Phase 2) then T018 (Phase 4)

### Parallel Opportunities

- T004 and T005 — the two readers, different languages, different files
- T028, T029, T030, T031, T032 — five independent documentation files
- T013 and T014 both need a running product; they can share one launch session but touch different configuration state, so run them in order rather than together

---

## Parallel Example: Phase 2

```text
# The two registry readers can be updated simultaneously:
Task: "Extend src/vcxproj/read_languages.ps1 with the enabled field and 4th record column"
Task: "Extend tools/translate/config.py with Language.enabled and include_disabled"

# Then, sequentially, the gate that proves neither broke anything:
Task: "Run build.cmd and python -m translate.config"
```

## Parallel Example: Phase 6

```text
Task: "Document the policy in tools/translate/README.md"
Task: "Update architecture/03-build-pipeline.md policy stage description"
Task: "Add the language build policy to CLAUDE.md"
Task: "Note the disabled state in translations/ukrainian/README.md"
Task: "Add the forward pointer in specs/038-.../quickstart.md"
```

---

## Implementation Strategy

### MVP First (User Story 1 only)

1. Phase 1 — capture the baseline (irreversible if skipped)
2. Phase 2 — the field and both readers
3. Phase 3 — reconciliation, the build stage, enabled-only production
4. **STOP and VALIDATE**: T011–T015. At this point the user's actual request is
   satisfied: Simplified Chinese, Russian and Ukrainian no longer reach users.

### Incremental Delivery

1. Setup + Foundational → the policy exists and parses, nothing behaves differently
2. + US1 → **the three broken languages stop shipping** (MVP)
3. + US2 → the decision is proven reversible and the authoring tools respect it
4. + US3 → the policy file can be trusted to be honest about mistakes
5. + Polish → documented, validated, recorded

### Notes

- Commit after each phase, not each task — several tasks edit the same file
- T001 is the only task that cannot be done later; everything else is recoverable
- No C++ is touched, so `clang-format` and the UTF-8-BOM source rule do not apply
  to any file in this feature. `languages.cfg` is UTF-8 **without** BOM — both
  readers depend on that
- The menu rendering defect is out of scope by decision. If a task tempts you
  toward diagnosing it, that is a separate feature (see spec.md, "Known defect
  recorded, not fixed here")
