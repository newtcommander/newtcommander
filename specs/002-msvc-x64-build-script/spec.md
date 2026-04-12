# Feature Specification: MSVC x64 Build Script

**Feature Branch**: `002-msvc-x64-build-script`
**Created**: 2026-03-20
**Status**: Draft
**Input**: User description: "Based on the architecture analysis recommendation, use MSVC 2022 to build the 64-bit version of the application and create an appropriate build script that builds the entire application."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Developer Builds the Application (Priority: P1)

A developer clones the repository and wants to build the complete
64-bit version of Open Salamander. They run a single build script
from the command line without needing to open Visual Studio. The
script compiles all projects (main application, plugins, language
modules, utilities) and produces a ready-to-run build output.

**Why this priority**: Building the application is the fundamental
prerequisite for all future development. Without a reliable,
automated build, no code changes can be verified.

**Independent Test**: Run the build script on a clean machine with
only the documented prerequisites installed. The script completes
without errors and produces a working salamand.exe with all plugins.

**Acceptance Scenarios**:

1. **Given** a fresh clone of the repository and the documented
   prerequisites installed, **When** the developer runs the build
   script, **Then** the entire 64-bit application is compiled
   successfully with zero errors.
2. **Given** a successful build, **When** the developer navigates
   to the build output directory, **Then** they find salamand.exe,
   all plugin .spl files, all language .slg files, and all utility
   executables.
3. **Given** the build output, **When** the developer launches
   salamand.exe, **Then** the application starts and the file
   manager UI is displayed.

---

### User Story 2 - Developer Builds After Code Changes (Priority: P2)

A developer makes a change to a source file and wants to rebuild
only the affected projects. They run the build script in incremental
mode (not a full rebuild) so that only changed files are recompiled,
saving time during the edit-compile-test cycle.

**Why this priority**: Fast iteration is essential for productive
development. A full rebuild of 90 projects is time-consuming;
incremental builds are needed for day-to-day work.

**Independent Test**: Modify a single source file, run the build
script, and verify that only affected projects are recompiled (not
the entire solution).

**Acceptance Scenarios**:

1. **Given** a previous successful build, **When** the developer
   modifies one source file and runs the build script, **Then**
   only the affected project(s) are recompiled.
2. **Given** an incremental build, **When** the build completes,
   **Then** the output is identical to what a full rebuild would
   produce for the changed project(s).

---

### User Story 3 - Developer Troubleshoots Build Failures (Priority: P3)

A developer runs the build script and it fails due to a compilation
or linking error. The script provides clear output showing which
project failed, the exact error message, and enough context to
diagnose the problem without searching through large log files.

**Why this priority**: Build errors are inevitable during development.
Clear, actionable error reporting reduces time spent debugging build
issues.

**Independent Test**: Intentionally introduce a syntax error in a
source file, run the build script, and verify the error output
clearly identifies the file, line, and nature of the problem.

**Acceptance Scenarios**:

1. **Given** a source file with a compilation error, **When** the
   build script runs, **Then** the error message includes the file
   path, line number, and error description.
2. **Given** a build failure, **When** the script exits, **Then**
   it returns a non-zero exit code indicating failure.
3. **Given** a build failure in one project, **When** the developer
   reviews the output, **Then** they can identify which project
   failed without scrolling through output from successful projects.

---

### Edge Cases

- What happens when `OPENSAL_BUILD_DIR` environment variable is not
  set? The script MUST provide a clear error message.
- What happens when Visual Studio 2022 or the C++ workload is not
  installed? The script MUST detect this and report what is missing.
- What happens when the Windows SDK is not installed or is a
  different version? The script MUST report the issue clearly.
- How does the build handle plugins with missing external
  dependencies (unrar.dll, OpenSSL, pvw32cnv.dll)? Those plugins
  MUST still compile (they just won't function fully at runtime).

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The build script MUST compile the complete 64-bit
  (x64) Debug configuration of Open Salamander using MSVC 2022.
- **FR-002**: The build script MUST be runnable from the command
  line without requiring Visual Studio IDE to be open.
- **FR-003**: The build script MUST build all solution projects:
  main application, all 35 plugins, all 36 language modules, shell
  extensions, utilities, and helper libraries.
- **FR-004**: The build script MUST validate prerequisites before
  starting the build: presence of MSVC 2022 compiler, Windows SDK,
  and the `OPENSAL_BUILD_DIR` environment variable.
- **FR-005**: The build script MUST support both full rebuild
  (clean + build) and incremental build (build only changed files).
- **FR-006**: The build script MUST return a non-zero exit code on
  any build failure.
- **FR-007**: The build script MUST display a summary at the end
  indicating success or failure, total build time, and error count.
- **FR-008**: The build output MUST be placed in the directory
  specified by `OPENSAL_BUILD_DIR` following the existing output
  structure (salamand.exe, plugins/, lang/).
- **FR-009**: The build script MUST use parallel compilation to
  utilize all available processor cores.
- **FR-010**: The build script MUST be located at the repository
  root or in a documented, easily discoverable location.

### Key Entities

- **Build Script**: The primary entry point for building the
  application — has a name, location, supported parameters
  (rebuild vs. incremental), and prerequisite checks.
- **Build Output**: The collection of compiled binaries — includes
  the main executable, plugins, language files, and utilities,
  organized in the standard output directory structure.
- **Build Configuration**: The combination of compiler settings —
  Debug x64 as the primary target, with defined preprocessor
  definitions, optimization level, and output paths.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A developer can build the complete 64-bit application
  by running a single command from the repository root.
- **SC-002**: The build script detects missing prerequisites and
  reports all issues before attempting compilation.
- **SC-003**: A full rebuild of all 90 projects completes without
  manual intervention (no interactive prompts during build).
- **SC-004**: The build output contains a functional salamand.exe
  that launches and displays the file manager interface.
- **SC-005**: An incremental build after a single-file change
  completes significantly faster than a full rebuild.
- **SC-006**: Build failures produce error output that identifies
  the failing project and the specific error within 5 lines of
  the failure message.
