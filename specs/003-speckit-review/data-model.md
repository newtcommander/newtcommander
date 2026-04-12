# Data Model: SpecKit & AI Development Review

**Branch**: `003-speckit-review` | **Date**: 2026-04-12

## Entities

This feature is an audit/review — it does not introduce new data entities
or modify existing ones. The entities below describe the existing SpecKit
data structures that were verified during the review.

### SpecKit Configuration

The central configuration state consisting of:

| File | Purpose | Key Fields |
|------|---------|------------|
| `init-options.json` | Installation options | ai, integration, branch_numbering, speckit_version |
| `integration.json` | Active integration config | integration, version, scripts |
| `extensions.yml` | Hook registry | installed, settings, hooks (by lifecycle stage) |
| `extensions/.registry` | Extension catalog | schema_version, extensions (by name) |
| `feature.json` | Active feature pointer | feature_directory |

**Relationships**: `integration.json` references the same version as `init-options.json`.
Extension commands in `.registry` must match hook references in `extensions.yml`.

### Manifest

Tracks installed files with integrity hashes:

| Manifest | Tracks | File Count |
|----------|--------|------------|
| `claude.manifest.json` | Claude Code skills + integration scripts | 11 files |
| `speckit.manifest.json` | Templates + core scripts | 11 files |

**Relationships**: Each manifest entry maps a file path to a SHA-256 hash.
All referenced files must exist on disk.

### Specification Feature

A feature directory under `specs/` containing workflow artifacts:

| Artifact | Created By | Purpose |
|----------|-----------|---------|
| `spec.md` | `/speckit.specify` | Feature specification |
| `plan.md` | `/speckit.plan` | Implementation plan |
| `research.md` | `/speckit.plan` | Research findings |
| `data-model.md` | `/speckit.plan` | Entity definitions |
| `quickstart.md` | `/speckit.plan` | Getting started guide |
| `tasks.md` | `/speckit.tasks` | Implementation tasks |
| `checklists/requirements.md` | `/speckit.specify` | Quality checklist |

**Relationships**: Each spec directory is independent. `feature.json` points
to the currently active one.

### Architecture Document

A markdown file in `architecture/` indexed by `architecture/README.md`:

| Document | Topic |
|----------|-------|
| 01 | Project overview |
| 02 | Solution structure (90 projects) |
| 03 | Build pipeline |
| 04 | Dependencies |
| 05 | Compiler comparison |
| 06 | Plugin architecture |
| 07 | Preprocessor definitions |
| 08 | Code standards |

**Relationships**: Referenced from `CLAUDE.md` and from spec 001.

## State Transitions

This review feature has no state transitions — it is a one-time verification
that produces a static report. The existing SpecKit workflow states
(Draft -> Planned -> Tasked -> Implementing -> Complete) apply to feature
specs but not to this audit.

## Validation Rules

- All manifest file paths must resolve to existing files
- Version fields across config files must be consistent
- Every hook command must have a registered extension command
- Every registered command must have a skill file and script implementation
- Every architecture document must be linked from README.md
