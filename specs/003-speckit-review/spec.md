# Feature Specification: SpecKit & AI Development Review

**Feature Branch**: `003-speckit-review`
**Created**: 2026-04-12
**Status**: Draft
**Input**: User description: "Analyzuj současný projekt, především návaznosti na AI vývoj pomocí Claude Code a SpecKit. Zaměř se na adresáře .specify, .claude, architecture, specs, ale i na celý zdroj. V rámci této etapy provedeme revizi dosavadní práce a především verifikaci platnosti dat a souborů po přechodu na novou verzi GitHub SpecKit, tak abychom mohli projekt dále vyvíjet."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Developer Verifies SpecKit Integration After Version Transition (Priority: P1)

A developer (or AI assistant) opens the Open Salamander project after
SpecKit has been updated to version 0.6.1 (installed from GitHub).
They need to confirm that all SpecKit configuration files, templates,
skills, extensions, and scripts are consistent and functional so that
the development workflow (`/speckit.specify` -> `/speckit.plan` ->
`/speckit.tasks` -> `/speckit.implement`) works correctly for future
features.

**Why this priority**: If the SpecKit infrastructure is broken or
inconsistent after the version transition, no further AI-assisted
development can proceed. This is a blocking prerequisite.

**Independent Test**: Run each SpecKit skill command in sequence on a
test feature and verify that all commands complete without errors,
produce correct output files, and respect the project constitution.

**Acceptance Scenarios**:

1. **Given** the SpecKit installation at version 0.6.1, **When** a
   reviewer checks `.specify/init-options.json`, `.specify/integration.json`,
   and `.specify/extensions.yml`, **Then** all configuration files are
   internally consistent (matching versions, valid references, correct
   integration type).
2. **Given** the Claude Code skills in `.claude/skills/`, **When** each
   skill file is compared against the manifest in
   `.specify/integrations/claude.manifest.json`, **Then** all skill
   files exist at their declared paths with matching hashes.
3. **Given** the SpecKit templates in `.specify/templates/`, **When**
   each template is compared against the manifest in
   `.specify/integrations/speckit.manifest.json`, **Then** all template
   files exist at their declared paths with matching hashes.
4. **Given** the git extension in `.specify/extensions/git/`, **When**
   the extension registry, config, and hook definitions are reviewed,
   **Then** all registered commands have corresponding skill files and
   script implementations for both bash and powershell.

---

### User Story 2 - Developer Reviews Existing Specifications and Architecture (Priority: P2)

A developer reviews the two completed specifications (001-project-build-analysis,
002-msvc-x64-build-script) and the architecture documentation to verify
that they accurately reflect the current state of the project and are
still valid references for future work.

**Why this priority**: Stale or inaccurate specifications and architecture
documents would mislead AI assistants and developers, leading to incorrect
decisions in future development phases.

**Independent Test**: Compare key facts in each document (project counts,
dependency lists, build instructions, compiler versions) against the actual
codebase state and verify accuracy.

**Acceptance Scenarios**:

1. **Given** the architecture documents in `architecture/`, **When** the
   reviewer checks project counts and structure against `salamand.sln`,
   **Then** the documented 90 projects match the actual solution file.
2. **Given** `specs/001-project-build-analysis/`, **When** the spec status
   and deliverables are reviewed, **Then** all referenced architecture
   documents exist and cover the topics described in the spec.
3. **Given** `specs/002-msvc-x64-build-script/`, **When** the build script
   deliverable is checked, **Then** `build.cmd` exists at the repo root
   and functions as described in the spec.
4. **Given** `CLAUDE.md` at the project root, **When** its content is
   reviewed, **Then** it accurately references the architecture directory,
   current project phase, and all key facts about the codebase.

---

### User Story 3 - Developer Identifies Gaps and Next Steps (Priority: P3)

After completing the verification, a developer wants a clear summary of
what is valid, what needs updating, and what the recommended next
development steps are. This ensures continuity and prevents redundant
work.

**Why this priority**: Knowing the current state is valuable only if it
leads to actionable next steps. This story ensures the review produces
a concrete output that guides future development.

