<!--
Sync Impact Report
===================
Version change: 1.1.0 → 2.0.0 (project rebranded to Newt Commander; principle II
redefined in a backward-incompatible way — compatibility baseline re-anchored from
Open Salamander 5.0 to Newt Commander 0.1.0 per feature 032)
Modified principles:
  - II. Backward Compatibility — baseline is now Newt Commander 0.1.0; the break
    with Open Salamander 5.0 (registry, IPC names, shell-extension identity,
    binary name) is recorded as a deliberate, documented, one-time decision
Added sections: N/A
Removed sections: N/A
Templates requiring updates:
  - .specify/templates/plan-template.md — ✅ no update needed (generic Constitution Check gate)
  - .specify/templates/spec-template.md — ✅ no update needed (generic structure)
  - .specify/templates/tasks-template.md — ✅ no update needed (generic structure)
Follow-up TODOs: none

Prior history:
  - 0.0.0 → 1.0.0 (initial adoption): Core Principles (5), Technical
    Constraints, Development Workflow, Governance
  - 1.0.0 → 1.1.0: added principle VI. UI Consistency
-->

# Newt Commander Constitution

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

The compatibility baseline is **Newt Commander 0.1.0**. Existing
Newt Commander functionality MUST NOT regress unless explicitly
deprecated with documented justification. User-facing behavior
changes MUST be opt-in or gated behind version checks. Plugin API
changes MUST maintain binary compatibility or provide a clear
migration path with a deprecation cycle of at least one minor
release.

Newt Commander is deliberately NOT compatible with the upstream
Open Salamander product identity: it uses its own binary name
(`newtcommander.exe`), registry root (`HKCU\Software\Newt
Commander`), inter-process object names, and shell-extension
identity, and it never reads or writes Open Salamander
configuration. This break with Open Salamander 5.0 was made once,
by design, in feature 032 (`specs/032-newt-commander-rebrand/`)
and MUST NOT be reintroduced piecemeal. The plugin ABI (version
104) was carried over unchanged.

**Rationale**: From version 0.1.0 on, all modifications,
extensions, and new functionality are developed as the open-source
application Newt Commander. Trust is maintained by preserving the
behavior Newt Commander users depend on; identity separation
protects both products' users from cross-corruption of settings.

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

Newt Commander is a pure WinAPI application. All features MUST
target Windows 11 and newer. No cross-platform abstraction layers
(Qt, wxWidgets, etc.) are permitted. Third-party dependencies MUST
be Windows-compatible and MUST NOT introduce licensing conflicts
with GPLv2. Visual Studio 2022 with the C++ Desktop workload is
the required toolchain.

**Rationale**: Newt Commander's value proposition is deep Windows
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

### VI. UI Consistency

New dialogs and controls MUST match the application's established
house style so the whole product looks like one program. Dialog
templates MUST be `DIALOGEX` declared with `DS_SHELLFONT`
(`DS_SETFONT | DS_FIXEDSYS`) and `FONT 8, "MS Shell Dlg"`, and MUST
use the standard themed WinAPI controls. A module (plugin or the
core) MUST NOT locally alter process-wide visual behavior — in
particular it MUST NOT pass `ICC_STANDARD_CLASSES` to
`InitCommonControlsEx`, embed its own manifest, or subclass/owner-draw
standard edit controls purely to restyle them. Application-wide visual
changes (fonts, control decoration, dark mode, DPI-awareness model)
MUST be a deliberate, versioned decision applied across the whole
application — never a side effect of one plugin.

**Rationale**: Visual uniformity across all dialogs builds user trust
and readability. Because comctl32 v6 registers its forked standard
window classes (Edit, Button, …) process-wide the moment any module
requests `ICC_STANDARD_CLASSES`, a single plugin's local choice
silently changes control rendering everywhere else — the exact defect
that made the SFTP plugin's edit boxes render in the modern Windows 11
style while the rest of the app stayed classic.

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

**Version**: 1.1.0 | **Ratified**: 2026-03-20 | **Last Amended**: 2026-07-17
