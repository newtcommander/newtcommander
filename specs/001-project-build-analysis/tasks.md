# Tasks: Project Build and Architecture Analysis

**Input**: Design documents from `/specs/001-project-build-analysis/`
**Prerequisites**: plan.md (required), spec.md (required), research.md, data-model.md

**Tests**: Not requested in specification. Manual review only.

**Organization**: Tasks are grouped by user story to enable independent
implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- All deliverables are Markdown files
- Architecture docs: `architecture/` at repository root
- AI context: `CLAUDE.md` at repository root
- Source data: `src/vcxproj/` (solution, props files, project files)

---

## Phase 1: Setup

**Purpose**: Create the architecture directory and establish document structure

- [x] T001 Create `architecture/` directory at repository root
- [x] T002 Create `architecture/README.md` with table of contents listing all 8 planned documents (01 through 08), each with title, one-line description, and relative link

**Checkpoint**: Directory exists with index file linking to all planned documents

---

## Phase 2: Foundational (Shared Data Gathering)

**Purpose**: Gather raw data from the codebase that multiple user stories depend on. These tasks read source files and produce notes that feed into the architecture documents.

**CRITICAL**: No document writing (US1/US2/US3) should begin until this data is gathered.

- [x] T003 Parse `src/vcxproj/salamand.sln` and extract all 90 project entries with GUID, path, and type. Save structured list for use by T006 and T007.
- [x] T004 [P] Read all `.props` files in `src/vcxproj/` and `src/plugins/shared/vcxproj/` — extract preprocessor definitions, compiler flags, linker settings, and output paths per configuration. Save structured notes for T008 and T010.
- [x] T005 [P] Inventory `src/common/dep/` directories and all plugin-embedded libraries. For each: name, location, license file content, version (from headers or changelogs). Save for T009.

**Checkpoint**: Raw data for all 90 projects, all props files, and all dependencies is captured and ready for document writing.

---

## Phase 3: User Story 1 — Developer Reads Architecture Documentation (Priority: P1)

**Goal**: A developer can navigate `architecture/` and understand the full build system, project structure, and dependencies.

**Independent Test**: A developer with no prior Salamander knowledge reads the architecture documents and can set up their environment and build the project on the first attempt.

### Implementation for User Story 1

- [x] T006 [US1] Write `architecture/01-project-overview.md` — high-level summary of Open Salamander: history (from README.md), purpose (two-panel file manager), technology stack (C++20, WinAPI, no frameworks), repository layout (`src/`, `convert/`, `doc/`, `help/`, `tools/`, `translations/`), and link to original README.md. Source: README.md + repo structure.
- [x] T007 [US1] Write `architecture/02-solution-structure.md` — list all 90 projects from salamand.sln organized by category (Core, Plugins, Language Modules, Shell Extensions, Utilities, Setup). For each project: name, GUID, output type (.exe/.spl/.slg/.dll), purpose (one-line), and inter-project dependencies (ProjectReference entries). Include the property sheet inheritance hierarchy diagram. Source: T003 data + .vcxproj files.
- [x] T008 [US1] Write `architecture/03-build-pipeline.md` — document prerequisites (Windows 11, VS2022, C++ workload, Windows SDK, OPENSAL_BUILD_DIR env var), build entry points (rebuild.cmd menu options, build.cmd params, VS IDE), MSBuild configuration flow (props inheritance), output directory structure with all paths, post-build steps (code signing, !populate_build_dir.cmd for redistributables). Source: rebuild.cmd, build.cmd, !populate_build_dir.cmd, T004 data.
- [x] T009 [US1] Write `architecture/04-dependencies.md` — table of all included third-party libraries (name, location, version, license, GPLv2 compatibility, used-by projects), table of missing/external dependencies (pvw32cnv.dll, unrar.dll, OpenSSL, Embarcadero RTL) with impact assessment and proposed OSS replacements. Note: zero NuGet packages, all deps are embedded. Source: T005 data + research.md R2.
- [x] T010 [P] [US1] Write `architecture/07-preprocessor-defs.md` — all preprocessor definitions from sal_base.props, sal_debug.props, sal_release.props, plugin_base.props, plugin_debug.props, plugin_release.props, x86.props, x64.props. Group by scope (base/debug/release/plugin/platform). For each: name, value, purpose. Source: T004 data + research.md R3.
- [x] T011 [P] [US1] Write `architecture/06-plugin-architecture.md` — document the plugin system: plugin API headers (spl_base.h, spl_com.h, spl_fs.h, spl_gen.h, spl_gui.h, spl_file.h), output format (.spl = DLL with .spl extension, .slg = resource-only DLL), plugin loading mechanism, plugin build configuration (plugin_base.props, .def files, base address mapping from baseaddr_x86/x64.txt), shared infrastructure in `src/plugins/shared/`. List all 35 plugins with category and status. Source: T003 data + plugin .vcxproj files + spl_*.h headers.
- [x] T012 [P] [US1] Write `architecture/08-code-standards.md` — document encoding (UTF-8-BOM), formatting (clang-format config, normalize.ps1 + normalize_config.json), C++ standard (C++20 /std:c++latest), unsigned char default (/J flag), comment language policy (Czech legacy OK, new comments in English), precompiled header convention (precomp.h). Source: normalize.ps1, .clang-format, sal_base.props.

