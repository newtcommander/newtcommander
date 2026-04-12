<!--
Sync Impact Report
===================
Version change: 0.0.0 → 1.0.0 (initial adoption)
Modified principles: N/A (first version)
Added sections:
  - Core Principles (5 principles)
  - Technical Constraints
  - Development Workflow
  - Governance
Removed sections: N/A
Templates requiring updates:
  - .specify/templates/plan-template.md — ✅ no update needed (generic Constitution Check gate)
  - .specify/templates/spec-template.md — ✅ no update needed (generic structure)
  - .specify/templates/tasks-template.md — ✅ no update needed (generic structure)
  - .specify/templates/commands/*.md — ✅ no command files exist
Follow-up TODOs: none
-->

# Open Salamander Constitution

## Core Principles

### I. Build Reproducibility

Every developer and CI environment MUST produce identical binaries
from the same source revision. The build system MUST be fully
automated and runnable from a single command. Build artifacts MUST
be clearly separated from source (via `OPENSAL_BUILD_DIR` or a
default output directory). No manual steps (copying DLLs, editing
registry, running GUI wizards) are permitted in the build pipeline.

**Rationale**: The first project phase focuses on establishing a
reliable build. Without reproducible builds, all subsequent work
(bug fixes, new features) cannot be verified or distributed.

### II. Backward Compatibility

Existing Open Salamander 5.0 functionality MUST NOT regress unless
explicitly deprecated with documented justification. User-facing
behavior changes MUST be opt-in or gated behind version checks.
Plugin API changes MUST maintain binary compatibility or provide a
clear migration path with a deprecation cycle of at least one
minor release.

**Rationale**: Open Salamander has an established user base. Trust
is maintained by preserving the behavior users depend on.

### III. Incremental Modernization

Code MUST be modernized incrementally — never as a big-bang rewrite.
Each change MUST be small enough to review, test, and revert
independently. When touching legacy code, apply modern practices
(e.g., safer memory handling, clearer naming) only to the code
being modified — do not refactor adjacent untouched code in the
same change. Comments in Czech are acceptable; new comments SHOULD
be in English.

**Rationale**: The codebase predates modern C++ practices. Attempting
wholesale modernization would introduce regressions and stall
feature delivery.

### IV. Windows Platform Commitment

Open Salamander is a pure WinAPI application. All features MUST
target Windows 11 and newer. No cross-platform abstraction layers
(Qt, wxWidgets, etc.) are permitted. Third-party dependencies MUST
be Windows-compatible and MUST NOT introduce licensing conflicts
with GPLv2. Visual Studio 2022 with the C++ Desktop workload is
the required toolchain.

**Rationale**: Salamander's value proposition is deep Windows
integration. Abstracting the platform would dilute this strength
and add complexity with no user benefit.

### V. Plugin Architecture Preservation

The plugin system is a core architectural element and MUST be
preserved and improved. Missing plugin dependencies (unrar.dll,
OpenSSL, PictView replacement) MUST be resolved using open-source
alternatives. New features SHOULD be implemented as plugins when
they represent self-contained functionality. Plugin interfaces
MUST be documented before modification.

**Rationale**: Plugins enable community contributions and keep the
core application lean. Resolving missing dependencies unblocks
features users expect from a file manager.

## Technical Constraints

- **Language**: C++ (MSVC compiler from VS2022)
- **Target OS**: Windows 11 and newer
- **SDK**: Windows 11 SDK (10.0.26100 or newer)
- **Encoding**: UTF-8-BOM for all source files
- **Formatting**: `clang-format` as configured in the repository
- **License**: GPLv2 or later; all dependencies MUST be
  GPLv2-compatible
- **Build output**: Controlled by `OPENSAL_BUILD_DIR` environment
  variable; build scripts MUST NOT hardcode absolute paths

## Development Workflow

- All changes MUST be submitted via pull requests against `main`.
- Each PR MUST address a single concern (one bug fix, one feature
  increment, or one build-system improvement).
- Build verification (`rebuild.cmd` or equivalent) MUST pass before
  merge.
- Code formatting MUST be verified via `normalize.ps1` or
  `clang-format` before merge.
- Commits MUST have descriptive messages in English referencing
  the issue or feature being addressed.

## Governance

This constitution is the authoritative source for project-wide
rules. It supersedes ad-hoc conventions and informal agreements.

**Amendment procedure**: Any contributor MAY propose an amendment
via a pull request modifying this file. Amendments MUST include
a rationale and MUST update the version number per semantic
versioning rules below.

**Versioning policy**:
- MAJOR: Principle removed or redefined in a backward-incompatible way
- MINOR: New principle or section added, or existing guidance
  materially expanded
- PATCH: Clarifications, typo fixes, non-semantic refinements

**Compliance review**: All PRs and code reviews SHOULD verify that
changes align with this constitution's principles. Violations
MUST be flagged and resolved before merge.

**Version**: 1.0.0 | **Ratified**: 2026-03-20 | **Last Amended**: 2026-03-20
