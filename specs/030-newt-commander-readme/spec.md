# Feature Specification: Newt Commander README Rebrand

**Feature Branch**: `030-newt-commander-readme`
**Created**: 2026-07-22
**Status**: Draft
**Input**: User description: "Uprav Soubor README.md, změň název projektu na 'Newt Commander' a popiš do něj, že se jedná o projekt, který vychází z open source projektu Open Salamander (https://github.com/OpenSalamander/salamander), ale že se jedná o verzi vyvíjenou v nové éře agentického programování pomocí principů Spec-Driven Development. Uveď, že princip vývoje je postaven na GitHub SpecKit (https://github.com/github/spec-kit) a využívá kombinaci různých agentních frameworků s nejlepšími dostupnými modely pro danou dobu (aktuálně Anthropic Fable 5). Dále do dokumentu popiš další detaily ohledně projektu, především pak jak projekt sestavit pomocí build.cmd a co je k tomu potřeba. Není nutné v tomto novém README opakovat původní informace ze starého README, to si lidé mohou přečíst kliknutím na odkaz Open Salamander."

## Clarifications

### Session 2026-07-22

- Q: Which additional sections should the new README include beyond identity, build instructions, and license? → A: Also a development-process section (SpecKit workflow, `specs/` directory, agentic development) and a brief repository structure overview; no project status/roadmap section.
- Q: Should the README explicitly acknowledge that the built application still carries the Open Salamander name and branding? → A: Yes — a brief note that the rebrand is currently at the project/documentation level.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Visitor Understands Project Identity (Priority: P1)

A visitor lands on the repository page (typically on GitHub, where README.md renders as the project front page). Within the first screen of text they learn: the project is called **Newt Commander**, it is a Windows two-panel file manager derived from the open-source **Open Salamander** project, and its distinguishing trait is that it is developed in the new era of agentic programming using **Spec-Driven Development** principles — built on **GitHub SpecKit** and a combination of agentic frameworks with the best available models of the day (currently Anthropic Fable 5).

**Why this priority**: The README is the single most-read document of the repository. Establishing the new identity and the "why this fork exists" story is the core purpose of this feature; without it, nothing else matters.

**Independent Test**: Open README.md and verify — without consulting any other file — that a first-time reader can answer: (1) What is the project's name? (2) What is it derived from? (3) How is it developed differently from the original?

**Acceptance Scenarios**:

