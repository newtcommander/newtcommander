# Tasks: Newt Commander README Rebrand

**Input**: Design documents from `/specs/030-newt-commander-readme/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/readme-content-contract.md, quickstart.md

**Tests**: Not requested — no automated test tasks. Verification is manual per contract and quickstart (the feature is a single Markdown document).

**Organization**: All work targets one file (`README.md`), so tasks within a phase are sequential (no same-file parallel edits). Story phases each deliver a complete, independently checkable part of the document.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files / read-only, no dependencies)
- **[Story]**: US1 (identity), US2 (build), US3 (no duplicated history)

## Phase 1: Setup

**Purpose**: Prepare the replacement document skeleton so each story fills its own sections

- [X] T001 Replace `README.md` content with the new document skeleton: section headings in the order defined by `specs/030-newt-commander-readme/data-model.md` (Title+identity, Development approach, Branding note, Building, Development Process, Repository Structure, License), preserving UTF-8 without BOM + CRLF (research R4)

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Re-confirm the facts every section will assert (guards SC-005 against drift between planning and implementation)

- [X] T002 Verify documented facts are still current: `build.cmd help` output matches research R1 (variants, order-independence, `OPENSAL_BUILD_DIR` default `.\build\`), `plugins.cfg` exists at repo root, and relative link targets exist (`doc/license_gpl.txt`, `doc/third_party.txt`, `AUTHORS`); note any discrepancy back into `specs/030-newt-commander-readme/research.md` before writing

**Checkpoint**: Facts confirmed — story phases can begin

---

## Phase 3: User Story 1 - Visitor Understands Project Identity (Priority: P1) 🎯 MVP

**Goal**: The first screen of `README.md` communicates: name Newt Commander, derived from Open Salamander (link), developed via agentic Spec-Driven Development (SpecKit link, best-models-of-the-era claim, currently Anthropic Fable 5), plus the honest branding note.

**Independent Test**: Open the README; without any other file answer (1) project name, (2) what it derives from, (3) how it is developed. Links C-02/C-04 resolve; branding note present.

### Implementation for User Story 1

- [X] T003 [US1] Write the title + identity section in `README.md`: `# Newt Commander`, one-paragraph identity (two-panel Windows file manager), derivation statement with link to https://github.com/OpenSalamander/salamander, pointer that all history/original docs live upstream (FR-001, FR-002; contract C-01, C-02)
- [X] T004 [US1] Write the development-approach section in `README.md`: new era of agentic programming, Spec-Driven Development principles, built on GitHub SpecKit (link https://github.com/github/spec-kit), combination of agentic frameworks with the best available models for the era — currently Anthropic Fable 5, phrased per research R6 (FR-003, FR-004, FR-005; contract C-03, C-04, C-05)
- [X] T005 [US1] Write the branding note in `README.md`: rebrand is currently at project/documentation level; the built application (window titles, binaries, installer) still carries the Open Salamander name (FR-015; contract C-06)
- [X] T006 [US1] Write the development-process section in `README.md`: features specified → clarified → planned → tasked → implemented via the SpecKit workflow, specifications live in `specs/` (FR-012; contract C-10)
- [X] T007 [US1] Verify US1 acceptance scenarios from `specs/030-newt-commander-readme/spec.md`: first-screen test (SC-001), both external links open correct targets, "Open Salamander" appears only as the upstream project name (contract N-04)

**Checkpoint**: README fully answers who/what/how — MVP deliverable

---

## Phase 4: User Story 2 - Developer Builds the Project (Priority: P2)

**Goal**: A developer following only the README prepares the machine and produces a runnable build via `build.cmd`.

**Independent Test**: On a machine with the prerequisites, follow the build section verbatim; every command behaves as documented.

### Implementation for User Story 2

- [X] T008 [US2] Write the prerequisites subsection in `README.md`: Windows 11+, Visual Studio 2022 (any edition) with "Desktop development with C++" workload, Windows 10/11 SDK (latest installed, `10.0`), optional `OPENSAL_BUILD_DIR` with trailing backslash and default `.\build\` when unset, optional Git / PowerShell 7.4+ for `normalize.ps1` (FR-007; contract C-07; research R2)
- [X] T009 [US2] Write the build-commands subsection in `README.md`: `build.cmd` (incremental Debug x64), `build.cmd rebuild`, `build.cmd release`, `build.cmd full`, `build.cmd full release`, arguments order-independent, `full` also deploys runtime data + `plugins.ver`; note that `plugins.cfg` at repo root decides which plugins are built (FR-006, FR-008; contract C-08, C-09; research R1)
- [X] T010 [US2] Write the repository-structure overview in `README.md`: brief top-level table (`src/`, `src/plugins/`, `specs/`, `architecture/`, `convert/`, `doc/`, `help/`, `tools/`, `translations/`) with one-line purposes, referencing `architecture/` for detail (FR-013; contract C-11)
- [X] T011 [US2] Verify US2 acceptance: run `build.cmd help` and compare against the written text; without `OPENSAL_BUILD_DIR` confirm the documented default appears; optionally run `build.cmd` end-to-end (SC-002, SC-005)

**Checkpoint**: Build section verified against actual script behavior

---

## Phase 5: User Story 3 - Reader Finds Original Project Information (Priority: P3)

**Goal**: No duplicated upstream historical content; the upstream link is the single pointer to history, features, docs, and community.

**Independent Test**: Search the README for the old historical sections — none present; upstream pointer present and sufficient.

### Implementation for User Story 3

- [X] T012 [US3] Sweep `README.md` against prohibited content: no Servant/Altap origin narrative, no upstream Resources list (Altap website/forum/changelogs/Wikipedia), no status/roadmap section; confirm the identity section's upstream pointer covers where that information lives (FR-009, FR-014; contract N-01, N-02, N-03)

**Checkpoint**: All three stories independently verifiable

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: File-level contract, license, and final validation

- [X] T013 Write the License section in `README.md`: GPLv2 (or later) statement with links to `doc/license_gpl.txt` and `doc/third_party.txt`; optionally link `AUTHORS` (FR-010; contract C-12)
- [X] T014 Verify file-level contract on `README.md`: first bytes not `EF BB BF` (UTF-8 no BOM), CRLF endings, English throughout (contract F-01, F-02, C-13; FR-011)
- [X] T015 Walk the full content contract `specs/030-newt-commander-readme/contracts/readme-content-contract.md` — all 13 C-items pass, all 4 N-items find no match, all links resolve (SC-003, SC-004)
- [X] T016 Run `specs/030-newt-commander-readme/quickstart.md` end to end and record the result in `specs/030-newt-commander-readme/` (e.g. a short validation note); confirm GitHub Markdown rendering is clean (headings, table, code blocks)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: none — start immediately
- **Foundational (Phase 2)**: after T001; blocks all stories (facts must be confirmed first)
- **US1 (Phase 3)**: after Phase 2
- **US2 (Phase 4)**: after Phase 2; independent of US1 content (different sections of the file, but edit sequentially to avoid conflicts)
- **US3 (Phase 5)**: after US1 (its sweep checks the final wording of the identity/upstream pointer)
- **Polish (Phase 6)**: after all stories

### Within Each User Story

- Content tasks precede the story's verification task (T007, T011, T012)
- All tasks edit the single file `README.md` → execute sequentially; no [P] on write tasks

### Parallel Opportunities

Limited by design (one file). Genuinely parallelizable: the read-only verifications T014 and T015 can run alongside each other after T013; T002's three fact checks (build.cmd, plugins.cfg, link targets) can run as parallel commands.

---

## Implementation Strategy

**MVP first**: T001 → T002 → Phase 3 (US1). At that point the README already fulfills its core purpose (identity + approach + honesty note) and could ship. Then add US2 (build), US3 (sweep), Polish. Single writer, single file — sequential execution is the practical strategy; total effort is small (~16 tasks, one document).
