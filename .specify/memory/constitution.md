<!--
Sync Impact Report
===================
Version change: 2.0.0 → 3.0.0 (project renamed to Tandem Commander; principle II
redefined in a backward-incompatible way — the product identity anchor moved from
Newt Commander 0.1.0 to Tandem Commander 0.1.0 per feature 046)
Modified principles:
  - II. Backward Compatibility — the identity (binary name, registry root,
    inter-process object names, shell-extension IPC identity) is now Tandem
    Commander's; the Newt → Tandem rename is recorded as the second deliberate,
    documented, one-time identity change (feature 046), with no configuration
    migration; the plugin ABI (interface version 105) carried over unchanged
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
  - 1.1.0 → 2.0.0: project rebranded to Newt Commander; principle II
    re-anchored from Open Salamander 5.0 to Newt Commander 0.1.0 (feature 032)
-->

# Tandem Commander Constitution

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

The compatibility baseline is **Tandem Commander 0.1.0**. Existing
Tandem Commander functionality MUST NOT regress unless explicitly
deprecated with documented justification. User-facing behavior
changes MUST be opt-in or gated behind version checks. Plugin API
changes MUST maintain binary compatibility or provide a clear
migration path with a deprecation cycle of at least one minor
release.

Tandem Commander is deliberately NOT compatible with its
predecessors' product identities: it uses its own binary name
(`tandemcommander.exe`), registry root (`HKCU\Software\Tandem
Commander`), inter-process object names, and shell-extension
identity, and it never reads or writes Open Salamander or Newt
Commander configuration. Identity changes are wholesale, one-time,
documented decisions and MUST NOT be reintroduced piecemeal: the
break with Open Salamander 5.0 was made in feature 032
(`specs/032-newt-commander-rebrand/`), and the Newt Commander →
Tandem Commander rename in feature 046
(`specs/046-tandem-commander-rebrand/`). The plugin ABI (interface
version 105) was carried over unchanged both times.

**Rationale**: From version 0.1.0 on, all modifications,
extensions, and new functionality are developed as the open-source
application Tandem Commander. Trust is maintained by preserving the
behavior Tandem Commander users depend on; identity separation
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

Tandem Commander is a pure WinAPI application. All features MUST
target Windows 11 and newer. No cross-platform abstraction layers
(Qt, wxWidgets, etc.) are permitted. Third-party dependencies MUST
be Windows-compatible and MUST NOT introduce licensing conflicts
with GPLv2. Visual Studio 2022 with the C++ Desktop workload is
the required toolchain.

**Rationale**: Tandem Commander's value proposition is deep Windows
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

**Version**: 3.0.0 | **Ratified**: 2026-03-20 | **Last Amended**: 2026-08-01