**Checkpoint**: 7 architecture documents written (01–04, 06–08). A developer can read these and understand the project. US1 is independently testable.

---

## Phase 4: User Story 2 — AI Assistant Gains Project Context (Priority: P2)

**Goal**: `CLAUDE.md` at project root enables an AI assistant to understand the project without scanning source files.

**Independent Test**: An AI assistant reading only `CLAUDE.md` and linked architecture documents can correctly answer questions about the build system, project structure, and dependencies.

### Implementation for User Story 2

- [x] T013 [US2] Write `CLAUDE.md` at the project root — replace the auto-generated skeleton with a comprehensive AI context file containing: project overview (Open Salamander, two-panel file manager, C++20/WinAPI), current development phase (Phase 1: build system analysis), repository structure summary, build instructions (quick-start: set OPENSAL_BUILD_DIR, open salamand.sln, build), key technical facts (90 projects, 35 plugins, .spl/.slg format, no frameworks), links to all architecture/ documents with one-line descriptions, list of missing dependencies and their status, code conventions summary. Keep under 200 lines for fast AI ingestion.
- [x] T014 [US2] Update `architecture/README.md` — add cross-references to CLAUDE.md, add "Quick Navigation" section mapping common questions to specific documents (e.g., "How to build?" → 03-build-pipeline.md, "What plugins exist?" → 02-solution-structure.md).

**Checkpoint**: CLAUDE.md exists and is self-contained enough for AI context. US2 is independently testable.

---

## Phase 5: User Story 3 — Team Evaluates Alternative Compilers (Priority: P3)

**Goal**: Compiler analysis document enables informed decision on compiler choice.

**Independent Test**: The document lists at least 3 compilers with concrete compatibility assessments, installation instructions, and effort estimates.

### Implementation for User Story 3

