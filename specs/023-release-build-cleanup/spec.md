# Feature Specification: Clean Release Build Output (No Intermediate / saltests Directories)

**Feature Branch**: `023-release-build-cleanup`  
**Created**: 2026-07-19  
**Status**: Draft  
**Input**: User description: "Uprav release build aplikace tak, aby vysledny build adresar neobsahoval Intermediate adresar a saltests adresar - ani jeden z nich neni potreba pro release"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Release output free of intermediate build artifacts (Priority: P1)

As someone producing a Release build of Open Salamander, when I run the release build the resulting output directory (`…\newtcommander\Release_x64\`) must not contain any `Intermediate` directory — neither the top-level one nor the nested ones inside plugins, language modules, and helper projects. Intermediate compilation artifacts (object files, precompiled headers, incremental-link state) are build-time scaffolding and have no place in a distributable release layout.

**Why this priority**: This is the core of the request and delivers the primary value — a clean, ship-ready release output. It is independently valuable even if nothing else changes.

**Independent Test**: Run a Release build, then recursively scan the release output tree; verify that zero directories named `Intermediate` exist anywhere beneath it.

**Acceptance Scenarios**:

1. **Given** a clean checkout, **When** I produce a Release build, **Then** the release output tree contains no `Intermediate` directory at any nesting level.
2. **Given** a previous Debug build that created `Intermediate` directories under the same build root, **When** I produce a Release build, **Then** the Release output tree is still free of any `Intermediate` directory (the Release layout is independent of Debug's).
3. **Given** a Release build already produced, **When** I run an incremental Release build again, **Then** it completes successfully and the output remains free of `Intermediate` directories.

---

### User Story 2 - Release output free of test binaries (Priority: P1)

As someone producing a Release build, the resulting output directory must not contain the `saltests` directory (the unit-test executable and its build artifacts). Test binaries are a development/CI concern and are not part of the shipped product.

**Why this priority**: Also explicitly requested and equally essential to a clean release layout; delivers value independently of User Story 1.

**Independent Test**: Run a Release build and verify no `saltests` directory exists anywhere in the release output tree.

**Acceptance Scenarios**:

1. **Given** a clean checkout, **When** I produce a Release build, **Then** the release output tree contains no `saltests` directory.
2. **Given** tests must remain available to developers, **When** I run the unit tests in the Debug configuration, **Then** the unit-test binary is still produced and runnable there.

---

### User Story 3 - Debug build behaviour unchanged (Priority: P2)

As a developer iterating in the Debug configuration, my Debug build must keep its `Intermediate` directories and its `saltests` binary so that incremental builds stay fast and unit tests remain immediately available. The cleanup must not leak into or degrade the Debug inner loop.

**Why this priority**: A guardrail so the release-focused cleanup does not harm day-to-day development. Lower priority than P1 because it protects existing behaviour rather than adding new value.

**Independent Test**: Run a Debug build and confirm both `Intermediate` directories and the `saltests` binary are still present, exactly as before this change.

**Acceptance Scenarios**:

1. **Given** the Debug configuration, **When** I build, **Then** the `Intermediate` directories and `saltests` output remain present as they were before this feature.

---

### Edge Cases

- **Incremental Release rebuild**: Removing intermediates from the output must not force a full recompilation of the whole solution on every subsequent Release build — a no-op incremental Release build must remain cheap.
- **Shipped debugging symbols**: The application and plugin PDB files (`salamand.pdb`, per-plugin PDBs) live at the output root / plugin directories, *not* inside `Intermediate`, and MUST be preserved.
- **Shared build root**: When Debug and Release are built into the same `OPENSAL_BUILD_DIR` root at different times, each configuration's layout must be governed independently (cleaning Release must not remove Debug artifacts and vice versa).
- **Alternative build entry points**: The behaviour must be consistent whether the build is produced via the root `build.cmd` or the alternative full-solution scripts under `src\vcxproj\`.
- **Empty-directory residue**: After the build, no empty `Intermediate` or `saltests` folder shells may remain in the release tree.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: A Release build MUST produce a release output directory tree that contains no directory named `Intermediate` at any nesting level.
- **FR-002**: A Release build MUST produce a release output directory tree that contains no `saltests` directory.
- **FR-003**: The Release build MUST continue to produce all runtime deliverables unchanged: `salamand.exe`, `salamand.pdb`, the application language module, all enabled plugins (`.spl` files plus their language modules), `plugins.ver`, and the runtime data directories (`convert`, `toolbars`, `utils`).
- **FR-004**: The change MUST NOT alter the Debug build's output layout; a Debug build retains its `Intermediate` directories and its `saltests` binary.
- **FR-005**: Incremental Release builds MUST remain functional — a second Release build with no source changes MUST NOT trigger a full recompilation of the whole solution solely as a consequence of this change.
- **FR-006**: Release debugging symbols (application and plugin PDBs) MUST remain present in the release output; only intermediate build scaffolding is removed.
- **FR-007**: The unit tests MUST remain buildable and runnable in at least the Debug configuration.
- **FR-008**: The release output tree MUST NOT retain empty `Intermediate` or `saltests` folder shells after the build completes.

### Key Entities *(include if feature involves data)*

- **Release output directory tree**: the folder `…\newtcommander\Release_x64\` and everything beneath it — the artifact whose cleanliness this feature governs.
- **Intermediate artifacts**: object files, precompiled headers, and incremental-link/build state produced during compilation; build-time only, never a runtime deliverable.
- **saltests output**: the unit-test executable and its build artifacts; a development/CI asset, not a shipping deliverable.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: After a Release build, a recursive search of the release output tree returns **zero** directories named `Intermediate`.
- **SC-002**: After a Release build, a recursive search of the release output tree returns **zero** `saltests` directories.
- **SC-003**: 100% of the runtime deliverables present before the change are still present after the change (`salamand.exe`, application and plugin PDBs, application language module, all enabled plugins and their language modules, `plugins.ver`, `convert`, `toolbars`, `utils`).
- **SC-004**: A no-op incremental Release build (no source changes) completes without recompiling the entire solution — its duration is comparable to a normal incremental build, not to a full clean rebuild.
- **SC-005**: The Debug build output remains structurally unchanged: `Intermediate` directories and `saltests` are still present after a Debug build.
- **SC-006**: The unit-test binary can still be produced and executed in the Debug configuration.

## Assumptions

- Scope is limited to the **Release** configuration (x64). The Debug configuration is intentionally left untouched to preserve fast incremental development and local test runs.
- "Intermediate" refers to **every** intermediate directory in the release output tree — the main application's top-level `Intermediate` plus the nested ones under plugins, language modules, and helper projects — because none of them are needed to run or distribute a release.
- "saltests" refers to the unit-test project's output; tests are treated as a development/CI asset, not a shipping deliverable, so they need not appear in the release output at all.
- The requirement governs the **final state** of the release output directory. Whether intermediates are relocated outside the output tree or removed as a build step is an implementation choice deferred to planning, provided FR-005 (working incremental builds) still holds.
- Builds are produced via the repository's build tooling (root `build.cmd` and the MSBuild project/property-sheet configuration). The alternative full-solution scripts under `src\vcxproj\` are expected to behave consistently but are a secondary concern.
- Release PDB files (application and plugins) reside outside `Intermediate` and are therefore unaffected by removing intermediate scaffolding.
