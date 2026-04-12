# Tasks: SpecKit & AI Development Review

**Input**: Design documents from `specs/003-speckit-review/`
**Prerequisites**: plan.md (required), spec.md (required for user stories), research.md, data-model.md, quickstart.md

**Tests**: Not requested for this feature (audit/review — no code to test).

**Organization**: Tasks are grouped by user story to enable independent verification of each area.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (independent verification areas)
- **[Story]**: Which user story this task belongs to (US1, US2, US3)
- All paths are relative to repository root

---

## Phase 1: Setup

**Purpose**: Prepare the verification environment and tooling

- [ ] T001 Read `.specify/init-options.json` and record version, integration type, and branch numbering mode
- [ ] T002 Read `.specify/integration.json` and record version and script references
- [ ] T003 Read `.specify/extensions.yml` and extract all hook definitions with their enabled/optional status
- [ ] T004 Read `.specify/extensions/.registry` and extract registered extensions and commands

**Checkpoint**: All configuration data collected — verification can begin

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Establish baseline data that all verification stories depend on

**CRITICAL**: No user story verification can begin until baseline data is collected

- [ ] T005 Read `.specify/integrations/claude.manifest.json` and extract file list with SHA-256 hashes (11 entries)
- [ ] T006 [P] Read `.specify/integrations/speckit.manifest.json` and extract file list with SHA-256 hashes (11 entries)
- [ ] T007 Cross-reference version numbers across T001-T006 outputs and flag any inconsistencies

**Checkpoint**: Baseline manifest and config data ready — story verification can proceed

---

## Phase 3: User Story 1 — SpecKit Integration Verification (Priority: P1) MVP

**Goal**: Confirm that all SpecKit configuration, manifests, skills, templates, extensions, and scripts are consistent and functional after the 0.6.1 transition.

**Independent Test**: Run each SpecKit skill command in sequence on a test feature — all commands complete without errors and produce correct output files.

### Implementation for User Story 1

- [ ] T008 [P] [US1] Verify all 11 files in `claude.manifest.json` exist at their declared paths in `.claude/skills/` and `.specify/integrations/claude/scripts/`
- [ ] T009 [P] [US1] Verify all 11 files in `speckit.manifest.json` exist at their declared paths in `.specify/templates/` and `.specify/scripts/`
- [ ] T010 [P] [US1] Verify git extension command definitions: 5 `.md` files in `.specify/extensions/git/commands/`
- [ ] T011 [P] [US1] Verify git extension bash scripts: 4 `.sh` files in `.specify/extensions/git/scripts/bash/`
- [ ] T012 [P] [US1] Verify git extension PowerShell scripts: 4 `.ps1` files in `.specify/extensions/git/scripts/powershell/`
- [ ] T013 [P] [US1] Verify git extension config files: `extension.yml`, `config-template.yml`, `git-config.yml`, `README.md` in `.specify/extensions/git/`
- [ ] T014 [US1] Cross-reference registered commands in `.registry` against actual skill files in `.claude/skills/` and command definitions in `.specify/extensions/git/commands/`
- [ ] T015 [US1] Cross-reference hook commands in `extensions.yml` against registered commands in `.registry`
- [ ] T016 [US1] Document verification results for US1 in `specs/003-speckit-review/research.md` section R1-R3

**Checkpoint**: SpecKit integration verified — all 73 infrastructure files confirmed present and consistent

---

## Phase 4: User Story 2 — Existing Specifications and Architecture Review (Priority: P2)

**Goal**: Verify that the two completed specifications and the architecture documentation accurately reflect the current project state.

**Independent Test**: Compare key facts in each document against the actual codebase state and verify accuracy.

### Implementation for User Story 2

- [ ] T017 [P] [US2] Verify all 8 architecture documents exist in `architecture/` (01 through 08)
- [ ] T018 [P] [US2] Verify `architecture/README.md` links to all 8 documents with correct paths and descriptions
- [ ] T019 [P] [US2] Verify `CLAUDE.md` at project root accurately references architecture directory and lists all key project facts
- [ ] T020 [P] [US2] Verify `build.cmd` exists at repo root and supports the modes described in spec 002 (incremental, rebuild, release)
- [ ] T021 [P] [US2] Verify `specs/001-project-build-analysis/` contains all 7 expected artifacts (spec, plan, research, data-model, quickstart, tasks, checklists/requirements)
- [ ] T022 [P] [US2] Verify `specs/002-msvc-x64-build-script/` contains all 7 expected artifacts (spec, plan, research, data-model, quickstart, tasks, checklists/requirements)
- [ ] T023 [US2] Verify spec 001 references to architecture documents — confirm each referenced document exists and covers the described topic
- [ ] T024 [US2] Verify spec 002 build script deliverable — confirm `build.cmd` behavior matches spec requirements (FR-001 through FR-010)
- [ ] T025 [US2] Document verification results for US2 in `specs/003-speckit-review/research.md` section R4-R6

