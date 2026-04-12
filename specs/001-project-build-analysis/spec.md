# Feature Specification: Project Build and Architecture Analysis

**Feature Branch**: `001-project-build-analysis`
**Created**: 2026-03-20
**Status**: Draft
**Input**: User description: "Perform detailed analysis of the entire project, map required technologies for compiling the program into executable form, analyze different compilation options — different compilers that could be used. The result will be a detailed analysis stored in the architecture directory at the project root. CLAUDE.md will describe what was done and reference this directory and its mapped architecture for future development context."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Developer Reads Architecture Documentation (Priority: P1)

A new or returning developer opens the project and needs to understand
the full build system, project structure, dependencies, and how to
compile Open Salamander into a working executable. They navigate to
the `architecture/` directory and find a complete, organized set of
documents that answer all their questions without needing to read
source code or build scripts.

**Why this priority**: Without clear architecture documentation, every
contributor must reverse-engineer the build system independently,
wasting time and increasing the risk of incorrect builds.

**Independent Test**: A developer with no prior Salamander knowledge
can read the architecture documents and successfully set up their
environment and build the project on the first attempt.

**Acceptance Scenarios**:

1. **Given** a fresh clone of the repository, **When** the developer
   reads `architecture/README.md`, **Then** they find a table of
   contents linking to all analysis documents with clear descriptions.
2. **Given** the architecture documentation, **When** the developer
   looks for build prerequisites, **Then** they find a complete list
   of required tools, SDKs, and environment variables.
3. **Given** the architecture documentation, **When** the developer
   searches for project structure information, **Then** they find a
   document mapping all 70+ projects in the solution, their roles,
   dependencies, and output artifacts.

---

### User Story 2 - AI Assistant Gains Project Context (Priority: P2)

An AI coding assistant (Claude or similar) starts a new conversation
about the Open Salamander project. It reads `CLAUDE.md` at the project
root, which provides a concise project summary and points to the
`architecture/` directory. The assistant can then load relevant
architecture documents to understand the codebase before making changes.

**Why this priority**: Future development phases will rely heavily on
AI-assisted coding. Providing structured context reduces errors and
eliminates repeated exploration of the same codebase.

**Independent Test**: An AI assistant reading only `CLAUDE.md` and the
linked architecture documents can correctly answer questions about the
build system, project structure, and dependencies without scanning
source files.

**Acceptance Scenarios**:

1. **Given** `CLAUDE.md` exists at the project root, **When** an AI
   assistant reads it, **Then** it contains a project overview, current
   development phase, and references to all architecture documents.
2. **Given** the architecture documents, **When** the assistant needs
   to understand a specific subsystem (e.g., plugin build process),
   **Then** a dedicated document exists covering that topic.

---

### User Story 3 - Team Evaluates Alternative Compilers (Priority: P3)

A team member wants to evaluate whether Open Salamander can be built
with compilers other than MSVC (e.g., Clang/LLVM for Windows, MinGW,
Intel C++). They consult the compiler analysis document to understand
compatibility constraints, required changes, and trade-offs for each
alternative.

**Why this priority**: Compiler diversity improves code quality
(different warnings catch different bugs) and may enable future
optimizations. However, this is secondary to establishing the primary
MSVC build.

**Independent Test**: The compiler analysis document lists at least
three compiler options with concrete compatibility assessments and
effort estimates.

**Acceptance Scenarios**:

1. **Given** the compiler analysis document, **When** the reader looks
   for MSVC compatibility details, **Then** they find the exact
   toolset version, C++ standard level, and platform-specific settings.
2. **Given** the compiler analysis document, **When** the reader looks
   for alternative compiler options, **Then** each option includes
   compatibility assessment, known blockers, and estimated effort.

---

### Edge Cases

- What happens when the existing build scripts reference hardcoded
  paths that no longer exist (e.g., specific VS2022 redistributable
  versions)?
- How does the analysis handle plugins with missing external
  dependencies (UnRAR, OpenSSL, PictView engine)?
- What if the project contains components that require non-MSVC
  compilers (e.g., WinSCP plugin needing Embarcadero C++ Builder)?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Analysis MUST document every project in the Visual Studio
  solution (`salamand.sln`), including its type (exe, dll, spl, slg),
  purpose, and dependencies on other projects.
- **FR-002**: Analysis MUST list all third-party libraries in
  `src/common/dep/` and any external dependencies, including their
  versions, licenses, and GPLv2 compatibility status.
- **FR-003**: Analysis MUST document the complete build pipeline from
  source to executable, including all build configurations (Debug/
  Release, x86/x64, Utils), environment variables, and required tools.
- **FR-004**: Analysis MUST identify and document all missing or
  unavailable dependencies (pvw32cnv.dll, unrar.dll, OpenSSL, etc.)
  and their impact on which plugins can be built.
- **FR-005**: Analysis MUST evaluate at least three compiler options
  (MSVC, Clang/LLVM, and one other) with compatibility assessment for
  the Open Salamander codebase.
- **FR-006**: Analysis MUST document all preprocessor definitions and
  their purpose, grouped by configuration (base, debug, release,
  plugin, platform).
- **FR-007**: Analysis MUST document the plugin architecture, including
  the plugin API (spl_*.h headers), plugin output format (.spl/.slg),
  and the plugin loading mechanism.
- **FR-008**: `CLAUDE.md` MUST exist at the project root with a
  project summary, current development phase, and references to all
  architecture documents.
- **FR-009**: All analysis documents MUST be stored in the
  `architecture/` directory at the project root.
- **FR-010**: Analysis MUST document the code formatting and encoding
  requirements (UTF-8-BOM, clang-format, normalize.ps1).

### Key Entities

- **Solution Project**: A build unit within salamand.sln — has a name,
  type, configuration, output path, and inter-project dependencies.
- **Third-Party Dependency**: An external library included or required
  by the project — has a name, version, license, location, and
  availability status.
- **Build Configuration**: A combination of build type (Debug/Release/
  Utils) and platform (x86/x64) — has specific preprocessor defines,
  compiler flags, and output paths.
- **Compiler Option**: A potential compiler toolchain — has a name,
  version, compatibility level, known blockers, and migration effort.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: The architecture documentation covers 100% of projects
  listed in the Visual Studio solution file.
- **SC-002**: A developer with Windows 11 and Visual Studio 2022 can
  follow the documentation to produce a successful build within 30
  minutes of reading.
- **SC-003**: All missing/unavailable dependencies are identified with
  proposed open-source replacements or workarounds.
- **SC-004**: At least 3 compiler options are evaluated with clear
  compatibility verdicts (compatible, partially compatible, or
  incompatible) and rationale.
- **SC-005**: `CLAUDE.md` enables an AI assistant to understand the
  project structure and build system without additional source code
  exploration in under 2 minutes of reading.
- **SC-006**: Every document in `architecture/` has a clear title,
  purpose statement, and is linked from the directory's README.
