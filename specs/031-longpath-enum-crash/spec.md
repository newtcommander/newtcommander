# Feature Specification: Directory-Listing Crash on Long Multi-Byte Names — Review & Regression Protection

**Feature Branch**: `031-longpath-enum-crash`
**Created**: 2026-07-23
**Status**: Draft
**Input**: User description: "Cílem aktuální opravy je revize a oprava pracování
se soubory a adresáři na základě již historicky provedených úprav ohledně
kódování a podpory dlouhých názvů. Nyní se opět stává, že celý program spadne.
V adresáři D:\Temp je podadrsář
\"ýášřtščýáíf buaweýáh čáíhšáífšfhčíáéfšh dnfýášřtščýáíf buaweýáh
čáíhšáífšfhčíáéfšh dnfýášřtščýáíf buaweýáh čáíhšáífšfhčíáéfšh dnfýášřtščýáíf
buaweýáh čáíhšáífšfhčíáéfšh dnfýášřtščýáíf buaweýáh čáíhšáífšfhčíáéfšh dnf\",
v okamžiku, kdy se znažím vstoupit do adresáře D:\Temp, program se zasekne a
spadne. Toto bude nejspíš spojené s úpravami ohledně práce s kódováním a
dlouhými názvy. Proveď detailní revizi, oprav to a zároveň během revize
zajisti, aby se nevracely tyto regresní chyby."

## Problem Statement

Features 004, 005, 010–015 and 027 progressively made panel navigation, file
operations, the viewer, and rename long-path and Unicode capable. The user now
reports the crash class has **returned**: merely **entering `D:\Temp`** — a
directory that *contains* a subdirectory with a very long, diacritics-heavy
name — makes the application freeze and then crash. The user never even enters
the long-named subdirectory itself; listing its *parent* is enough.

