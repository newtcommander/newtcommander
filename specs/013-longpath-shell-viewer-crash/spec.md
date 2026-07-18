# Feature Specification: Fix Remaining Long-Path Crashes — Viewer Thread & Shell Machinery

**Feature Branch**: `013-longpath-shell-viewer-crash`
**Created**: 2026-07-18
**Status**: Draft
**Input**: User report after feature 012: F3 (view) still crashes/does not
work, and clipboard Copy/Paste (Ctrl+C / Ctrl+V) in a long-path directory
crashes the application again. Requested a full re-run of the analysis and
fix flow.

## Problem Statement

Features 010–012 made navigation and most operations long-path capable, but
two crash sites remained and were **pinpointed from the user's fresh
Release-build crash dumps** (symbolicated against the matching PDB), both
`STATUS_STACK_BUFFER_OVERRUN` (0xC0000409):

1. **F3 / internal viewer** — the viewer runs on its own thread, whose body
   copies the file name into a fixed `MAX_PATH` stack buffer with an
   unbounded `strcpy`. Feature 012 fixed the panel-side compose so the *full
   long name* is now handed to the viewer — which promptly overruns this
   buffer. (Before feature 012 the panel rejected long names, so the viewer
   thread never received one; the fix moved the failure downstream.)

2. **Clipboard Copy/Paste (Ctrl+C / Ctrl+V) and drag-drop** — the Windows
   shell drop-target / data-object machinery holds the panel and source
   paths in fixed `MAX_PATH` and `2*MAX_PATH` (≈520-byte) buffers (both
   object members and stack locals). A Unicode long path (the test tree
   reaches ~540 UTF-8 bytes, above 520) overruns them; an ASCII path in the
   260–520 range overruns the `MAX_PATH` ones. Feature 012 widened one such
   buffer (`GetShellFolder`) but the drop-target's own path store and its
   siblings were missed.

The goal is that F3/view and clipboard/drag operations inside a long-path
directory never crash — the internal viewer shows the file, and clipboard
Copy/Paste and drag-drop complete or degrade safely — with genuine
external-API limits (shell verb invocation) degrading to a bounded error,
not a crash.

## Clarifications

### Session 2026-07-18

- Q: Autonomous execution, headless environment? → A: Yes (as features
  011/012). Verification is build (Debug+Release) + crash-dump forensic
  confirmation of the fixed frames + static exhaustion of the shell/viewer
  path buffers; interactive keystroke automation is unavailable in the
  environment, so the user performs the final walkthrough.
- Q: Scope boundary? → A: Only the two dump-confirmed crash clusters (viewer
  thread; shell drop-target/clipboard/data-object/context-menu path
  buffers). Genuine external-API limits (shell verb command length,
  `IShellLink`) remain safe degradation.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Viewing a file in a long-path directory never crashes (Priority: P1)

A user presses F3 on a file inside a directory whose path exceeds the legacy
limit. The internal viewer opens and shows the content; the application keeps
running.

**Why this priority**: Reported, reproducible, process-killing; view is a
frequent read-only operation.

**Independent Test**: F3 on a file inside the 291-char ASCII directory and
inside the Unicode chain; the viewer displays the file and the process
survives.

**Acceptance Scenarios**:

1. **Given** a file inside a long-path directory, **When** the user presses
   F3, **Then** the internal viewer opens and shows the content, no crash.
2. **Given** the viewer open on that file, **When** its window caption shows
   the (long) path, **Then** it renders (possibly shortened) without failure.

---

### User Story 2 - Clipboard Copy/Paste in a long-path directory never crashes (Priority: P1)

A user selects files inside a long-path directory, copies (Ctrl+C) or cuts
(Ctrl+X), and pastes (Ctrl+V) — into a long-path target and from a long-path
source. The operation completes or reports a bounded error; the application
never crashes.

**Why this priority**: The reported, process-killing copy crash via the
clipboard route.

**Independent Test**: Ctrl+C a file in the 291-char directory, Ctrl+V into a
short directory and into the Unicode long directory; process survives and
bytes match.

**Acceptance Scenarios**:

1. **Given** files inside a long-path directory, **When** the user presses
   Ctrl+C then Ctrl+V into another directory, **Then** the paste completes
   and the app keeps running.
2. **Given** a long-path directory as the paste target, **When** the user
   pastes files into it, **Then** the process survives (completes or bounded
   error).
