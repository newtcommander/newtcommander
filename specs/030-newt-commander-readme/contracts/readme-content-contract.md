# Content Contract: README.md

**Date**: 2026-07-22 | **Spec**: [../spec.md](../spec.md)

The README's "interface" is what it promises its readers. Each row is a checkable contract
item; the new document satisfies this feature only if every MUST row passes and every
MUST NOT row finds no match.

## MUST contain

| ID | Contract item | Verification |
|----|---------------|--------------|
| C-01 | Title names the project "Newt Commander" | First `#` heading contains "Newt Commander" (FR-001) |
| C-02 | Statement of derivation from Open Salamander with link | Text links `https://github.com/OpenSalamander/salamander` in the opening section (FR-002) |
| C-03 | Agentic programming / Spec-Driven Development description | Opening or dedicated section names both concepts (FR-003) |
| C-04 | GitHub SpecKit reference with link | Text links `https://github.com/github/spec-kit` (FR-004) |
| C-05 | Agentic frameworks + best-models-of-the-era claim, currently Anthropic Fable 5 | Sentence present, phrased to age gracefully (FR-005, research R6) |
| C-06 | Branding note: built app still shows Open Salamander | Explicit sentence near identity or build section (FR-015) |
| C-07 | Prerequisites: Windows 11+, VS2022 + "Desktop development with C++", Windows SDK, optional `OPENSAL_BUILD_DIR` incl. default `.\build\` | All listed before build commands (FR-007, US2/AS1) |
| C-08 | Build commands: `build.cmd`, `build.cmd rebuild`, `build.cmd full`, `build.cmd full release` with one-line meanings | Commands match `build.cmd` actual behavior (FR-006, SC-005) |
| C-09 | `plugins.cfg` governs which plugins are built | Mentioned in build section (FR-008) |
| C-10 | Development process section: `specs/` directory + SpecKit workflow | Section present (FR-012) |
| C-11 | Brief repository structure overview | Top-level directories with one-line purposes (FR-013) |
| C-12 | License statement: GPLv2 (or later) + third-party notices, with in-repo links | Links `doc/license_gpl.txt`, `doc/third_party.txt` (FR-010) |
| C-13 | Document is in English | Whole document (FR-011) |

## MUST NOT contain

| ID | Contract item | Verification |
|----|---------------|--------------|
| N-01 | Upstream historical narrative (Servant/Altap story, name origin, company history) | No such paragraphs; grep for "Servant", "Altap", "Fine" finds at most the derivation/license context, not narrative (FR-009) |
| N-02 | Upstream resource list (Altap website, forum, changelogs, Wikipedia) | No Resources section with those links (FR-009) |
| N-03 | Project status / roadmap section | No such heading or list (FR-014) |
| N-04 | "Open Salamander" used as this project's name | Every occurrence refers to the upstream project (FR-001) |

## File-level contract

| ID | Contract item | Verification |
|----|---------------|--------------|
| F-01 | Encoding UTF-8 without BOM | First bytes are not `EF BB BF` (research R4) |
| F-02 | CRLF line endings | File uses CRLF consistently (research R4) |
| F-03 | All links resolve | External: HTTP 200 targets; relative: files exist in repo (SC-003) |
