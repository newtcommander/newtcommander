# Phase 0 Research: Newt Commander README Rebrand

**Date**: 2026-07-22 | **Spec**: [spec.md](spec.md)

No `NEEDS CLARIFICATION` markers remained in the Technical Context; research therefore
focused on verifying every fact the new README will assert, so the document cannot drift
from reality (SC-005: every documented command must behave as described).

## R1. Build instructions — verified against `build.cmd`

**Decision**: Document the repository-root `build.cmd` as the only primary build path, with
exactly these facts (all verified by reading the script, 2026-07-22):

- Usage: `build.cmd [rebuild] [release] [full]` (+ `help`); arguments are **order-independent**.
- No args → incremental **Debug x64** build.
- `rebuild` → full clean + rebuild; `release` → Release x64; combinable (`build.cmd rebuild release`).
- `full` → complete build: app + all enabled plugins + language modules + runtime data
  (conversion tables, toolbars, sample scripts) + generated `plugins\plugins.ver` so plugins
  auto-register in Plugin Manager.
- `OPENSAL_BUILD_DIR` is **optional**: when unset, the script defaults to `.\build\` under the
  repository root and prints a note. When set, the value needs a trailing backslash
  (e.g. `D:\Build\OpenSal\`).
- VS2022 is located via `vswhere.exe` requiring version `[17.0,18.0)` and component
  `Microsoft.VisualStudio.Component.VC.Tools.x86.x64` — i.e. the "Desktop development with C++"
  workload; a clear error is printed when missing.
- The plugin set is governed by `plugins.cfg` in the repository root (`name=on|off`); every run
  validates the file, builds only enabled plugins, and removes outputs of disabled ones.

**Rationale**: FR-006/FR-007/FR-008 and SC-005 require the README to match actual script
behavior. The old README documents `src\vcxproj\rebuild.cmd`, which is now a secondary path.

**Alternatives considered**: Documenting both `build.cmd` and the `src\vcxproj` scripts in
equal depth — rejected (spec assumption: root `build.cmd` is the primary path; mentioning
alternatives at most in passing keeps the README lean).

## R2. Prerequisites list

**Decision**: Windows 11 or newer; Visual Studio 2022 (any edition) with the
"Desktop development with C++" workload; a Windows 10/11 SDK (projects target `10.0` = latest
installed). Optional: `OPENSAL_BUILD_DIR` (default `.\build\`), Git, PowerShell 7.4+
(only for `normalize.ps1` source formatting).

**Rationale**: Matches `CLAUDE.md` "Prerequisites" and the checks `build.cmd` actually performs.
The old README pinned a specific SDK build (10.0.26100.4654); the projects actually use `10.0`
(latest installed SDK), so the README should not over-pin.

**Alternatives considered**: Copying the old README's pinned SDK version — rejected as
unnecessarily strict and a future maintenance burden.

## R3. Document structure and section order

**Decision**: Title (Newt Commander) → one-paragraph identity (two-panel Windows file manager,
derived from Open Salamander, link) → "A New Era of Development" (agentic programming,
Spec-Driven Development, GitHub SpecKit link, agentic frameworks + best models of the era,
currently Anthropic Fable 5) → branding note (FR-015: built app still says Open Salamander)
→ Building (prerequisites, `build.cmd` variants, `plugins.cfg`) → Development Process
(`specs/` directory, SpecKit workflow stages) → Repository Structure (brief top-level table)
→ License (GPLv2 + third-party notices).

**Rationale**: SC-001 requires name/heritage/approach on the first screen, so identity and
the development-approach story come first. Clarification session decided: include workflow +
repo structure sections, exclude status/roadmap (FR-014).

**Alternatives considered**: Mirroring the old README's section order (Origin → Development →
Repository Content → Resources → License) — rejected; the origin narrative is exactly the
content FR-009 says not to duplicate.

## R4. Encoding and line endings

**Decision**: Keep the file UTF-8 **without** BOM with CRLF line endings.

**Rationale**: Measured the current `README.md`: first bytes `23 20 4F` (no BOM), CRLF
terminators. `normalize_config.json` includes no `*.md` pattern, so `normalize.ps1` imposes
nothing on Markdown; matching the existing file avoids a spurious whole-file diff dimension
and keeps GitHub rendering unaffected.

**Alternatives considered**: UTF-8-BOM (the repo convention for *source* files) — rejected;
the constitution's encoding constraint targets source files processed by the normalization
pipeline, and the present README already uses no BOM.

## R5. External links policy

**Decision**: Exactly these external links: upstream repository
`https://github.com/OpenSalamander/salamander` (FR-002) and GitHub SpecKit
`https://github.com/github/spec-kit` (FR-004). In-repo relative links for license files
(`doc/license_gpl.txt`, `doc/third_party.txt`), `AUTHORS`, and `plugins.cfg` as needed.

**Rationale**: Edge case in spec — small link-rot surface; upstream link doubles as the
pointer for all historical information (FR-009), replacing the old Resources section.

**Correction (T002 fact check, 2026-07-22)**: the GPL license file lives at
`doc/license/license_gpl.txt`, not `doc/license_gpl.txt` as the old README (and the plan's
file list) claimed — the old README's license link was broken. The new README links the
correct path.

**Alternatives considered**: Retaining the old Resources list (Altap website, forum,
Wikipedia) — rejected per FR-009; those belong to the upstream project's story.

## R6. Wording of the model claim

**Decision**: Phrase as "the best models available at the time — currently Anthropic Fable 5",
so the sentence stays truthful as models advance.

**Rationale**: Spec edge case: the claim is time-sensitive by nature and must age gracefully.

**Alternatives considered**: Naming only "Anthropic Fable 5" without qualification — rejected;
reads as false the moment a newer model is adopted.
