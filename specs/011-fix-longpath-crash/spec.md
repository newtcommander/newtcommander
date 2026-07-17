# Feature Specification: Fix Application Crash When Entering a Long-Path Directory

**Feature Branch**: `011-fix-longpath-crash`
**Created**: 2026-07-18
**Status**: Draft
**Input**: User description: "Oprava pádu aplikace při vstupu do adresáře s dlouhou cestou. Navazuje na feature 010: po třech kolech oprav (Execute, ReadDirectory, SalCheckPath, GetErrorText) už navigace do cest delších než MAX_PATH neohlásí chybu, ale program při pokusu o vstup do adresáře s dlouhým jménem (např. ...\\long-paths\\ascii-level-zzz..., čistě ASCII, 327 znaků celkem) ÚPLNĚ SPADNE. Cíl: detailní hluboká analýza celého post-navigačního řetězce, jasná identifikace všech míst schopných způsobit pád, návrh řešení a bezchybná oprava, ověřená proti kompletnímu long-path testovacímu stromu."

## Problem Statement

Feature 004 introduced long-path support (paths beyond the legacy 260-character
limit) and feature 010 removed the remaining navigation blockers: the
too-long-name gate in Enter navigation, the listing buffer, the
path-accessibility check truncation, and the error-text encoding. Directory
*entry* into a long path now proceeds — and that is precisely the regression
surface: **the application crashes outright** when the user enters a
long-path directory, for example:

```
C:\Users\pavel\AppData\Local\Temp\salamander-test\010\long-paths\ascii-level-zzz…  (327 chars, pure ASCII)
```

Two facts narrow the defect class decisively:

1. The crash occurs on a **pure-ASCII** path — character encoding is not
   involved. This is a **memory-safety failure**: code holding the new,
   longer-than-260-character current path in fixed-size buffers sized to the
   old limit.
2. Until feature 010, such paths could never become the *current* panel path
   — every consumer downstream of a successful path change (window title,
   per-drive current-directory bookkeeping, path history, directory-change
   monitoring, icon reading, status/free-space reporting, command line, and
   similar) was written when the path length was guaranteed ≤ 260 and was
   therefore never exercised with a longer one.

A crash is categorically worse than any error message: it can lose user
context and interrupt running operations. The goal of this feature is that
entering, working in, and leaving a long-path directory is **stable** —
zero crashes — across the entire post-navigation chain, with every consumer
of the current path either fully long-path capable or safely degrading
(truncated display, skipped optional service) — never corrupting memory.

## Clarifications

### Session 2026-07-18

- Q: The user is not at the keyboard; how are scope decisions made?
  → A: Autonomous execution mandated by the user. Scope, priorities, and
  acceptable-degradation decisions follow the precedents recorded in
  feature 010 (`specs/010-fix-filename-encoding/implementation-notes.md`)
  and the project constitution; no interactive clarification is possible.
- Q: What counts as an acceptable non-crash outcome for a consumer that
  cannot reasonably support long paths (e.g. a legacy OS facility)?
  → A: Safe degradation — the consumer must bound-check and either truncate
  its *display* output at a safe boundary or skip its optional service for
  that path, never overflow. This mirrors the feature 010 accepted-limitation
  precedent (`.lnk` resolution, drive-bar labels).
- Q: Verification depth, given no human at the screen? → A: The fix must be
  verified by (a) a successful clean build, (b) an automated end-to-end
  drive of the running application into every directory of the long-path
  test tree (scripted keystroke navigation), confirming the process
  survives, plus (c) static exhaustion of the audited call chain. A final
  human walkthrough remains listed as follow-up.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Entering a long-path directory never crashes (Priority: P1)

A user navigates (Enter/double-click, Shift+F7 paste, history, hot path)
into a directory whose full path exceeds 260 characters — ASCII or Unicode.
The application enters the directory, lists its content, updates all its
chrome (title, Directory Line, status), and continues running.

**Why this priority**: This is the reported, reproducible defect and a
process-killing failure. Everything else is irrelevant while entry crashes.

