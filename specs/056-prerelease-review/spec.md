# Feature Specification: Pre-release Review and Release of Version 0.1.2

**Feature Branch**: `056-prerelease-review`
**Created**: 2026-08-07
**Status**: Draft
**Input**: User description: "Alokuj několik nezávislých agentů a realizuj celkovou revizi provedených úprav před vydáním verze 0.1.2. Zaměř se na celkovou stabilitu a bezbečnost. Cílem této úpravy je celkové otestování a revize této verze před vydáním. Zároveň v rámci tohoto úkolu aktualizuj číslo verze na 0.1.2, uprav changelog tak, že verze 0.1.2 bude dnes vydána."

## Background

Version 0.1.1 (build 185) was released on 2026-08-05. Since then the tree has
accumulated four features awaiting release: the plugin-metadata encoding
contract that fixed garbled plugin names (052), two rounds of SFTP plugin
work — the connect dialog rework and the reachable-settings / overlapping
connect fixes (053, 054) — and the product-wide contextual re-translation of
machine-translated UI strings (055). These touch security-sensitive territory
(SSH connection handling, credential prompts, text-encoding conversions) and
high-traffic UI (every non-English string in the product).

This feature is the release gate for **0.1.2**: a multi-perspective review of
everything unreleased, carried out by several independent reviewers so no
single reader's blind spots decide what ships; confirmed problems fixed or
knowingly deferred; the existing automated test suites re-run; and the
release mechanics performed — version number raised to 0.1.2 everywhere it is
mandated, changelog section for 0.1.2 dated today (2026-08-07).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Independent stability & security review of the unreleased delta (Priority: P1)

Multiple independent reviewers, each with a distinct focus (memory and
resource safety, concurrency and UI-thread discipline, network/protocol
security, credential handling, encoding correctness, translation-data
integrity), examine every change made since version 0.1.1. Each raised
finding is adversarially verified before it is accepted as real — a finding
survives only if a second, skeptical examination confirms it. Confirmed
defects of consequence are fixed before release; anything consciously not
fixed is recorded with a reason.

**Why this priority**: The delta includes SSH connection logic, credential
prompts, thread handoffs, and text-encoding conversions — the classes of code
where a defect means a hang, a crash, or a security exposure in the hands of
every user of the release. The release must not ship unreviewed.

**Independent Test**: The review report exists, lists every reviewed area
against the actual set of unreleased changes, shows at least three
independent review perspectives, and every finding carries a verified
verdict plus a resolution (fixed / deferred-with-reason / refuted).

**Acceptance Scenarios**:

1. **Given** the set of all changes since the 0.1.1 release, **When** the
   review completes, **Then** every changed area (core encoding contract,
   SFTP plugin behaviour, translation tooling, translation data, build/
   release scripts) has been examined by at least one reviewer whose focus
   covers it, and the report says which reviewer covered what.
2. **Given** a finding raised by any reviewer, **When** it is evaluated,
   **Then** an independent verification pass classifies it as confirmed or
   refuted before any code change is made from it.
3. **Given** a confirmed finding rated critical or high for stability or
   security, **When** the release proceeds, **Then** the finding has been
   fixed and re-verified — or the release is blocked.
4. **Given** a confirmed finding rated lower, **When** it is not fixed in
   this release, **Then** the report records it with a reason and it is
   visible for future planning.

---

### User Story 2 - The release is testably green before it ships (Priority: P1)

The existing automated verification of the product passes on the exact
source state that becomes 0.1.2: the full build of the application, all
plugins and all 8 shipped languages completes without errors; the SFTP
plugin's behavioural test harness passes against the reference server; the
encoding-contract guard and the translation round-trip check pass; and the
application starts and presents a working UI in a non-English language.

**Why this priority**: Review reads code; tests execute it. Both are
required for a release claim. Co-equal P1 — a clean review of a build that
does not pass its own tests is worthless.

**Independent Test**: Each named verification can be run standalone and its
pass/fail result is recorded in the release report.