- [x] T015 [US3] Write `architecture/05-compiler-comparison.md` — comprehensive compiler evaluation covering: (1) MSVC 2022 v143 — current compiler, installation via winget/VS installer, full compatibility, zero migration effort; (2) Clang-cl (LLVM) — installation, MSBuild ClangCL toolset integration, SEH support status, MSVC pragma compatibility, __declspec support, known WIL/ATL issues, medium effort; (3) MinGW-w64 (GCC) — installation via MSYS2, self-contained (no MSVC needed), smallest install (1.5–2.5 GB), BUT no x86 SEH, no MSBuild, incomplete WinAPI headers, different ABI, verdict: incompatible; (4) Intel ICX — LLVM-based, same as clang-cl with extra weight, no benefit for file manager. Include: MSVC-specific code pattern inventory (SEH in 89 files, 5,301 calling conventions, 609 #pragma once, etc.), comparison matrix, recommendation (MSVC primary, Clang-cl secondary CI). Source: research.md R3 + R4.

**Checkpoint**: Compiler comparison complete. US3 is independently testable.

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Final validation, consistency, and cleanup across all documents

- [x] T016 Review all architecture documents for internal consistency — verify cross-references between documents are correct, all 90 projects mentioned in 02-solution-structure.md match salamand.sln, all dependencies in 04-dependencies.md match src/common/dep/
- [x] T017 [P] Verify `architecture/README.md` links — confirm every document listed in README.md exists and every document in architecture/ is listed in README.md
- [x] T018 [P] Run quickstart.md validation — follow the verification steps in specs/001-project-build-analysis/quickstart.md against the actual deliverables
- [x] T019 Final review of `CLAUDE.md` — verify it stays under 200 lines, all architecture/ links resolve, project overview matches README.md, no stale or incorrect information

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — can start immediately
- **Foundational (Phase 2)**: Depends on Phase 1 (directory must exist)
  — BLOCKS all user story document writing
- **User Story 1 (Phase 3)**: Depends on Phase 2 data gathering
- **User Story 2 (Phase 4)**: Depends on Phase 3 (CLAUDE.md references
  architecture docs that must exist first)
- **User Story 3 (Phase 5)**: Depends on Phase 2 only (can run in
  parallel with US1 if desired, but logically follows)
- **Polish (Phase 6)**: Depends on all user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Phase 2 — no dependencies
  on other stories
- **User Story 2 (P2)**: Depends on US1 (CLAUDE.md links to
  architecture docs created in US1)
- **User Story 3 (P3)**: Can start after Phase 2 — independent of US1
  but benefits from US1 data

### Within Each User Story

- Tasks marked [P] within a story can run in parallel
- Sequential tasks must complete in order listed

### Parallel Opportunities

- T004 and T005 can run in parallel (different source files)
- T010, T011, T012 can all run in parallel (different output files,
  all depend only on Phase 2 data)
- T017 and T018 can run in parallel (independent validation checks)
- US1 and US3 can run in parallel after Phase 2

---

## Parallel Example: User Story 1

```bash
# After Phase 2 completes, launch these in parallel:
Task: "Write architecture/07-preprocessor-defs.md"     # T010
Task: "Write architecture/06-plugin-architecture.md"    # T011
Task: "Write architecture/08-code-standards.md"         # T012

# These must be sequential (each builds on codebase data):
Task: "Write architecture/01-project-overview.md"       # T006
Task: "Write architecture/02-solution-structure.md"     # T007
Task: "Write architecture/03-build-pipeline.md"         # T008
Task: "Write architecture/04-dependencies.md"           # T009
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (T001–T002)
2. Complete Phase 2: Data Gathering (T003–T005)
3. Complete Phase 3: User Story 1 (T006–T012)
4. **STOP and VALIDATE**: Developer can read architecture/ and
   understand the project
5. Deliver if ready — architecture docs are the primary value

### Incremental Delivery

1. Complete Setup + Foundational → Raw data ready
2. Add User Story 1 → architecture/ docs complete → Deliver (MVP!)
3. Add User Story 2 → CLAUDE.md complete → AI assistants have context
4. Add User Story 3 → Compiler comparison complete → Team can decide
5. Polish → All cross-references verified → Final delivery

### Sequential Strategy (Recommended for Solo Dev)

Since this is documentation work by a single author:
1. Phase 1 + Phase 2: Setup and data gathering
2. Phase 3: Write all architecture docs (US1) — this is the bulk
3. Phase 4: Write CLAUDE.md (US2) — quick, references existing docs
4. Phase 5: Write compiler comparison (US3) — uses research.md
5. Phase 6: Polish and validate

---

## Notes

- [P] tasks = different output files, no dependencies
- [Story] label maps task to specific user story for traceability
- Each user story is independently completable and testable
- Commit after each document or logical group of documents
- Stop at any checkpoint to validate story independently
- All tasks produce Markdown files — no source code changes
- Source data comes from: salamand.sln, .vcxproj, .props, .cmd,
  README.md, src/common/dep/, src/plugins/shared/, research.md
