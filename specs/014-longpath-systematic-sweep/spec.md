# Feature Specification: Systematic Whole-Program Long-Path Hardening

**Feature Branch**: `014-longpath-systematic-sweep`
**Created**: 2026-07-18
**Status**: Draft
**Input**: User: take the full accumulated context of all long-path problems
(the saved md files and prior conversation) and prepare a detailed plan to fix
the **whole program** — F3 file viewing, copying, etc. still do not work;
sometimes a "name too long" popup appears, but mostly the program crashes
outright.

## Problem Statement

Across features 010–013 the application's *panel path* and *navigation*
became long-path capable (paths beyond the legacy 260-character `MAX_PATH`
limit), and individual crash sites were fixed one at a time — each confirmed
from the user's crash dumps. That approach has **not converged**: every fix
removed one overflowing fixed-size buffer only for the failure to reappear at
the next fixed-size buffer downstream (navigation → default-dir → copy
builder → viewer thread → shell drop-target …). The user still cannot view
(F3), copy, or otherwise operate on files inside a long-path directory: a
"name too long" popup sometimes appears, but **mostly the program crashes
entirely**.

The root cause is a **program-wide code class**, not a handful of sites: a
census of the core source found **~764 fixed-size `MAX_PATH`/`2*MAX_PATH`
character buffers**, ~90+ of them directly receiving a full path, spread
across ~30 files. Any one of them, when handed a path longer than its fixed
size, either overruns the stack (the app crashes with a stack-buffer-overrun)
or is rejected by an internal length gate (the "name too long" popup). Because
these buffers sit on nearly every operation's path-composition route, the
defect surfaces wherever the user goes next — which is why point-fixing does
not work.

This feature abandons the reactive, dump-driven point-fix loop and instead
**systematically eliminates the entire class**: enumerate every fixed-size
path buffer in the application, classify each, and fix or safely bound all of
them in one coordinated pass — so that **every common operation on files and
directories inside a long-path location either works or degrades with a
clear, correct message, and nothing crashes** — verified across the whole
operation surface rather than discovered by the next crash.

## Clarifications

### Session 2026-07-18

- Q: Autonomous execution, and how is a whole-program change verified when
  interactive UI automation is unavailable? → A: Fully autonomous (per the
  established pattern across features 011–013). Verification combines: a clean
  Debug **and Release** build; a static-exhaustion audit proving every
  fixed-size path buffer is classified and resolved; crash-dump forensic
  re-confirmation if any new dump appears; and command-line-driven navigation
  where the environment permits. A final human walkthrough of the operation
  matrix remains a follow-up the user performs.
- Q: What is the scope boundary of "whole program"? → A: The entire core
  application's file/directory/path handling — every panel operation (view,
  edit, open, copy, move, rename, delete, create, attributes, properties,
  calculate size, change case, convert, pack/unpack), clipboard and
  drag-and-drop, shell integration, dialogs, configuration/persistence,
  change-notification, history, and status/title chrome. Bundled enabled
  plugins are covered for their own file-operation UI in a documented
  secondary pass; third-party plugins are out of scope.
- Q: When a path genuinely cannot be handled (an external Windows API imposes
  the 260 limit), what must the user see? → A: A **clear, correct, bounded
  message** that names the real limitation — never a crash, and never a
  spurious "too long" for a path the application itself can handle. The set of
  operations that must degrade (rather than work) is limited to those bound by
  an external API and is documented explicitly.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - No operation on a long-path item ever crashes (Priority: P1)

A user works entirely inside a directory whose path exceeds the legacy limit
and performs the full range of everyday operations — view (F3), edit (F4),
open (Enter), copy (F5), move (F6), rename (F2), delete (Del), create
directory (F7), change attributes, properties (Alt+Enter), clipboard
Copy/Cut/Paste (Ctrl+C/X/V), drag-and-drop, calculate occupied space, pack
(Alt+F5) and unpack (Alt+F6). No operation crashes the application.