**Acceptance Scenarios**:

1. **Given** the release source state, **When** the full build runs, **Then**
   it produces the application, all enabled plugins and all 8 language sets
   with zero errors.
2. **Given** the SFTP reference server is available, **When** the SFTP
   behavioural harness runs, **Then** all its scenarios pass; key-format
   fixture checks pass likewise.
3. **Given** the built application, **When** it is started with a non-English
   language, **Then** it opens usably (no missing-resource failures, correct
   plugin names in the Plugins Manager).

---

### User Story 3 - Version 0.1.2 is stamped and documented as released today (Priority: P2)

A user who installs or inspects the released product sees version 0.1.2
everywhere a version is shown — the application, every plugin's version
resource, the installer — and the changelog opens with a 0.1.2 section dated
2026-08-07 that describes, in user language, what this release contains.
Project documentation that records the current version says 0.1.2.

**Why this priority**: This is the release act itself, but it is mechanical;
it must not happen before the P1 gates pass, which is why it is ordered
after them despite being mandatory for the feature.

**Independent Test**: A sweep over the version-bearing files shows the same
new version and build number everywhere the project's release rule mandates;
the changelog's first released section is 0.1.2 dated 2026-08-07 and the
previously unreleased entries now live under it.

**Acceptance Scenarios**:

1. **Given** the release commit, **When** version-bearing sources are
   inspected, **Then** the product version reads 0.1.2 with a build number
   one higher than 0.1.1's, consistently in every mandated location, and the
   built application and installer report 0.1.2.
2. **Given** the changelog, **When** a user reads it, **Then** the
   `[Unreleased]` content of this cycle appears under `## [0.1.2] — 2026-08-07`
   with a short release summary line, and `[Unreleased]` is empty or absent.
3. **Given** the review found release-blocking problems, **When** they are
   not yet resolved, **Then** the version stamp and changelog release do not
   happen (the release gate holds).

---

### Edge Cases

- **A reviewer finds a defect in code that predates 0.1.1**: it is recorded
  and classified like any finding, but only regressions introduced since
  0.1.1 or pre-existing issues of critical severity block this release.
- **Two reviewers disagree** (one confirms, one refutes): the finding goes to
  an additional independent verification; the majority of verifying passes
  decides, and the report keeps the dissent visible.
- **The SFTP reference server is unavailable**: the behavioural harness
  cannot run; the release is blocked until it runs, or the maintainer
  explicitly accepts the gap in the report.
- **The review finds nothing at all**: suspicious for a delta of this size —
  the report must show what was actually examined (files, scenarios) so
  "clean" is distinguishable from "unread".
- **Version collision**: if any version-bearing file already carries 0.1.2 or
  a build number ahead of the expected one, the sweep flags it instead of
  silently overwriting.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The review MUST cover the complete change set between the
  0.1.1 release and the current tree — application core, SFTP plugin,
  translation tooling, translation data, and build/release scripting — and
  the resulting report MUST map each area to the reviewer(s) that examined it.
- **FR-002**: The review MUST be performed by several independent reviewers
  with distinct, stated focuses, together covering at minimum: memory and
  resource safety; concurrency and UI-thread discipline; network and
  protocol security including host-key trust and error handling; credential
  and secret handling; text-encoding correctness; and translation-data
  integrity (placeholders, accelerators, resource structure).
- **FR-003**: Every raised finding MUST pass an independent verification
  step before it is treated as real; refuted findings are kept in the report
  as refuted, not silently dropped.
- **FR-004**: Confirmed findings MUST be triaged by severity; critical and
  high stability/security findings MUST be fixed and re-verified before the
  version stamp happens, and every fix MUST itself be reviewed.
- **FR-005**: Confirmed findings that are not fixed in this release MUST be
  recorded with severity and a deferral reason in the release report.
- **FR-006**: The release gate MUST include the project's existing automated
  checks, all passing on the release source state: the full product build
  including all shipped languages, the SFTP behavioural harness and
  key-format fixtures, the encoding-contract guard, and the translation
  round-trip verification.
