# Feature Specification: Long-Path File Operations (Copy, Move, View, and the Rest)

**Feature Branch**: `012-longpath-file-operations`
**Created**: 2026-07-18
**Status**: Draft
**Input**: User report: browsing long-path directories now works (feature 011),
but *operating* on files inside them fails — F3 (view file content) shows a
"name too long" error for some files, and attempting to **copy** a file
**crashed the application** and left the source long-path directory
apparently gone from the panel. The user asked for a deep analysis of all
follow-up operations (view, copy, rename, move, delete, and more) and a
complete fix.

## Problem Statement

Feature 004 made the panel's *current path* long-path capable and features
010/011 fixed *navigation* into directories whose full path exceeds the
legacy 260-character (`MAX_PATH`) limit. That work stopped at the panel
boundary: the moment the user performs an **operation** on a file or
directory inside a long path, the code composes a *full item path*
(`current path` + `\` + `name`) — and almost every operation still does that
in a fixed 260-`MAX_PATH`-class buffer written by an unbounded copy. The
result, confirmed by a four-part source audit and the user's crash, is that
long-path support is **doubly broken** for operations:

- **Crashes** (stack-buffer overrun, `STATUS_STACK_BUFFER_OVERRUN`): the F5
  copy/move engine's *script builder*, and every shell-backed action
  (Properties, New, right-click menu, clipboard Copy/Cut, drag-and-drop),
  overflow a fixed path buffer and terminate the process — the exact copy
  crash the user hit.
- **Refusals / errors** (bounded "name too long"): viewing (F3), editing
  (F4), and creating a directory (F7) reject items once the composed path
  passes the limit, even though the underlying file APIs are long-path
  capable.
- **Apparent data loss** (the "directory disappeared" symptom): the source
  directory is *not* actually deleted — the copy engine's file I/O uses
  long-path-safe dynamic strings — but (a) the crash killed the process
  mid-operation, and (b) the panel's last path is saved to the registry in
  full yet **reloaded truncated**, and post-operation change-notifications
  compare a **truncated** current path, so the panel is steered away from
  the long directory and cannot return to it, in-session and across
  restarts.

The goal is that **every file operation available from the panel works, or
degrades safely, for items inside long-path directories** — no crashes, no
truncation that operates on the wrong file, and no persistent loss of the
panel's location — with genuine external-API limits (shell verbs, shortcut
resolution) degrading to a clear bounded error rather than a crash.

## Clarifications

### Session 2026-07-18

- Q: The user is not at the keyboard; how are scope and verification handled?
  → A: Fully autonomous execution (as with features 011). Scope, priorities,
  and safe-degradation choices follow the audit
  ([research.md](research.md)) and feature 004/010/011 precedents. Automated
  verification (build + scripted drive where possible + static exhaustion of
  the audited sinks); a final human walkthrough is listed as follow-up.
- Q: For operations whose limit is a genuine external Windows API (shell
  context-menu/verb invocation, `IShellLink` shortcut resolution, external
  viewer/editor command line, the shell Recycle Bin), what is the required
  behavior? → A: **Safe degradation** — a clear bounded "name too long"
  message and no state corruption; never a crash. Making those long-path
  capable is out of scope (they cannot be without replacing the external
  API).
- Q: What is the required outcome for the "directory disappeared" symptom?
  → A: The panel must be able to remain in, and return to, a long-path
  directory across refresh, operations, and application restart — the last
  path must round-trip through persistence without truncation. No actual
  file data may be lost by any operation.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Copying and moving files/dirs in a long path never crashes (Priority: P1)

A user selects one or more files (and/or subdirectories) inside a directory
whose path exceeds 260 characters and copies (F5) or moves (F6) them to
another location — and vice-versa, copies items *into* a long-path target.
The operation completes, or reports a clear error, but never crashes and
never operates on the wrong path.

**Why this priority**: This is the reported, process-killing defect with a
perceived data-loss aftermath. Copy/move are among the most frequent and
most destructive-if-wrong operations.

**Independent Test**: With the long-path test tree, copy a file out of the
291-char directory to a short target, copy a file into the long directory,
and move a subtree; verify the process survives, the bytes match, and the
source is intact until a move's copy half succeeds.

**Acceptance Scenarios**:

1. **Given** a file inside a 291-char directory, **When** the user copies it
   (F5) to a short target directory, **Then** the copy succeeds and the app
   keeps running.
2. **Given** a file in a short directory, **When** the user copies it into a
   291-char target directory, **Then** the copy succeeds.
3. **Given** a subdirectory inside a long path (a directory copy exercises
   the recursive script builder), **When** the user copies or moves it,
   **Then** the operation completes or reports a bounded error — never
   crashes.
4. **Given** a move (F6) of a file out of a long path, **When** the copy half
   fails or is refused, **Then** the source file is **not** deleted (no data
   loss).

---

### User Story 2 - Viewing, editing, and opening files in a long path works (Priority: P2)

A user presses F3 (internal viewer), F4 (edit), or Enter (open/execute) on a
file inside a long-path directory. The internal viewer shows the content;
editing and opening either work or, where an external program imposes the
limit, report a clear bounded message.

**Why this priority**: The reported F3 "name too long" error; viewing is a
frequent, read-only, low-risk operation that should simply work with the
internal viewer since the file APIs are long-path capable.

**Independent Test**: F3 a file inside the 291-char directory and confirm the
internal viewer displays it; step to the next file (Space) within the long
directory; F4/Enter behave without crashing.

**Acceptance Scenarios**:

1. **Given** a file inside a 291-char directory, **When** the user presses F3,
   **Then** the internal viewer opens and shows the file content (no "name
   too long" error).
2. **Given** the internal viewer open on that file, **When** the user steps
   to the next/previous file in the same long directory, **Then** it opens
   the correct neighboring file.
3. **Given** a file in a long path opened via an external viewer/editor or
   the shell, **When** the composed command exceeds the OS command-line or
   shell limit, **Then** a clear bounded error is shown — never a crash.

---

### User Story 3 - Shell-backed actions in a long path never crash (Priority: P2)

A user invokes Properties (Alt+Enter), the New submenu (F7 shell "New"), the
right-click context menu, clipboard Copy/Cut (Ctrl+C/Ctrl+X), or
drag-and-drop on items inside a long-path directory. These route through the
Windows shell; they must not crash the process — they either work or show a
bounded error.

**Why this priority**: Clipboard Copy/Cut and the context menu are a second
route by which the user's "copy" crashed; Properties/New/drag share the same
crash sink and are common actions.

**Independent Test**: On a file inside the 291-char directory, invoke
Properties, Copy to clipboard, and start a drag; verify the process survives
each.

**Acceptance Scenarios**:

1. **Given** a file inside a 291-char directory, **When** the user opens
   Properties or the right-click context menu, **Then** the app keeps
   running (menu shown, or a bounded shell-limit error).
2. **Given** the same file, **When** the user presses Ctrl+C / Ctrl+X, **Then**
   the clipboard operation completes or reports a bounded error — no crash.
3. **Given** a long-path panel, **When** the user drags files out of it or
   drops onto it, **Then** the drag/drop does not crash.

---

### User Story 4 - The panel stays in (and returns to) a long-path directory (Priority: P2)

A user works in a long-path directory across refreshes, file operations, and
an application restart. The panel remains pointed at the long directory (or
returns to it on restart) and never silently jumps away because its path was
truncated.

**Why this priority**: This is the "directory disappeared / stays invisible"
symptom. Even without data loss, losing the location and being unable to
return is a serious usability failure.

**Independent Test**: Navigate into the 291-char directory, trigger a refresh
(and an external change), then close and reopen the app configured to restore
the last path; verify the panel is (or returns) there.

**Acceptance Scenarios**:

1. **Given** the panel in a 291-char directory, **When** a refresh or an
   external change notification occurs, **Then** the panel stays in that
   directory (the change match is not fooled by a truncated path).
2. **Given** the panel in a 291-char directory, **When** the application is
   closed and reopened with "restore last path" enabled, **Then** the panel
   returns to that directory (the saved path round-trips without truncation),
   or — where the store genuinely cannot hold it — falls back to a valid
   ancestor without error.
3. **Given** any move/delete inside a long path, **When** it completes,
   **Then** the correct source and target directories are refreshed (the
   change notification is not truncated to a wrong path).

---

### User Story 5 - The whole file-operation surface is audited, not just the crash sites (Priority: P3)

Every panel operation that composes a full item path — copy, move, delete
(recycle + permanent), rename, create directory, change attributes,
properties, view, edit, open/execute, calculate size, change case, convert,
pack/unpack, clipboard, drag-drop — is identified, classified (long-path
capable / safely degrading / defective), and every defective site fixed, so
the crash-and-truncation class is eliminated across the operation surface.

**Why this priority**: Feature 011's history shows single-site fixes only
move the failure to the next operation. The user explicitly asked to cover
*all* follow-up functions, not just copy.

**Independent Test**: Walk the documented operation-audit table; each row has
a verdict, and exercising it with the long-path test set produces no crash
and no wrong-path action.

**Acceptance Scenarios**:

1. **Given** the operation-audit table, **When** each operation is exercised
   on the long-path test set, **Then** none crashes or acts on a truncated
   path.
2. **Given** a site classified "safely degrading" (external-API limit), **When**
   it is hit with a long path, **Then** it shows a bounded error and leaves
   state intact.

---

### Edge Cases

- A directory *copy/move* (recursive builder) versus a single-file copy —
  they hit different buffers; both must be covered. A single-file copy that
  today is silently *refused* (name-too-long gate) must instead work.
- Path lengths that overflow the larger operation buffers (`2*MAX_PATH`,
  `2*MAX_PATH+200` ≈ 520–720): the Unicode test chain (up to ~540 bytes) and
  a deep nested chain approach these thresholds.
- A move whose copy half fails/refuses: the source must never be deleted.
- Truncation to a *different real directory* (a shorter prefix that exists):
  enumeration/delete must never target it.
- Post-operation refresh and external change notifications on a long path
  must match the real path, not a 260-char prefix.
- Restart with a long last-path (restore-last-path enabled) and a long path
  saved in history.
- Genuine external limits (shell verbs, `IShellLink`, external editor command
  line, Recycle Bin) must degrade to a bounded message, never a crash.
- ASCII and Unicode variants of every case (Unicode multiplies byte length
  up to 3×).

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Copying or moving files and directories to or from a directory
  whose path exceeds `MAX_PATH` MUST NOT crash or corrupt memory, for any
  path the operating system can host (ASCII or Unicode), including the
  recursive directory-copy script builder and its error/confirmation
  messages.
- **FR-002**: A single-file copy/move whose composed source or target path
  exceeds the legacy limit MUST be performed (not silently refused), using
  the long-path-capable file APIs, up to the platform's extended path limit.
- **FR-003**: Viewing (internal viewer, F3), stepping to adjacent files in
  the viewer, and editing (F4) files inside a long-path directory MUST work
  with the internal tools; where an external program or the shell imposes the
  limit, the operation MUST show a bounded "name too long" message and never
  crash.
- **FR-004**: Shell-backed actions — Properties, the New submenu, the
  right-click context menu, clipboard Copy/Cut, and drag-and-drop — on items
  inside a long-path directory MUST NOT crash; they either function or show a
  bounded error.
- **FR-005**: No operation may truncate a full item path to a shorter,
  *existing* path and then act on it (enumerate, copy, delete, or notify) —
  operations MUST act on the true path or fail safely; in particular a move
  MUST NOT delete a source that was not successfully copied.
- **FR-006**: The panel MUST be able to stay in and return to a long-path
  directory across refresh, file operations, and application restart: the
  current path MUST round-trip through persistence and change-notification
  matching without truncation, or fall back to a valid ancestor without
  error where a store genuinely cannot hold it.
- **FR-007**: Creating a directory (F7) whose resulting path exceeds the
  legacy limit MUST succeed up to the platform limit (not be refused by an
  internal `MAX_PATH` bound).
- **FR-008**: The team MUST produce and document an audit of the complete
  panel file-operation surface with a per-operation verdict (capable /
  degrading / fixed) so the crash-and-truncation class is provably
  exhausted, and MUST verify the result (build + automated drive where the
  environment permits + static exhaustion of the audited sinks).
- **FR-009**: Behavior for operations on items within the legacy limit MUST
  remain unchanged (zero regressions in copy, move, delete, rename, view,
  edit, properties, and clipboard for normal paths).

### Key Entities

- **Full item path**: `current path` + separator + item name; the string
  every operation composes and the value whose copy into fixed buffers
  causes the crash/refusal class.
- **Operation script**: the built list of per-item source/target work
  records the copy/move/delete engine executes; its per-item paths are
  dynamic (safe), but the *builder's* scratch/error buffers are the defect.
- **Shell folder binding**: the shell `IShellFolder`/data-object/context-menu
  binding for the current directory, built from the panel path — a shared
  crash sink for all shell-backed actions.
- **Persisted panel path**: the last directory saved to and restored from
  configuration; the truncating restore is the persistent "disappeared
  directory" cause.
- **Operation-audit table**: the documented per-operation verdict list
  (FR-008 deliverable).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Copying a file out of, and into, a 291-char directory, and
  copying/moving a subdirectory whose path exceeds the legacy limit, complete
  with the process alive and the bytes intact in 100% of attempts.
- **SC-002**: Pressing F3 on a file inside a 291-char directory shows the file
  content in the internal viewer in 100% of attempts (no "name too long"
  error); F4/Enter never crash.
- **SC-003**: Properties, clipboard Copy/Cut, the context menu, New, and
  drag-and-drop on items inside a 291-char directory leave the process alive
  in 100% of attempts.
- **SC-004**: No operation on the long-path test set deletes a source that
  was not successfully copied, and none acts on a truncated (wrong) path —
  verified by content and directory-listing checks.
- **SC-005**: After navigating into a 291-char directory, closing and
  reopening the application (restore-last-path enabled) returns the panel to
  that directory, or to a valid ancestor, with no error.
- **SC-006**: The operation-audit table covers 100% of the identified panel
  operations with an explicit verdict; zero sites remain classified
  *defective*.
- **SC-007**: All operations on sub-260 paths behave exactly as before (zero
  regressions), and a clean full rebuild passes with no new warnings in the
  changed files.

## Assumptions

- Builds on features 004/010/011: the panel path, navigation, and the copy
  engine's file I/O (dynamic `char*` source/target) are already long-path
  capable; the residual defects are in the *operation path-composition
  buffers*, the *shell binding*, the *builder error/confirmation buffers*,
  the *single-file build gate*, and *path persistence / change-notification*
  — as established by the four-part audit in [research.md](research.md).
- The user runs the **Release** build; `/GS` turns the stack overruns into
  hard `STATUS_STACK_BUFFER_OVERRUN` crashes (confirmed from feature 011's
  dumps). The fix removes the overrun, not the check.
- The long-path test tree from feature 010/011
  (`%LOCALAPPDATA%\Temp\salamander-test\010\long-paths\`) plus files to copy
  is the verification data set; it covers ASCII and Unicode and boundary
  lengths.
- Genuine external-API limits (shell verb invocation, `IShellLink`
  resolution, external viewer/editor command line, the shell Recycle Bin)
  are accepted limitations: they must degrade safely (bounded error), and
  making them long-path capable is out of scope.
- The interactive-desktop constraint of the autonomous environment (keystroke
  automation may be unavailable) means verification leans on the build,
  command-line-driven navigation, and static exhaustion; a final human
  walkthrough is follow-up.