**Checkpoint**: Existing work verified — architecture docs, specs, and build script confirmed accurate

---

## Phase 5: User Story 3 — Gap Analysis and Next Steps (Priority: P3)

**Goal**: Produce a clear summary of what is valid, what needs updating, and recommended next development steps.

**Independent Test**: The review produces a written summary categorizing all items with actionable next steps.

### Implementation for User Story 3

- [ ] T026 [US3] Compile all verification results from T016 and T025 into a categorized summary (valid / needs-update / missing)
- [ ] T027 [US3] For any items categorized as "needs-update", document the specific file, issue, and suggested resolution
- [ ] T028 [US3] For any items categorized as "missing", document what is expected, where it should be, and how to create it
- [ ] T029 [US3] Write recommended next development steps in `specs/003-speckit-review/quickstart.md` based on findings
- [ ] T030 [US3] Update `specs/003-speckit-review/plan.md` Verification Results table with final status

**Checkpoint**: All verification complete — summary produced with actionable next steps

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Final documentation and cleanup

- [ ] T031 [P] Update `specs/003-speckit-review/checklists/requirements.md` with final pass/fail status
- [ ] T032 [P] Verify no orphaned files exist in `.specify/`, `.claude/`, `specs/`, or `architecture/` (files not referenced by any config or index)
- [ ] T033 Run quickstart.md validation — confirm all next steps are actionable and reference existing infrastructure

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion — BLOCKS all user stories
- **User Story 1 (Phase 3)**: Depends on Foundational phase
- **User Story 2 (Phase 4)**: Depends on Foundational phase — can run in parallel with US1
- **User Story 3 (Phase 5)**: Depends on US1 and US2 completion (needs their results)
- **Polish (Phase 6)**: Depends on all user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Foundational (Phase 2) — no dependencies on other stories
- **User Story 2 (P2)**: Can start after Foundational (Phase 2) — no dependencies on US1, can run in parallel
- **User Story 3 (P3)**: Depends on US1 AND US2 — compiles their results into a summary

### Parallel Opportunities

- T001-T004 (Setup): Sequential reads, low overhead — run sequentially
- T005-T006 (Foundational): Can run in parallel (independent manifests)
- T008-T013 (US1): All marked [P] — verify 6 independent file groups in parallel
- T017-T022 (US2): All marked [P] — verify 6 independent document groups in parallel
- US1 and US2 (Phase 3 + Phase 4): Can run entirely in parallel
- T031-T032 (Polish): Can run in parallel

---

## Parallel Example: User Story 1

```bash
# Launch all file group verifications in parallel:
Task: "Verify claude.manifest files exist (11 files)"
Task: "Verify speckit.manifest files exist (11 files)"
Task: "Verify git extension commands (5 files)"
Task: "Verify git extension bash scripts (4 files)"
Task: "Verify git extension PowerShell scripts (4 files)"
Task: "Verify git extension config files (4 files)"

# Then sequential cross-reference checks:
Task: "Cross-reference registry vs skills"
Task: "Cross-reference hooks vs registry"
Task: "Document US1 results"
```

## Parallel Example: User Story 2

```bash
# Launch all document verifications in parallel:
Task: "Verify 8 architecture documents"
Task: "Verify architecture README links"
Task: "Verify CLAUDE.md accuracy"
Task: "Verify build.cmd exists and matches spec"
Task: "Verify spec 001 artifacts (7 files)"
Task: "Verify spec 002 artifacts (7 files)"

# Then sequential content checks:
Task: "Verify spec 001 references"
Task: "Verify spec 002 deliverable"
Task: "Document US2 results"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (collect config data)
2. Complete Phase 2: Foundational (collect manifest data)
3. Complete Phase 3: User Story 1 (verify SpecKit infrastructure)
4. **STOP and VALIDATE**: Is the SpecKit workflow functional?
5. If yes — infrastructure confirmed, can start new features

### Incremental Delivery

1. Setup + Foundational -> Config baseline ready
2. User Story 1 -> SpecKit verified -> Can resume AI-assisted development (MVP!)
3. User Story 2 -> Architecture and specs verified -> Full context confidence
4. User Story 3 -> Gap analysis complete -> Clear roadmap for next work
5. Each story adds verification confidence without breaking previous results

---

## Notes

- [P] tasks = independent file groups, no cross-dependencies
- [Story] label maps task to specific verification area
- This is an audit feature — all tasks are read-only verification, no code changes
- Results already captured in research.md during the planning phase
- Tasks T008-T025 formalize the verification that was performed by research agents
- Commit after completing each user story phase