- **FR-007**: The built application MUST be smoke-verified: it starts,
  presents a usable UI in at least one non-English language, and the Plugins
  Manager lists plugins with correctly rendered names.
- **FR-008**: The version MUST be raised to 0.1.2 with the build number
  incremented by one, consistently in every location the project's release
  rule mandates (application/plugin version resources, installer version,
  and the project documentation line that records the current version), in
  the same change as the changelog release.
- **FR-009**: The changelog MUST gain a `## [0.1.2] — 2026-08-07` section
  containing this cycle's previously unreleased entries plus a one-line
  release summary, following the structure of previous releases.
- **FR-010**: The release report MUST be persisted with the feature
  documentation: reviewed areas and their reviewers, all findings with
  verdicts and resolutions, test-gate results, and the version-consistency
  sweep result.
- **FR-011**: The version stamp and changelog release MUST NOT be performed
  while any release-blocking finding or failing gate remains.

### Key Entities

- **Unreleased delta**: all source, data, and documentation changes between
  the 0.1.1 release state and the current tree (features 052, 053, 054, 055
  and housekeeping).
- **Review perspective**: an independent reviewer with a stated focus area;
  perspectives are chosen so their union covers the whole delta.
- **Finding**: a claimed defect or risk — carries origin (which reviewer),
  location, severity, verification verdict (confirmed / refuted), and
  resolution (fixed / deferred / release-blocking).
- **Release gate**: the set of conditions that must all hold before the
  version stamp: no open release-blocking findings, all automated checks
  green, smoke verification done.
- **Version-bearing locations**: the files the project's release rule names
  as carrying the version and build number, plus the changelog.
- **Release report**: the persisted record proving the gate was satisfied.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% of the files changed since the 0.1.1 release are covered
  by at least one review perspective, and the report can show the mapping.
- **SC-002**: At least 3 independent review perspectives run, and every
  finding carries an explicit verified verdict; 0 findings are resolved by
  the reviewer who raised them without independent verification.
- **SC-003**: 0 confirmed critical or high stability/security findings
  remain open at the moment the version stamp is made.
- **SC-004**: All named automated gates pass on the release source state:
  full build with 0 errors and all 8 languages produced, SFTP behavioural
  scenarios 100% pass, key-format fixtures pass, encoding guard pass,
  translation round-trip pass.
- **SC-005**: The version sweep finds 0.1.2 and the incremented build number
  in 100% of mandated locations and no stale 0.1.1 stamp in any of them; the
  changelog's newest released section reads 0.1.2 with today's date.
- **SC-006**: The release report documents every finding (including refuted
  and deferred ones) — none exist only in a reviewer's transient output.

## Assumptions

- "Several independent agents" is an explicit request for multi-agent
  orchestration during implementation; the spec expresses it
  technology-neutrally as independent review perspectives with adversarial
  verification.
- The review baseline is the released 0.1.1 state (git tag `v0.1.1`); the
  delta is everything on the current branch since then, including feature
  055's translation refresh already merged into this line of work.
- Build number for 0.1.2 is 186 (0.1.1 was 185; the project rule is +1 per
  release). The plugin interface version is independent and is NOT bumped —
  no plugin API change happened in this cycle.
- The release act in this feature covers source-level release readiness:
  version stamps, changelog, green gates, and a successful Release build.
  Producing and publishing the signed installer (`sign` / `setup` build
  steps) and the git tag remain the maintainer's push-button steps after
  merge, per the existing code-signing workflow; they are out of scope here.
- The SFTP behavioural harness needs the local Docker reference server
  (`tandem-sftp`, localhost:2222); it is expected to be available as in
  feature 051/053/054 verification runs.
- Pre-existing defects (present already in 0.1.1) found during review do not
  block this release unless critical; they are recorded for future work.
- The changelog keeps an empty `[Unreleased]` section head for the next
  cycle, matching the file's existing convention.
