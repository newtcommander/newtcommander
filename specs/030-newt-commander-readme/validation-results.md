# Validation Results: Newt Commander README Rebrand

**Date**: 2026-07-22 | **Branch**: `030-newt-commander-readme` | **Quickstart**: [quickstart.md](quickstart.md)

## 1. Content check (SC-001, SC-004) — PASS

First screen answers all three identity questions (name: Newt Commander; base: Open
Salamander with upstream link; approach: agentic Spec-Driven Development via GitHub
SpecKit, best models of the era — currently Anthropic Fable 5). Branding note present.
Old README's historical sections (Origin narrative, Altap history, Resources list) absent.

## 2. Contract check — PASS (13/13 C-items, 4/4 N-items, 3/3 F-items)

- C-01…C-13: all verified present; C-08 build commands compared 1:1 against live
  `build.cmd help` output; C-12 license link uses the **corrected** path
  `doc/license/license_gpl.txt` (the old README linked non-existent `doc/license_gpl.txt`
  — broken link fixed by this feature; recorded in research.md R5 correction).
- N-01…N-04: grep sweep for `servant|forum|wikipedia|altap.cz|finesoftware|roadmap|changelog`
  → clean. Single "Altap Salamander" mention is derivation context (allowed). All three
  "Open Salamander" occurrences refer to the upstream project or app branding.
- F-01: first bytes `23 20 4E` — UTF-8 without BOM. F-02: `file` reports CRLF terminators.
  C-13: document is English throughout.

## 3. Link check (SC-003) — PASS

External (all HTTP 200 via curl -L): upstream repo, GitHub SpecKit, VS downloads,
git-scm.com, PowerShell install docs. Relative (all exist): `plugins.cfg`,
`doc/license/license_gpl.txt`, `doc/third_party.txt`, `AUTHORS`, `specs/`, `architecture/`.

## 4. Build-instruction check (SC-002, SC-005) — PASS (command surface); end-to-end run SKIPPED

- `build.cmd help` output matches every documented command, the order-independence claim,
  and the `OPENSAL_BUILD_DIR` default (`.\build\`) — the help text itself states
  "OPENSAL_BUILD_DIR env var (optional, defaults to .\build\)"; the default-note behavior
  is additionally confirmed in `build.cmd` source (lines 43–46).
- Optional end-to-end build deliberately skipped: this feature changes no build input, and
  the build system was verified clean (full Debug + Release x64) in feature 029, commit
  `91734a9`. A from-scratch full build into an empty `.\build\` would add tens of minutes
  with no informational gain for a documentation-only change.

## 5. Rendering — PASS (structural)

Document uses plain GitHub-flavored Markdown: headings, one table (pipe-aligned, valid
separator row), one fenced `batch` block, one blockquote. No exotic constructs; renders
cleanly in any Markdown viewer.

**Overall: PASS** — all 15 functional requirements satisfied; one pre-existing defect
(broken GPL license link in the old README) found and fixed along the way.