**Why this priority**: The reported, dominant failure mode ("mostly the
program crashes entirely"). A file manager that crashes on ordinary
operations is unusable for long paths.

**Independent Test**: Walk the documented operation matrix on the long-path
test tree (ASCII 291-char and Unicode ~540-byte directories) and confirm the
process is alive after every operation.

**Acceptance Scenarios**:

1. **Given** a file inside a long-path directory, **When** the user performs
   any operation in the matrix, **Then** the application keeps running.
2. **Given** a directory (recursive) inside a long path, **When** the user
   copies, moves, deletes, or sizes it, **Then** no stack is overrun at any
   recursion depth.
3. **Given** a Unicode long path (~540 UTF-8 bytes, exceeding the 520-byte
   `2*MAX_PATH` buffers), **When** it is a source or target of any operation,
   **Then** no buffer is overrun.

---

### User Story 2 - Long-path operations work, and only genuine limits degrade — clearly (Priority: P1)

For every operation the application performs itself (internal viewer/editor,
its own copy/move/delete/rename/create engine, its own dialogs and chrome),
long paths *work*. Only operations bound by an external Windows facility (the
shell verb/command invocation, shortcut resolution, the Recycle Bin, the shell
"New" menu) may decline — and when they do, they show a clear, correct message
that names the real limitation; they never show a spurious "too long" popup
for a path the application can handle, and never crash.

**Why this priority**: The second reported symptom ("sometimes a too-long
popup"). Users must be able to actually use long paths, and where they truly
cannot, understand why.

**Independent Test**: On the long-path test tree, confirm F3/F4/copy/move/
rename/create/size complete successfully; confirm the documented external-limit
operations show a bounded, accurate message rather than a crash or a false
rejection.

**Acceptance Scenarios**:

1. **Given** a file inside a long-path directory, **When** the user presses
   F3, **Then** the internal viewer shows its content (no "too long" popup, no
   crash).
2. **Given** files inside a long-path directory, **When** the user copies them
   to another location and back, **Then** the copies succeed with matching
   bytes.
3. **Given** an operation bound by an external API on a path beyond that API's
   limit, **When** invoked, **Then** a clear bounded message is shown and the
   application keeps running.
4. **Given** a path the application can handle, **When** any self-performed
   operation is invoked, **Then** it is **not** rejected with a "too long"
   message.

---

### User Story 3 - The fix is provably exhaustive, not another point patch (Priority: P2)

Every fixed-size path buffer in the application is enumerated, classified
(handles a full path / holds only a component name / bound by an external API),
and resolved (widened, made dynamic, or safely bounded) — with the result
documented so that the overflow-and-rejection class is provably eliminated
program-wide, and future regressions can be checked against the same
enumeration.

**Why this priority**: The user's explicit ask ("fix the whole program") and
the lesson of features 011–013 that per-site fixes do not converge.

**Independent Test**: The audit inventory lists every fixed-size path buffer
with a verdict; a repeatable static check finds zero unresolved
crash-capable sites; the operation matrix passes with no crash.

**Acceptance Scenarios**:

1. **Given** the audit inventory, **When** the static exhaustion check is
   run, **Then** no fixed-size buffer that can receive a full path remains
   unclassified or unresolved.
2. **Given** the inventory, **When** each subsystem is exercised on the
   long-path test set, **Then** none crashes or truncates a path it should
   handle.

---

### Edge Cases

- Paths at every threshold: just over 260, over the 520-byte `2*MAX_PATH`
  buffers (Unicode), and near the extended-length maximum.
- Recursive operations (directory copy/move/delete/size) at depth — a fix
  that puts a large buffer on each recursion frame must not itself overflow
  the stack.
- Component-name-only buffers (a single file/dir name ≤ 255) must stay bounded
  and must not be needlessly enlarged.
- Operations bound by external APIs (shell verbs, `IShellLink`, Recycle Bin,
  shell "New", shell-extension IPC) must degrade with a correct message.
- Both source and target long simultaneously (copy/move/paste between two long
  locations).
- Persistence round-trip: a long last-path saved and restored across restart;
  long paths in history and change-notification.
- ASCII and Unicode variants of every case (Unicode multiplies byte length up
  to 3×).
- No file data may be lost by any operation (the file-operation engine already
  uses dynamic path strings; this must remain true).

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: No fixed-size path buffer anywhere in the application may be
  overrun by a path the operating system can host; every buffer that can
  receive a full path MUST hold the full path (up to the platform's
  extended-length limit) or be provably bounded so it truncates safely rather
  than overrunning.
- **FR-002**: Every operation the application performs with its own code
  (internal viewer, editor, open, copy, move, rename, delete, create
  directory, change attributes, calculate size, change case, convert, its own
  dialogs, chrome, history, and change-notification) MUST work for long-path
  items up to the platform limit.
- **FR-003**: Operations bound by an external Windows facility (shell verb
  invocation, shortcut resolution, Recycle Bin, shell "New", shell-extension
  IPC) MAY decline for paths beyond that facility's limit, but MUST do so with
  a clear, correct, bounded message and MUST NOT crash; the complete list of
  such operations MUST be documented.
- **FR-004**: The application MUST NOT reject with a "name too long" message
  any path that it is itself capable of handling (no spurious internal limit).
- **FR-005**: Recursive operations MUST remain within safe stack bounds at any
  directory depth (large per-frame path buffers must be avoided on recursive
  routes).
- **FR-006**: Component-name-only buffers (holding a single file/directory
  name) MUST remain correctly bounded and need not be enlarged.
- **FR-007**: No operation may lose file data or act on a truncated (wrong)
  path; a move MUST NOT delete a source that was not successfully copied.
- **FR-008**: The team MUST produce a documented, program-wide audit
  enumerating every fixed-size path buffer with a per-buffer verdict (handles
  full path / component-only / external-API-bound) and resolution, and a
  repeatable check that proves no crash-capable site remains, covering the
  core application and the bundled enabled plugins' own file-operation UI.
- **FR-009**: Behavior for operations on items within the legacy limit MUST
  remain unchanged (zero regressions in view, edit, copy, move, rename,
  delete, create, clipboard, drag, pack/unpack, and chrome for normal paths).

### Key Entities

- **Fixed-size path buffer**: any fixed-length character buffer that a full
  path can flow into; the program-wide population (~764 in the core) whose
  overflow or rejection is the defect class.
- **Operation matrix**: the enumerated set of user operations × path lengths
  (ASCII/Unicode, legacy/long) exercised for verification.
- **Buffer audit inventory**: the documented enumeration of every fixed-size
  path buffer with its verdict and resolution — the FR-008 deliverable and the
  regression baseline.
- **External-limit operation set**: the documented, minimal set of operations
  that must degrade (not work) because an external API imposes the limit.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% of the operation matrix (view, edit, open, copy, move,
  rename, delete, create, attributes, properties, clipboard, drag, size,
  pack/unpack) executed on the long-path test tree (ASCII 291-char and Unicode
  ~540-byte) completes with the process alive.
- **SC-002**: 100% of the self-performed operations (internal viewer/editor
  and the application's own file engine and dialogs) succeed on long-path
  items, with copy/move byte-for-byte correct and no data loss.
- **SC-003**: Every operation that shows a "too long" message is on the
  documented external-limit list; zero self-performed operations show a
  spurious "too long" message.
- **SC-004**: The buffer audit inventory covers 100% of the enumerated
  fixed-size path buffers with an explicit verdict; a repeatable static check
  reports zero unresolved crash-capable sites.
- **SC-005**: A clean full Debug **and** Release build passes with no new
  warnings in the changed files; command-line-driven navigation across the
  long-path tree survives (no regression).
- **SC-006**: All operations on sub-260 paths behave exactly as before (zero
  regressions).

## Assumptions

- Builds on features 004/010/011/012/013: the panel path, navigation, the
  file-operation engine's dynamic path storage (no data loss), and the
  already-fixed sites remain in force; this feature completes the remaining
  program-wide population rather than re-doing them.
- The user runs the **Release** build; a stack-buffer overrun manifests as a
  hard `STATUS_STACK_BUFFER_OVERRUN` crash. Removing the overrun (not the
  guard) is the fix.
- The correct resolution per buffer is one of: widen to the extended-length
  size, make dynamic/heap-backed (required on recursive routes to avoid stack
  exhaustion), or keep bounded with a safe-degradation message (external-API
  and component-name buffers). "Replace every `MAX_PATH` with the large size"
  is explicitly NOT the approach.
- The long-path test tree (`%LOCALAPPDATA%\Temp\salamander-test\010\long-paths\`,
  ASCII and Unicode, boundary lengths) is the verification data set; regenerated
  if a prior operation displaced it.
- Interactive keystroke automation is unavailable in the autonomous
  environment; verification relies on build, static exhaustion, dump forensics,
  and command-line navigation, with a final human walkthrough as follow-up.
- The genuine external-API limits (shell verbs, `IShellLink`, Recycle Bin,
  shell "New", shell-extension IPC command length) remain accepted
  safe-degradation, consistent with features 012/013.