Measured facts about the reported reproduction case (the directory exists on
the user's machine and was verified during spec preparation):

| Property | Value |
|----------|-------|
| Subdirectory name length | 215 characters |
| Full path length (`D:\Temp\<name>`) | 223 characters |
| Name size in UTF-8 | 330 bytes |
| Name size in Windows-1250 (ANSI) | 215 bytes |

The critical insight: **this is not a "long path" by the classic definition.**
At 223 characters the full path is *below* the legacy 260-character Windows
limit, and the name component is below the 255-character component limit. The
name only becomes "too long" (330 bytes > 260) when it is converted to a
multi-byte encoding such as UTF-8, because every Czech diacritic character
expands to two bytes. Historic long-path testing was driven by *character
counts*, so this class — ordinary-length paths whose *byte* representation
overflows legacy-sized storage — escaped coverage and has now caused a
user-facing crash.

The user asks for three things: (1) fix the crash, (2) perform a detailed
review of file/directory handling with respect to the historic encoding and
long-name changes, and (3) put protection in place so this regression class
cannot silently return.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Browsing a folder with very long names never crashes (Priority: P1)

A user opens `D:\Temp` (or any directory) that contains one or more entries
whose names are very long and rich in accented or otherwise multi-byte
characters. The panel lists all entries promptly and completely, each name is
shown exactly as it is on disk, and the application stays responsive — no
freeze, no crash. This holds no matter how the user arrives: typing the path,
clicking through parents, panel refresh, or returning from a subdirectory.

**Why this priority**: This is the reported defect — a hard crash triggered by
simply displaying a folder. A file manager that dies on listing a folder is
unusable; every other capability depends on this working.

**Independent Test**: Create a directory containing a subdirectory named with
215+ diacritic characters (the user's exact reproduction name), enter the
parent directory in the panel, and confirm the listing appears completely and
the application keeps running and responding.

**Acceptance Scenarios**:

1. **Given** `D:\Temp` contains the reported 215-character diacritics-heavy
   subdirectory, **When** the user navigates the panel into `D:\Temp`,
   **Then** all entries including that subdirectory are listed with exact
   names and the application does not freeze or crash.
2. **Given** the same directory is already displayed, **When** the user
   forces a refresh or the folder content changes in the background,
   **Then** the listing updates correctly and the application remains stable.
3. **Given** a directory containing an entry whose name is at the maximum the
   file system allows (255 characters) consisting entirely of multi-byte
   characters, **When** the user lists it, **Then** the entry appears with its
   exact full name and no instability occurs.

---

### User Story 2 - All everyday operations work on such entries (Priority: P2)

The user works normally with the long-named entry: enters the subdirectory,
walks back up, creates files and folders inside it, renames it, copies and
moves it (both with in-program commands and via the system clipboard), views
properties and sizes, deletes it. Every operation either completes correctly
or — where the platform itself refuses — fails with a clear, understandable
error message. The application never crashes, never hangs, and never silently
acts on a truncated or corrupted version of the name.

**Why this priority**: Fixing only the listing would leave the user one
keystroke away from the next crash. The user explicitly asked for a review of
file and directory handling as a whole, informed by the historic encoding and
long-name work.

**Independent Test**: On a generated test tree containing boundary-length
multi-byte names, execute the operation matrix — enter/up, create, rename,
copy (both directions, in-program and clipboard), move, delete, properties —
and verify each completes or fails gracefully with the name always intact.

**Acceptance Scenarios**:

1. **Given** the long-named subdirectory, **When** the user enters it and
   creates, renames, and deletes files inside, **Then** all operations
   succeed with names preserved exactly.
2. **Given** the long-named subdirectory is selected, **When** the user copies
   it into another directory (in-program copy and clipboard paste), **Then**
   the copy arrives complete with the exact name, and the application remains
   stable.
3. **Given** an operation the file system genuinely cannot perform (e.g. the
   resulting name would exceed the file-system limit), **When** the user
   attempts it, **Then** a clear error message is shown, nothing is silently
   truncated, and the application continues running.

---

### User Story 3 - The defect class is fenced off against regressions (Priority: P3)

A developer (or the user, months later) can trust that this class of defect —
names whose byte-size in some encoding exceeds legacy limits even though their
character count does not — stays fixed. Automated checks covering the class
run as part of the standard test suite, so any future change that reintroduces
the problem is caught before release, not by a crash on the user's machine.
The review performed for this feature is recorded: every examined hazard is
either fixed or explicitly justified as safe.

**Why this priority**: The user's request is explicit: "zajisti, aby se
nevracely tyto regresní chyby" — this crash class has already been fixed and
has come back once; without automated fencing it will come back again.

**Independent Test**: Run the standard automated test suite and confirm it
contains checks that fail if the fix is reverted; inspect the recorded review
findings for completeness.

**Acceptance Scenarios**:

1. **Given** the standard automated test suite, **When** it runs after this
   feature, **Then** it includes checks exercising names at boundary lengths
   whose multi-byte representation exceeds legacy limits, and all pass.
2. **Given** the fix is experimentally reverted, **When** the suite runs,
   **Then** at least one of the new checks fails (the tests demonstrably
   guard the defect).
3. **Given** the review is complete, **When** its findings are inspected,
   **Then** every identified hazard in file/directory-name handling is marked
   fixed or justified, with none left unaddressed silently.

---

### Edge Cases

- Name exactly at the file-system component limit (255 characters) composed
  entirely of 2-byte, 3-byte, or 4-byte characters (Czech diacritics, CJK,
  emoji / surrogate pairs) — byte sizes up to ~1020 bytes for a single name.
- Path whose *character* count is just under the legacy 260 limit while its
  *byte* count in a multi-byte encoding is far above it (the reported case),
  and the mirror case: character count above 260 handled by the existing
  long-path support.
- A directory containing *many* such entries at once (stress on listing,
  sorting, and column layout).
- Such entries on non-NTFS media (exFAT/FAT32) and on network shares, where
  limits and behaviors differ.
- Background change notifications for a displayed directory in which a
  long-multi-byte-named entry is created, renamed, or removed by another
  program.
- Displaying such names in dependent UI surfaces (title bar, status line,
  quick search, command line, dialogs) without truncation-induced instability.
- Plugin-provided listings (archives, network plugins) encountering such
  names: full support is not required here, but they must not crash the host.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Listing any directory containing entries with names of any
  length and character content legal on the underlying file system MUST
  complete without crash, freeze, or omission, displaying every entry under
  its exact name.
- **FR-002**: Panel interactions on such entries (enter, go up, refresh,
  select, quick search, sort) MUST work correctly and never destabilize the
  application.
- **FR-003**: File operations on such entries — create, rename, copy and move
  (both in-program and via the system clipboard, in both directions), delete,
  properties/size queries — MUST either complete correctly with names
  preserved exactly, or fail with a clear user-facing error message.
- **FR-004**: The application MUST never act on a silently truncated or
  otherwise corrupted form of a name; if a name cannot be represented in a
  context, the operation MUST be refused with an explanation rather than
  proceed on wrong data.
- **FR-005**: A systematic review of the file/directory-name handling surface
  — every place a name is converted between encodings or held in fixed-size
  storage — MUST be performed in light of the historic changes (features 004,
  005, 010–015, 027); each confirmed hazard MUST be fixed or explicitly
  recorded as justified-safe, and the findings MUST be recorded in the
  feature's documentation.
- **FR-006**: Automated regression tests MUST cover the defect class:
  boundary name lengths combined with multi-byte character expansion —
  specifically including names whose byte representation exceeds legacy
  limits while their character count does not — and MUST run as part of the
  standard test suite.
- **FR-007**: All existing automated tests MUST continue to pass; the fix
  MUST NOT regress previously delivered long-path, Unicode, or file-operation
  behavior.
- **FR-008**: Listing and operating on directories containing such entries
  MUST NOT be perceptibly slower than on directories with ordinary names.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Entering the user's reported reproduction directory (`D:\Temp`
  containing the 215-character diacritics subdirectory) succeeds in 100 of
  100 attempts with zero crashes or freezes, and the listing appears within
  1 second on a local disk.
- **SC-002**: The full operation matrix (list, enter, create, rename, copy
  both directions including clipboard, move, delete, properties) on names at
  boundary lengths completes with 0 crashes; every platform-refused operation
  produces a user-visible error message and never a truncated result.
- **SC-003**: The standard automated test suite contains checks for the
  multi-byte-expansion defect class, they pass on the fixed build, and at
  least one fails when the fix is reverted.
- **SC-004**: 100% of pre-existing automated tests still pass after the
  change.
- **SC-005**: The recorded review lists every examined name-handling hazard
  with a resolution (fixed / justified-safe); zero hazards are left
  unclassified.

## Assumptions

- The reproduction directory on `D:\Temp` remains available for manual
  verification; automated tests will generate equivalent directory trees
  themselves so they do not depend on the user's machine state.
- The maximum name component the target file systems allow is 255 UTF-16
  characters (NTFS); tests treat that as the upper boundary.
- The application already carries the long-path and Unicode groundwork from
  features 004, 005, 010–015 and 027; this feature reviews and hardens that
  work rather than re-introducing it.
- Scope centers on the core file panels and the application's own file
  operations. Plugin-provided listings are in scope only to the extent that
  they must not crash the host application; full long-multi-byte-name support
  inside individual plugins is out of scope.
- The primary target platform is Windows 11 on NTFS, consistent with the
  project's platform commitment; non-NTFS and network locations are covered
  as robustness (no crash, graceful errors), not as feature parity.
