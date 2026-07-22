# Data Model: Newt Commander README Rebrand

**Date**: 2026-07-22 | **Spec**: [spec.md](spec.md)

The feature's only artifact is one Markdown document. The "data model" is the content model
of that document: its required sections, the elements each must contain, and the links it
carries.

## Entity: README document

| Attribute | Value |
|-----------|-------|
| Path | `README.md` (repository root) |
| Format | GitHub-flavored Markdown |
| Language | English (FR-011) |
| Encoding / EOL | UTF-8 without BOM, CRLF (research R4) |
| Lifecycle | Full replacement of current content (no append) |

## Entity: Section (ordered composition of the document)

| # | Section | Required elements | Source FRs |
|---|---------|-------------------|------------|
| 1 | Title + identity | Name "Newt Commander"; two-panel Windows file manager; "based on Open Salamander" + upstream link | FR-001, FR-002 |
| 2 | Development approach | Agentic programming era; Spec-Driven Development; GitHub SpecKit link; agentic frameworks with best models of the era, currently Anthropic Fable 5 | FR-003, FR-004, FR-005 |
| 3 | Branding note | Rebrand is project/documentation-level; built app still shows Open Salamander name/branding | FR-015 |
| 4 | Building | Prerequisites (Win 11+, VS2022 + C++ workload, Windows SDK, optional `OPENSAL_BUILD_DIR` with default `.\build\`); `build.cmd` variants (default/`rebuild`/`release`/`full`, order-independent); `plugins.cfg` policy | FR-006, FR-007, FR-008 |
| 5 | Development process | `specs/` directory; SpecKit workflow (specify → clarify → plan → tasks → implement) | FR-012 |
| 6 | Repository structure | Brief top-level directory table (src, specs, architecture, convert, doc, help, tools, translations) | FR-013 |
| 7 | License | GPLv2 (or later) statement; links to `doc/license_gpl.txt` and `doc/third_party.txt` | FR-010 |

**Prohibited content** (validation rules): upstream historical narrative, Altap history,
upstream resource/link lists (FR-009); project status or roadmap section (FR-014);
"Open Salamander" used as the name of *this* project (FR-001).

## Entity: Link

| Link | Target | Kind | Required by |
|------|--------|------|-------------|
| Upstream project | `https://github.com/OpenSalamander/salamander` | external | FR-002, FR-009 |
| GitHub SpecKit | `https://github.com/github/spec-kit` | external | FR-004 |
| GPL license | `doc/license_gpl.txt` | relative | FR-010 |
| Third-party notices | `doc/third_party.txt` | relative | FR-010 |
| Contributors | `AUTHORS` | relative | optional (identity section) |

Validation rule: 100 % of links resolve (SC-003); external links limited to the two above
(research R5).

## State transitions

None — the document has no runtime state. The only transition is the one-time replacement
old README → new README on branch `030-newt-commander-readme`.