**Independent Test**: The review produces a written summary that lists
all verified items, any discrepancies found, and recommended actions,
which can be used as input for the next `/speckit.specify` invocation.

**Acceptance Scenarios**:

1. **Given** the completed review, **When** the summary is produced,
   **Then** it categorizes all reviewed items as "valid", "needs update",
   or "missing".
2. **Given** identified discrepancies, **When** the summary lists them,
   **Then** each discrepancy includes the specific file, the issue, and
   a suggested resolution.

---

### Edge Cases

- What happens if manifest hashes don't match actual file hashes? This
  indicates files were modified outside SpecKit and need re-registration.
- What happens if the SpecKit version in `init-options.json` doesn't match
  the version in `integration.json`? The newer version takes precedence
  and the older file should be updated.
- What happens if an architecture document references a file or path that
  no longer exists in the repository?
- How does the review handle the existing `feature.json` state that was
  overwritten when this spec was created?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Review MUST verify internal consistency of all SpecKit
  configuration files (`.specify/init-options.json`, `.specify/integration.json`,
  `.specify/extensions.yml`, `.specify/extensions/.registry`).
- **FR-002**: Review MUST verify that all Claude Code skill files listed
  in `.specify/integrations/claude.manifest.json` exist at their declared
  paths.
- **FR-003**: Review MUST verify that all SpecKit template and script
  files listed in `.specify/integrations/speckit.manifest.json` exist at
  their declared paths.
- **FR-004**: Review MUST verify that the git extension's registered
  commands each have both a command definition (`.md`) and script
  implementations (bash + powershell).
- **FR-005**: Review MUST verify that existing specifications
  (`specs/001-*`, `specs/002-*`) reference deliverables that actually
  exist in the repository.
- **FR-006**: Review MUST verify that all 8 architecture documents exist
  and are linked from `architecture/README.md`.
- **FR-007**: Review MUST verify that `CLAUDE.md` is current and
  accurately describes the project state, build instructions, and
  architecture references.
- **FR-008**: Review MUST verify that `build.cmd` exists at the
  repository root and is consistent with the spec 002 description.
- **FR-009**: Review MUST produce a structured summary categorizing
  each reviewed item as valid, needs-update, or missing.
- **FR-010**: Review MUST identify any files in `.specify/`, `.claude/`,
  `specs/`, or `architecture/` that are orphaned (not referenced by any
  configuration or index).

### Key Entities

- **SpecKit Configuration**: The set of JSON/YAML files controlling
  SpecKit behavior — includes version, integration type, branch numbering,
  hook definitions, and extension registry.
- **Skill File**: A Claude Code skill definition (`.claude/skills/*/SKILL.md`)
  — registered in the manifest and invoked by SpecKit commands.
- **Specification**: A feature directory under `specs/` containing spec.md
  and related artifacts — represents a completed or in-progress feature
  definition.
- **Architecture Document**: A markdown file in `architecture/` — documents
  a specific aspect of the project's technical foundation.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% of files listed in both manifests (`claude.manifest.json`
  and `speckit.manifest.json`) are confirmed to exist at their declared paths.
- **SC-002**: All SpecKit configuration files use consistent version
  numbers and valid cross-references.
- **SC-003**: All 8 architecture documents are confirmed present and
  linked from the architecture README.
- **SC-004**: The review summary covers every item in the `.specify/`,
  `.claude/`, `specs/`, and `architecture/` directories with a clear
  validity status.
- **SC-005**: Any identified discrepancies have specific, actionable
  resolution steps documented.
- **SC-006**: After the review, a developer or AI assistant can confidently
  start the next feature specification knowing the infrastructure is verified.

## Assumptions

- The SpecKit version 0.6.1 installed from GitHub is the target baseline
  for verification — earlier version artifacts that are incompatible should
  be flagged.
- The project is on the `aidevel/main` branch, which is the active
  development branch. Verification is against this branch's state, not
  `main`.
- The `build.cmd` at the repository root is the deliverable from spec 002
  and should be functional for Debug x64 builds.
- File hash verification is informational — mismatched hashes indicate
  local modifications but do not necessarily indicate errors.
- The existing two specifications (001, 002) represent completed Phase 1
  work and should be treated as reference material, not active specs.
