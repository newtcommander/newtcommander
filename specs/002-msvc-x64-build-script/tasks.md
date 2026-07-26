# Tasks: MSVC x64 Build Script

**Input**: Design documents from `/specs/002-msvc-x64-build-script/`
**Prerequisites**: plan.md (required), spec.md (required), research.md, data-model.md

**Tests**: Not requested in specification. Manual verification only.

**Organization**: Tasks are grouped by user story to enable independent
implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- Build script: `build.cmd` at repository root
- Existing scripts: `src/vcxproj/build.cmd`, `src/vcxproj/rebuild.cmd` (unchanged)
- Documentation: `CLAUDE.md`, `architecture/03-build-pipeline.md`

---

## Phase 1: Setup

**Purpose**: No setup needed — this is a single-file script feature.

(No tasks — proceed directly to user stories.)

---

## Phase 2: Foundational

**Purpose**: No foundational infrastructure needed — the script is
self-contained.

(No tasks — proceed directly to user stories.)

---

## Phase 3: User Story 1 — Developer Builds the Application (Priority: P1)

**Goal**: A developer runs a single command from the repo root to
build the complete 64-bit Open Salamander.

**Independent Test**: Run `build.cmd` on a machine with VS2022
installed and `OPENSAL_BUILD_DIR` set. Verify salamand.exe and all
plugins are produced.

### Implementation for User Story 1

- [x] T001 [US1] Write `build.cmd` at the repository root with the following functionality: (1) Parse command-line arguments — no args = incremental build, `rebuild` = full clean rebuild, `help` = show usage. (2) Validate OPENSAL_BUILD_DIR is set and has trailing backslash. (3) Locate Visual Studio 2022 using vswhere.exe at `%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe` — query for VS2022+ with Microsoft.VisualStudio.Component.VC.Tools.x86.x64 component. (4) Derive MSBuild.exe path from VS installation root. (5) If vswhere not found, fall back to checking PATH for msbuild, then hardcoded Community path. (6) Record build start time. (7) Change directory to `src\vcxproj\` and run MSBuild: `MSBuild.exe salamand.sln /t:build /p:Configuration=Debug /p:Platform=x64 /m:%NUMBER_OF_PROCESSORS%` (use `/t:rebuild` for rebuild mode). (8) Capture MSBuild exit code. (9) Calculate build duration. (10) Display summary: status (SUCCEEDED/FAILED), configuration, duration, output directory. (11) Exit with MSBuild's exit code (or 1 for prerequisite failures). Source: research.md R1-R4, data-model.md.

- [x] T002 [US1] Test `build.cmd` by running it from the repository root with OPENSAL_BUILD_DIR set. Verify: (1) Script finds MSBuild without errors. (2) All 90 projects compile. (3) `%OPENSAL_BUILD_DIR%newtcommander\Debug_x64\salamand.exe` exists. (4) Plugin .spl files exist in `plugins\` subdirectories. (5) Language .slg files exist. (6) Exit code is 0. (7) Summary shows BUILD SUCCEEDED with duration.

**Checkpoint**: Developer can build the entire application with `build.cmd`. US1 is independently testable.

---

## Phase 4: User Story 2 — Developer Builds After Code Changes (Priority: P2)

**Goal**: Incremental build only recompiles changed files.

**Independent Test**: After a successful build, modify one .cpp file,
run `build.cmd`, verify it completes faster than a full rebuild.

### Implementation for User Story 2

- [x] T003 [US2] Verify that `build.cmd` (default mode, no arguments) performs an incremental build by using MSBuild `/t:build` target. Test by: (1) Running `build.cmd` for a full initial build. (2) Modifying a single source file (e.g., add a comment to `src/bitmap.cpp`). (3) Running `build.cmd` again. (4) Verifying that the second build completes significantly faster. (5) Verifying the output is correct. No code changes needed — T001 already implements incremental mode as default.

**Checkpoint**: Incremental builds work correctly. US2 is independently testable.

---

## Phase 5: User Story 3 — Developer Troubleshoots Build Failures (Priority: P3)

**Goal**: Build errors are clearly reported with project name, file, and line.

**Independent Test**: Introduce a syntax error, run build.cmd, verify
the error output identifies the file and line.

### Implementation for User Story 3

- [x] T004 [US3] Verify that `build.cmd` error reporting works correctly. Test by: (1) Introducing a deliberate syntax error in a source file. (2) Running `build.cmd`. (3) Verifying the output shows the file path, line number, and error description. (4) Verifying the exit code is non-zero. (5) Verifying the summary shows BUILD FAILED. (6) Reverting the syntax error. No code changes needed — MSBuild already outputs errors in `file(line): error C####: message` format, and the script captures MSBuild's exit code per T001.

**Checkpoint**: Build failures are clearly reported. US3 is independently testable.

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Update documentation to reference the new build script.

- [x] T005 [P] Update `CLAUDE.md` at the repository root — change the "Build Quick Start" section to reference `build.cmd` at the repo root (instead of `src\vcxproj\build.cmd`). Add `build.cmd rebuild` for full rebuild. Keep the existing `src\vcxproj\rebuild.cmd` reference as a secondary option.

- [x] T006 [P] Update `architecture/03-build-pipeline.md` — add a new section at the top under "Quick Start" documenting `build.cmd` at the repo root as the primary build entry point. Update the "Build Entry Points" section to list `build.cmd` first, before the existing `src/vcxproj/` scripts.

- [x] T007 Verify the complete build workflow end-to-end per `specs/002-msvc-x64-build-script/quickstart.md` — run all 5 verification steps: (1) Build succeeds with exit code 0. (2) Output files exist (salamand.exe, plugins, lang). (3) Application launches. (4) Missing OPENSAL_BUILD_DIR is detected. (5) Incremental build is faster than full rebuild.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 3 (US1)**: No dependencies — can start immediately
- **Phase 4 (US2)**: Depends on US1 (needs build.cmd to exist)
- **Phase 5 (US3)**: Depends on US1 (needs build.cmd to exist)
- **Phase 6 (Polish)**: Depends on US1 (docs reference the script)

### User Story Dependencies

- **User Story 1 (P1)**: No dependencies — the core script
- **User Story 2 (P2)**: Depends on US1 — uses the same script
- **User Story 3 (P3)**: Depends on US1 — uses the same script

### Parallel Opportunities

- T005 and T006 can run in parallel (different files)
- US2 and US3 are verification tasks that can run in parallel after US1

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete T001: Write build.cmd
2. Complete T002: Test it works
3. **STOP and VALIDATE**: Developer can build the full application
4. Deliver if ready — the build script is the primary value

### Full Delivery

1. T001: Write build.cmd (core implementation)
2. T002: Verify full build works (US1 validation)
3. T003: Verify incremental build works (US2 validation)
4. T004: Verify error reporting works (US3 validation)
5. T005 + T006: Update documentation (parallel)
6. T007: End-to-end verification

---

## Notes

- T001 is the only task that writes new code — all other tasks are
  verification or documentation updates
- US2 and US3 are essentially test/verification stories — the
  functionality is already built into T001's implementation
- The script is ~100–150 lines of Windows Batch
- Existing scripts in src/vcxproj/ are NOT modified