**Independent Test**: Drive the built application into each directory of
`%LOCALAPPDATA%\Temp\salamander-test\010\long-paths\` (327-char ASCII,
540-char Unicode, 255-char component, 265-char edge) and verify the process
is alive after each entry and displays the listing.

**Acceptance Scenarios**:

1. **Given** the 327-char ASCII test directory, **When** the user enters it
   from the panel, **Then** the panel lists its content and the application
   keeps running (no crash, no corruption).
2. **Given** the 540-char Unicode test directory chain, **When** the user
   enters level 1 → 2 → 3 successively, **Then** each level lists correctly
   and the application keeps running.
3. **Given** a long-path directory as the current path, **When** the window
   title, Directory Line, and status line update, **Then** they render
   (ellipsized where needed) without any failure.
4. **Given** the deepest test directory, **When** the user navigates back up
   (Backspace/"..") to the root of the tree, **Then** every step succeeds.

---

### User Story 2 - Working inside a long-path directory is stable (Priority: P2)

With a long path as the current directory, the user performs ordinary panel
work: moves the cursor (status line updates per file), refreshes (Ctrl+R),
switches panels, opens the directory in the second panel, views a file (F3),
and leaves the application running while the directory changes on disk
(external change triggers the change monitor).

**Why this priority**: Entry is only the first exposure; every consumer that
periodically touches the current path (monitoring, refresh, icon reading,
free-space polling, focus bookkeeping) is a further crash candidate of the
same class.

**Independent Test**: Scripted session in the deepest test directory
covering cursor movement, refresh, second-panel open, and an externally
created file (triggering the auto-refresh monitor); process alive throughout.

**Acceptance Scenarios**:

1. **Given** the deepest test directory listed in the panel, **When** the
   user moves the cursor across files and presses Ctrl+R, **Then** the
   status line and listing update normally, no crash.
2. **Given** the same directory, **When** a file is created in it externally
   while the application is running, **Then** the panel auto-refresh (or its
   safe absence) does not crash the process.
3. **Given** the same directory open in both panels, **When** the user
   switches focus between panels, **Then** per-panel path bookkeeping does
   not corrupt memory.

---

### User Story 3 - The whole post-navigation chain is audited, not just the crash site (Priority: P3)

Every consumer of the current panel path that runs after a successful path
change is systematically identified, classified (long-path capable / safely
degrading / defective), and every defective site fixed — so that the crash
class (fixed-size buffer receiving an unbounded copy of the current path) is
eliminated, not just the first symptomatic instance.

**Why this priority**: Feature 010's history proves single-site fixes only
move the failure to the next site (three rounds so far). The user explicitly
mandates a deep analysis of *all* aspects.

**Independent Test**: The documented audit table lists every identified
consumer with a verdict; a code sweep for unbounded copies of the current
path into fixed buffers in the audited chain returns no unhandled sites.

**Acceptance Scenarios**:

1. **Given** the audit of the post-navigation chain, **When** each consumer
   is exercised with the long-path test set, **Then** none crashes or
   corrupts memory.
2. **Given** the audit table, **When** a consumer is classified "safely
   degrading", **Then** its degradation is bounded (truncation/skip), never
   an overflow, and is recorded with a rationale.

---

### Edge Cases

- Path lengths at and just above the legacy boundary (259, 260, 261, 265) —
  off-by-one truncations are the classic failure of this class.
- Maximum test depth (540 chars) and a 255-char single component.
- Long path as the *startup* directory (application configured to reopen the
  last path, or path passed at startup) — the post-navigation chain runs
  during initialization.
- Long path in *both* panels simultaneously.
- Leaving the long path (up-navigation, drive change) — consumers that
  compare or store the *previous* path also receive long input.
- Long path recorded into persisted state (history, last-path configuration)
  and read back on next start — must not crash on either side.
- External change notifications on a long-path directory (monitor
  registration may legitimately fail — must degrade, not overflow).
- ASCII and Unicode variants of all of the above (Unicode multiplies byte
  length up to 3×; ASCII 327 chars already crashes, Unicode 540 exercises
  larger byte counts).

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Entering a directory whose full path exceeds the legacy
  260-character limit MUST NOT crash or corrupt the application, for any
  path the operating system can host (up to the platform's extended limit),
  ASCII or Unicode.
- **FR-002**: All application services that consume the current panel path
  after a successful path change (window/taskbar captions, per-drive
  current-directory bookkeeping, path histories, directory-change
  monitoring, icon/thumbnail reading, free-space and status reporting,
  command-line integration, and equivalents) MUST either operate correctly
  with long paths or degrade safely — bounded truncation of display output
  or documented skip of an optional service — never overflowing a buffer.
- **FR-003**: Ordinary work inside a long-path directory (cursor movement,
  refresh, panel switching, dual-panel use, viewing files, external-change
  monitoring) MUST be stable for the whole session.
- **FR-004**: Leaving a long-path directory (up-navigation, drive change,
  history jump) MUST be stable, including consumers that process the
  previous path.
- **FR-005**: Long paths written to persisted state (histories,
  last-path configuration) MUST round-trip without crashing on save or on
  the next start's load; where a store cannot hold the full path, the entry
  MUST be safely omitted or truncated by policy, never overflowed.
- **FR-006**: The team MUST produce a documented audit of the complete
  post-navigation consumer chain with a per-consumer verdict (capable /
  degrading / fixed), so the crash class is provably exhausted, and MUST
  verify the result with an automated end-to-end drive of the running
  application through the long-path test tree.
- **FR-007**: Behavior for paths within the legacy limit MUST remain
  unchanged (zero regressions in navigation, chrome, histories, monitoring).

### Key Entities

- **Current panel path**: The authoritative long-path-capable string owned
  by each panel; the value whose propagation into fixed-size consumers
  causes the crash class.
- **Post-navigation consumer**: Any code that receives or derives from the
  current path after a successful change — classified in the audit as
  *capable* (handles full length), *degrading* (bounded fallback), or
  *defective* (fixed buffer + unbounded copy → must be fixed).
- **Audit table**: The documented list of all consumers with verdicts and
  resolutions; the deliverable proving class exhaustion (FR-006).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Automated end-to-end navigation into 100% of the directories
  of the long-path test tree (327-char ASCII, 540-char Unicode chain,
  255-char component, 265-char edge) completes with the process alive and
  responsive after every step.
- **SC-002**: A scripted work session inside the deepest directory (cursor,
  refresh, dual-panel, external change) completes with zero crashes.
- **SC-003**: The audit table covers 100% of identified post-navigation
  consumers with an explicit verdict; zero sites remain classified
  *defective*.
- **SC-004**: All navigation and chrome behavior for sub-260 paths is
  unchanged (smoke pass on standard directories, zero regressions).
- **SC-005**: A clean full rebuild passes with no new warnings in the
  changed files.

## Assumptions

- Builds directly on feature 010's context
  (`specs/010-fix-filename-encoding/implementation-notes.md`): the
  navigation chain up to and including listing is long-path capable as of
  commit `6050f91`; the crash therefore originates in consumers running
  after (or concurrently with) the successful change. If analysis finds a
  remaining defect *inside* the navigation chain, fixing it is in scope.
- The debug build's runtime checks make buffer overwrites fatal
  (deterministic crash), which is why the defect manifests as a hard crash;
  the release build would corrupt memory silently — the fix removes the
  overflow, not the check.
- The long-path test tree from feature 010
  (`%LOCALAPPDATA%\Temp\salamander-test\010\long-paths\`) is the
  verification data set; it already covers ASCII/Unicode and the boundary
  lengths.
- Feature 010's accepted limitations remain accepted (shortcut resolution,
  drive-bar volume labels, reparse retry anchors) — provided they are
  crash-safe; any of them found to overflow is in scope to make safe.
- Autonomous execution: all phase decisions are made without user
  interaction, following feature 010 precedents; verification is automated
  (scripted application drive) with a final human walkthrough listed as
  follow-up.