3. **Given** a Unicode long path (~540 bytes), **When** it is the copy source
   or paste target, **Then** no buffer is overrun.

---

### User Story 3 - Drag-and-drop and shell context actions in a long path never crash (Priority: P2)

Drag-drop out of / onto a long-path panel, and the right-click context menu /
Properties / New on items inside it, do not crash — they function or show a
bounded error.

**Why this priority**: Same shell machinery as Story 2; the drop target and
context-menu binding share the path buffers.

**Independent Test**: Start a drag from the long-path panel and open the
context menu on a file in it; the process survives.

**Acceptance Scenarios**:

1. **Given** a file inside a long-path directory, **When** the user starts a
   drag or opens its context menu, **Then** the app keeps running.
2. **Given** a shell verb whose composed command exceeds the OS limit, **When**
   invoked, **Then** it fails gracefully (bounded error), no crash.

---

### Edge Cases

- Unicode long path exceeding the `2*MAX_PATH` (520-byte) shell buffers, and
  ASCII paths in the 260–520 range exceeding the `MAX_PATH` ones.
- Copy source and paste target both long simultaneously.
- The viewer caption (long path) and the viewer "next/previous file" step
  within a long directory.
- Genuine external limits (shell verb command length, `IShellLink`) → bounded
  error, never crash.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Viewing a file (internal viewer, F3) inside a directory whose
  path exceeds the legacy limit MUST NOT crash; the viewer thread MUST handle
  the full long file name.
- **FR-002**: Clipboard Copy/Cut/Paste (Ctrl+C/Ctrl+X/Ctrl+V) on items inside
  or into a long-path directory MUST NOT crash or corrupt memory, for any
  path the OS can host (ASCII or Unicode up to the extended limit); it either
  completes or shows a bounded error.
- **FR-003**: Drag-and-drop and shell context actions (context menu,
  Properties, New) on items inside a long-path directory MUST NOT crash; the
  underlying shell drop-target / data-object / context-menu path buffers MUST
  hold long paths or degrade safely.
- **FR-004**: No shell/viewer path buffer may be overrun by a long path;
  genuine external-API limits (shell verb command length, shortcut
  resolution) MUST degrade to a bounded error, never a crash.
- **FR-005**: The fix MUST be confirmed against the user's crash dumps (the
  previously faulting frames no longer overrun) and by a clean Debug+Release
  build, with a static sweep showing no remaining unbounded copy of a path
  into a fixed shell/viewer buffer.
- **FR-006**: Behavior for view/clipboard/drag on items within the legacy
  limit MUST remain unchanged (zero regressions).

### Key Entities

- **Viewer thread data**: the file name (and caption) handed to the viewer
  thread; the fixed buffer that copies it is the F3 defect.
- **Shell drop-target / data-object binding**: the object holding the panel
  and source paths (`CurDir`, source-FS-path, source-path members and their
  stack siblings) — the clipboard/drag defect cluster.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: F3 on a file inside the 291-char and Unicode long directories
  shows the content with the process alive in 100% of attempts.
- **SC-002**: Ctrl+C then Ctrl+V involving a long-path source or target
  completes with the process alive and bytes intact in 100% of attempts.
- **SC-003**: Drag-drop and context menu / Properties on items inside a
  long-path directory leave the process alive in 100% of attempts.
- **SC-004**: The two dump-confirmed faulting frames (viewer thread body;
  shell drop-target `SetDirectory`) no longer contain an unbounded path copy;
  a clean Debug+Release build passes with no new warnings in changed files.
- **SC-005**: View/clipboard/drag on sub-260 paths behave exactly as before.

## Assumptions

- Root causes are dump-confirmed (fresh 2026-07-18 07:14–07:15 Release
  minidumps, `STATUS_STACK_BUFFER_OVERRUN`, symbolicated to the viewer thread
  body and `CImpDropTarget::SetDirectory`/`GetShellFolder` chain).
- The copy engine's file I/O is long-path safe (feature 012), so no file data
  is lost; these are crash sites in the view and shell-clipboard paths.
- The long-path test tree (`%LOCALAPPDATA%\Temp\salamander-test\010\long-paths\`)
  including the Unicode chain (~540 bytes) is the verification data set.
- Genuine external-API limits remain accepted safe-degradation (feature 012
  precedent).
- Autonomous / headless execution; interactive keystroke automation is
  unavailable, so verification relies on build + dump forensics + static
  exhaustion, with a final human walkthrough as follow-up.