1. **Given** the repository front page, **When** a visitor reads the README title and opening section, **Then** the project presents itself as "Newt Commander" (the name "Open Salamander" appears only as the referenced upstream project, not as this project's name).
2. **Given** the opening section, **When** the visitor looks for the project origin, **Then** they find an explicit statement that the project is based on Open Salamander with a working link to https://github.com/OpenSalamander/salamander.
3. **Given** the opening section, **When** the visitor looks for the development approach, **Then** they find a description of agentic, Spec-Driven Development with a working link to https://github.com/github/spec-kit and a mention that development combines various agentic frameworks with the best models available at the time (currently Anthropic Fable 5).

---

### User Story 2 - Developer Builds the Project (Priority: P2)

A developer clones the repository and wants to build a runnable Newt Commander. Following only the README, they learn what to install first (Windows 11 or newer, Visual Studio 2022 with the C++ desktop workload and a Windows SDK), what to optionally configure (the `OPENSAL_BUILD_DIR` environment variable), and how to run `build.cmd` from the repository root, including its main variants (incremental Debug build, `rebuild`, `full`, `full release`) and the role of `plugins.cfg` in deciding which plugins are built.

**Why this priority**: Building the project is the most common practical task a reader performs after understanding what the project is. The old README documents an outdated build path (`src\vcxproj\rebuild.cmd`); the new one must document the current supported entry point `build.cmd`.

**Independent Test**: On a machine with the listed prerequisites, follow the README build section verbatim and confirm the build completes and produces a runnable application.

**Acceptance Scenarios**:

1. **Given** a reader with none of the tooling installed, **When** they read the build section, **Then** all prerequisites are listed before the build commands, so they can prepare the machine in one pass.
2. **Given** a prepared machine, **When** the reader follows the documented `build.cmd` invocation(s), **Then** each documented command matches the actual behavior of the script in the repository root (no references to superseded scripts as the primary path).
3. **Given** a reader who has not set `OPENSAL_BUILD_DIR`, **When** they run the documented default build, **Then** the README has told them what happens (the documented default build directory is used) — no undocumented failure.

---

### User Story 3 - Reader Finds Original Project Information (Priority: P3)

A reader interested in the history, features, or documentation of the original file manager (Servant/Altap/Open Salamander story, forums, changelogs) is not served duplicated content in the Newt Commander README; instead the README points them to the upstream Open Salamander project, where that information lives.

**Why this priority**: Keeping the README lean is an explicit goal — duplicated upstream content would rot and dilute the new identity. Still, this is a navigation convenience, not the document's core message.

**Independent Test**: Search the new README for the old README's historical sections (origin story, Altap history, upstream resource lists) and confirm they are absent, while the upstream link is present and sufficient to reach that information.

**Acceptance Scenarios**:

1. **Given** the new README, **When** a reader looks for the original project's history, **Then** they find no duplicated historical narrative but do find the link to the Open Salamander repository where it is available.

---

### Edge Cases

- A reader lands on the README from a search for "Open Salamander": the README must still make the relationship clear (fork/derivative), so they know they found a related — but distinct — project.
- `OPENSAL_BUILD_DIR` not set: the README must state the default behavior so the first build does not surprise the reader.
- External links (upstream repository, GitHub SpecKit) may change over time: the README should limit external links to the few that are essential, so link rot surface stays small.
- The claim "currently Anthropic Fable 5" is time-sensitive by nature: the wording must make clear it reflects the best model *at the time* ("for a given era"), so the statement does not read as false when models advance.
- Legal continuity: the project is a GPLv2 derivative; removing all licensing information from the README would misrepresent the terms under which the code is offered, so a license statement must remain.
- A developer builds the project and the resulting application identifies itself as "Open Salamander": the README must have set this expectation in advance (see FR-015), so the mismatch does not read as a broken build.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The README MUST present the project name as "Newt Commander" in its title and introductory text; the name "Open Salamander" MUST appear only when referring to the upstream project.
- **FR-002**: The README MUST state that the project is based on the open-source Open Salamander project and MUST link to https://github.com/OpenSalamander/salamander.
- **FR-003**: The README MUST describe the project as a version developed in the new era of agentic programming following Spec-Driven Development principles.
- **FR-004**: The README MUST state that the development process is built on GitHub SpecKit and MUST link to https://github.com/github/spec-kit.
- **FR-005**: The README MUST state that development uses a combination of agentic frameworks with the best available models for the given era, currently Anthropic Fable 5.
- **FR-006**: The README MUST document building the project via `build.cmd` from the repository root, including the main invocation variants (incremental Debug build, `rebuild`, `full`, `full release`) and what each produces.
- **FR-007**: The README MUST list the build prerequisites: Windows 11 or newer, Visual Studio 2022 with the "Desktop development with C++" workload, a Windows 10/11 SDK, and the optional `OPENSAL_BUILD_DIR` environment variable including its default when unset.
- **FR-008**: The README MUST mention that the set of built plugins is controlled by `plugins.cfg` in the repository root.
- **FR-009**: The README MUST NOT duplicate the historical/origin content of the old README (Servant/Altap history, upstream feature/documentation/forum link lists); readers MUST be directed to the upstream Open Salamander project for that content.
- **FR-010**: The README MUST retain a license statement consistent with the project's GPLv2 licensing and existing license files in the repository.
- **FR-011**: The README MUST be written in English, consistent with the repository's documentation convention.
- **FR-012**: The README MUST include a section describing the development process in practice: features are specified, planned, and implemented via the SpecKit workflow, with specifications living in the `specs/` directory of the repository.
- **FR-013**: The README MUST include a brief repository structure overview (top-level directories and their purpose), concise enough not to duplicate the detailed architecture documentation.
- **FR-014**: The README MUST NOT include a project status or roadmap section (such content ages quickly and is tracked in `specs/` instead).
- **FR-015**: The README MUST briefly note that the rebrand is currently at the project/documentation level: the built application (window titles, binary names, installer) still carries the Open Salamander name and branding.

### Key Entities

- **README.md**: The repository front-page document being replaced; the sole artifact of this feature. Its content is fully authored anew (identity, development approach, build instructions, license), superseding the upstream-inherited text.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A first-time reader can correctly answer "what is the project called, what is it based on, and how is it developed" after reading only the first screen of the README (verifiable by inspection of the opening section).
- **SC-002**: A developer on a machine meeting the listed prerequisites can produce a runnable build by following the README alone, without consulting any other document in the repository.
- **SC-003**: 100% of links in the README resolve to their intended targets (upstream repository, GitHub SpecKit, in-repo license files).
- **SC-004**: Zero sections of the old README's historical narrative (origin story, Altap history, upstream resource list) remain duplicated in the new README.
- **SC-005**: Every build command shown in the README exists and behaves as described when executed in a fresh clone (no references to superseded entry points as the primary build path).

## Assumptions

- The README remains in English (repository convention: new documentation and comments are English; the audience of a GitHub front page is international). The user's request being written in Czech does not imply a Czech README.
- The old README content is fully replaced, not appended to — "rebrand" means the document reads as Newt Commander's front page, with upstream information reachable via the link.
- A license statement stays in the README because the project is a GPLv2 derivative; this is treated as part of "further project details" rather than forbidden "old content".
- Renaming the project anywhere else (window titles, binaries, installer, docs other than README.md) is **out of scope** for this feature; only README.md changes. The README itself acknowledges this state (FR-015).
- The documented build entry point is the repository-root `build.cmd` (introduced by feature 002 and extended by feature 007); the older `src\vcxproj` scripts may be mentioned as alternatives at most, not as the primary path.
- "Currently Anthropic Fable 5" is understood as a statement true at the time of writing and phrased so it ages gracefully.
