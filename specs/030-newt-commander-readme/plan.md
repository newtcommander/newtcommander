# Implementation Plan: Newt Commander README Rebrand

**Branch**: `030-newt-commander-readme` | **Date**: 2026-07-22 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/030-newt-commander-readme/spec.md`

## Summary

Replace the upstream-inherited `README.md` with a new front page that presents the project as **Newt Commander**: a derivative of Open Salamander developed in the era of agentic programming via Spec-Driven Development (GitHub SpecKit + agentic frameworks with the best models of the day, currently Anthropic Fable 5). The new document covers identity and heritage, an honest note that the built application still carries Open Salamander branding, build instructions centered on the repository-root `build.cmd` (verified against the actual script behavior), the development process (`specs/` workflow), a brief repository structure overview, and the GPLv2 license statement. Historical/upstream content is not duplicated — readers follow the upstream link.

## Technical Context

**Language/Version**: Markdown (GitHub-flavored), English text
**Primary Dependencies**: None (single documentation file); facts sourced from `build.cmd`, `plugins.cfg`, `CLAUDE.md`, `architecture/` docs
**Storage**: N/A (file in repository root: `README.md`)
**Testing**: Manual verification — GitHub rendering preview, link resolution, cross-check of every documented command against `build.cmd` behavior
**Target Platform**: GitHub repository front page (renders in any Markdown viewer)
**Project Type**: Documentation-only change (single file replacement)
**Performance Goals**: N/A
**Constraints**: UTF-8 without BOM + CRLF line endings (matches current `README.md`; `normalize.ps1` does not process `*.md`); no duplication of upstream historical content; external links limited to essentials (upstream repo, GitHub SpecKit)
**Scale/Scope**: One file (~60–90 lines); 15 functional requirements (FR-001…FR-015)

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Applies? | Assessment |
|-----------|----------|------------|
| I. Build Reproducibility | Indirectly | PASS — README documents the existing single-command build (`build.cmd`) exactly as implemented; no build changes. Documenting the `OPENSAL_BUILD_DIR` default supports this principle. |
| II. Backward Compatibility | Indirectly | PASS — no functional change; README explicitly acknowledges the application itself is unchanged (still Open Salamander branding, FR-015), so no user-facing behavior is misrepresented. |
| III. Incremental Modernization | Yes | PASS — a single, small, independently revertable documentation change. |
| IV. Windows Platform Commitment | Indirectly | PASS — README states Windows 11+/VS2022 prerequisites, consistent with the constitution's toolchain requirements. |
| V. Plugin Architecture Preservation | Indirectly | PASS — README mentions `plugins.cfg` plugin policy (FR-008); no plugin changes. |
| VI. UI Consistency | No | N/A — no UI change. |
| Technical Constraints (encoding/formatting) | Yes | PASS — `*.md` is outside `normalize.ps1` scope (`normalize_config.json` textfiles includes contain no `*.md`); the file keeps its current UTF-8 (no BOM) + CRLF convention. License statement retained (GPLv2, FR-010). |
| Development Workflow | Yes | PASS — change goes through the feature branch `030-newt-commander-readme`; build verification is not applicable (no code), formatting check not applicable (`*.md` not covered). |

**Gate result (pre-Phase 0)**: PASS — no violations, Complexity Tracking not needed.
**Gate result (post-Phase 1 re-check)**: PASS — design artifacts introduce no new tooling, dependencies, or code changes.

## Project Structure

### Documentation (this feature)

```text
specs/030-newt-commander-readme/
├── spec.md              # Feature specification (complete, clarified)
├── plan.md              # This file
├── research.md          # Phase 0 output — verified facts and content decisions
├── data-model.md        # Phase 1 output — README content model (sections, links)
├── quickstart.md        # Phase 1 output — how to verify the new README
├── contracts/
│   └── readme-content-contract.md   # FR → required content element mapping
├── checklists/
│   └── requirements.md  # Spec quality checklist (passed)
└── tasks.md             # Phase 2 output (/speckit.tasks — NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
README.md                # The only file modified by this feature (full replacement)
```

Facts are read from (not modified): `build.cmd`, `plugins.cfg`, `CLAUDE.md`,
`architecture/01-project-overview.md`, `doc/license_gpl.txt`, `doc/third_party.txt`, `AUTHORS`.

**Structure Decision**: Single-file documentation change at the repository root. No source, build, or test tree is touched.

## Complexity Tracking

Not applicable — Constitution Check passed with no violations.
