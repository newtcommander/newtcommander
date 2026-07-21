# Feature Specification: Long-Path & Unicode File-Operation Stability Revision

**Feature Branch**: `027-longpath-fileops-stability`
**Created**: 2026-07-21
**Status**: Draft
**Input**: User description: "Cílem této úpravy je znovuprovedení celkové revize
zdrojového kódu s cílem zvýšení stability a rychlosti při práci se soubory —
především je nutné se zaměřit na správné souborové operace v adresářích s velmi
dlouhými názvy, resp. s dlouhými názvy a různými typy Unicode znaků. Čerpej
informace z již provedených úprav v minulých fázích vývoje, kde se tento problém
již několikrát řešil. Např. testovací adresáře:
`C:\Users\pavel\AppData\Local\Temp\salamander-test\010\long-paths\L1-dlouhy-nazev-ěščřžýáíé-…\L2-…\L3-…`
Zde jsou stále problémy s kopírováním souborů (především z těchto adresářů, ale
i do nich), a to především pomocí systémového příkazu Ctrl+C + Ctrl+V.
V některých případech ale kopírování pomocí F5 v programu již funguje. Proveď
tedy celkovou revizi možností, jak celé zlepšit — tj. především opravit,
stabilizovat atd. Při některých operacích kopírování souborů program úplně
spadnul."

## Problem Statement

Features 004 and 010–015 made the application's panel navigation, its own
copy/move engine, the viewer, rename, and display long-path and Unicode
capable, fixing crash sites confirmed from the user's Release crash dumps.
Feature 014 committed to a systematic whole-program elimination of the
fixed-size path-buffer class; its dump-confirmed crashes were fixed, but the
exhaustive program-wide audit (~30 files, ~764 fixed-size path buffers) was
left pending.

The user now reports the problem is **still not closed** on the established
long-path Unicode test tree
(`%LOCALAPPDATA%\Temp\salamander-test\010\long-paths\L1-…ěščřžýáíé…\L2-…\L3-…`):

1. **Copying files out of these directories — and into them — still fails**,
   most prominently via the **system clipboard (Ctrl+C followed by Ctrl+V)**.
2. **Some copy operations crash the application outright.**
3. In-program copy (F5) now works **in some cases**, i.e. gaps remain there
   too.

This feature is the requested **overall revision**: re-examine the complete
file-operation surface for long-path and Unicode correctness — building on
everything learned and fixed in features 004 and 010–015 — and close the class
for good, with two explicit goals: **stability** (no crashes, no silent
failures) and **speed** (operations in long-path directories must not be
noticeably slower than in ordinary ones).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Clipboard copy & paste works with long Unicode paths (Priority: P1)

A user browses into a deeply nested directory whose every level has a very
long, diacritics-heavy name (the established test tree), selects one or more
files or folders, presses Ctrl+C (or Ctrl+X), navigates elsewhere — an
ordinary directory, another long-path directory, or the other panel — and
presses Ctrl+V. The items are pasted completely and correctly. The same works
in the opposite direction: items copied from an ordinary directory paste
correctly **into** a long-path Unicode directory. Content copied to the
clipboard by the application is also usable by other programs (e.g. Windows
Explorer) to the extent those programs themselves support such paths.

**Why this priority**: This is the user's primary reported failure — the
system clipboard route (Ctrl+C + Ctrl+V) fails where the in-program F5 copy
already partially works, and it is the most common way users move files.

**Independent Test**: On the test tree, run the clipboard matrix — {copy, cut}
× {source: long-path dir, ordinary dir} × {target: long-path dir, ordinary
dir, other panel} — and verify every combination pastes byte-for-byte
identical files with names preserved exactly.

**Acceptance Scenarios**:

1. **Given** files selected inside the L3 long-path Unicode directory,
   **When** the user presses Ctrl+C and pastes with Ctrl+V into an ordinary
   directory, **Then** all items are copied byte-for-byte with their exact
   Unicode names, and the application keeps running.
2. **Given** files selected in an ordinary directory, **When** the user
   pastes them with Ctrl+V into the L3 long-path directory, **Then** all
   items arrive intact under their exact names.
3. **Given** files cut with Ctrl+X from a long-path directory, **When**
   pasted elsewhere, **Then** the items are moved — the source is removed
   only after the copy has fully succeeded.
4. **Given** a folder (recursive, itself containing long Unicode names)
   anywhere in the test tree, **When** copied via clipboard in either
   direction, **Then** the entire subtree arrives intact.
5. **Given** items placed on the clipboard from the long-path directory,
   **When** an external program that supports such paths pastes them,
   **Then** it receives valid data; where the external program itself cannot
   handle the path, the application is not the failing party and does not
   crash.

---

### User Story 2 - No file operation on the test tree ever crashes (Priority: P1)

A user performs any everyday file operation — copy, move, paste, drag-and-drop,
delete, rename, view, open, create, properties, calculate size — on items in
the long-path Unicode test tree, in either direction. The application never
terminates unexpectedly. Where something genuinely cannot be done, the user
sees a clear, accurate message and the application continues.

**Why this priority**: The user reports the application **crashed completely**
during some copy operations. Crashes are the most severe failure mode of a
file manager and destroy trust in every other fix.

**Independent Test**: Execute the full operation matrix on the test tree
(both directions, ASCII-long and Unicode-long variants, single files and
recursive folders) and confirm the process is alive after every step.

**Acceptance Scenarios**:

1. **Given** any item in the test tree, **When** any operation from the
   matrix is invoked, **Then** the application keeps running — success or a
   clear bounded error, never a crash.
2. **Given** an operation that previously crashed (clipboard-driven copy),
   **When** repeated on the same data, **Then** it completes or reports a
   clear error, and the process survives.
3. **Given** a genuinely impossible operation (an external facility with a
   hard path limit), **When** invoked, **Then** the message names the real
   limitation and never blames a path the application itself can handle.

---

### User Story 3 - In-program copy/move works in **all** cases (Priority: P2)

A user copies (F5) or moves (F6) files and folders between panels where one
or both sides are long-path Unicode directories. Every combination works —
not just "some cases" as today — including recursive folders, name
collisions with overwrite prompts, and cancellation mid-operation without
data corruption.

**Why this priority**: F5/F6 is the application's core competence; the user
confirms it partially works, so this closes the remaining gaps rather than
opening a new front.

**Independent Test**: Run the F5/F6 matrix — {file, recursive folder} ×
{long→normal, normal→long, long→long} × {no collision, collision-overwrite,
collision-skip, cancel midway} — and verify correct results and an intact
source in every cancel/skip case.

**Acceptance Scenarios**:

1. **Given** any source/target combination in the matrix, **When** the user
   copies or moves with F5/F6, **Then** the result is byte-for-byte correct
   with exact names, and no case fails that involves only the application's
   own engine.
2. **Given** a collision during such a copy, **When** the user chooses
   overwrite, skip, or cancel, **Then** the chosen outcome happens exactly,
   and cancelled/skipped items leave both source and target uncorrupted.

---

### User Story 4 - Long-path operations are fast (Priority: P3)

A user working in the long-path test tree experiences the same responsiveness
as in an ordinary directory: listing the directory, starting a copy, and the
copy throughput itself show no perceptible slowdown attributable to path
length or Unicode names.

**Why this priority**: Explicitly requested ("zvýšení stability **a
rychlosti**"), but secondary to correctness — a fast crash is still a crash.

**Independent Test**: Time the same operation set (list, copy N files of
fixed total size) in an ordinary directory and in the L3 long-path directory
on the same volume; compare.

**Acceptance Scenarios**:

1. **Given** an identical set of files in an ordinary and a long-path
   directory on the same volume, **When** each is copied to the same target,
   **Then** the long-path copy takes no more than marginally longer
   (see SC-005).
2. **Given** the revision's code changes, **When** operations run on
   ordinary sub-260 paths, **Then** they are no slower than before the
   revision.

---

### User Story 5 - The revision is provably complete, not another point fix (Priority: P3)

The overall revision finishes what feature 014 started: every fixed-size path
buffer and every path-handling route on the file-operation surface is
enumerated, classified, and resolved or explicitly documented as an external
limitation — recorded so the class is provably closed and future regressions
can be checked against the inventory.

**Why this priority**: The user explicitly asks for a "celková revize"
(overall revision) informed by the previous phases; features 011–014 proved
that per-crash point fixes do not converge. The pending exhaustive audit from
014 is the vehicle.

**Independent Test**: The audit inventory covers the complete enumerated
population with a verdict per site; a repeatable static check reports zero
unresolved crash-capable sites; the operation matrix passes end to end.

**Acceptance Scenarios**:

1. **Given** the audit inventory, **When** the static exhaustion check runs,
   **Then** no path-capable fixed-size buffer on the file-operation surface
   remains unclassified or unresolved.
2. **Given** the documented external-limitation list, **When** each listed
   operation is invoked on an over-limit path, **Then** it degrades with the
   documented message — and no operation outside the list degrades at all.

---

### Edge Cases

- Paths at every threshold: just over 260 characters, beyond the 520-byte
  double-buffer sizes (Unicode names roughly double or triple byte length —
  the test tree's `ěščřžýáíé` names are 2 bytes per character in UTF-8), and
  near the platform's extended-length maximum.
- Directory names ending in unusual characters (the test tree's levels end in
  `-`), names differing only by diacritics, and mixed-script names.
- Both source **and** target long simultaneously; source and target on the
  same volume vs. different volumes.
- Clipboard round-trips: copy in the application → paste in the application;
  copy in the application → paste in an external program; copy in an external
  program → paste in the application into a long-path target.
- Cut (Ctrl+X) semantics: source must never be removed unless the copy fully
  succeeded; a failed or cancelled paste leaves the source untouched.
- Recursive folders whose *relative* content paths are themselves long, so
  target paths exceed limits even when the target root is short.
- Collisions and prompts (overwrite / skip / rename) with long Unicode names
  displayed correctly in the prompt itself.
- Cancellation mid-copy: no partial file left behind unnoticed, no corrupted
  target, no lost source.
- Drag-and-drop as the sibling of clipboard transfer — same data, same
  guarantees, no crash.
- Operations invoked while a previous long-path operation is still running.
- Sub-260 everyday paths: everything must behave exactly as before the
  revision (zero regressions).

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Clipboard Copy and Cut of items in long-path and/or
  Unicode-named directories MUST place complete, valid data on the system
  clipboard, and Paste inside the application MUST reproduce the items
  byte-for-byte with names preserved exactly — for every combination of
  ordinary and long-path source and target.
- **FR-002**: Paste (and drag-and-drop, its equivalent transfer route) INTO a
  long-path Unicode directory MUST work for content originating inside or
  outside the application, up to the platform's extended-length limit.
- **FR-003**: No file operation on long-path or Unicode-named items —
  clipboard transfer, drag-and-drop, in-program copy/move, delete, rename,
  view, open, create, properties, size calculation — may terminate the
  application. Every failure MUST surface as a clear, bounded, accurate
  message.
- **FR-004**: In-program copy (F5) and move (F6) MUST succeed for **all**
  source/target combinations involving only the application's own engine —
  including recursive folders, collisions, and cancellation — closing the
  currently remaining partial failures.
- **FR-005**: A move or cut-paste MUST NOT remove any source item that was
  not fully and verifiably transferred; cancellation or failure mid-operation
  MUST leave no corrupted or silently partial target and an intact source.
- **FR-006**: Unicode names MUST survive every operation exactly (no `?`
  substitution, no mojibake, no silent renaming), in file-system results and
  in every prompt, progress display, and error message shown along the way.
- **FR-007**: Only operations bound by a genuine external-facility limit MAY
  decline long paths; each MUST show a clear message naming the real
  limitation, the complete set MUST be documented, and no self-performed
  operation may appear on it (no spurious "too long" rejections) —
  consistent with the lists established in features 012–014.
- **FR-008**: The revision MUST complete the program-wide audit begun in
  feature 014: every fixed-size path buffer and path-composition route on the
  file-operation surface enumerated, classified, and resolved, with a
  repeatable check proving zero unresolved crash-capable sites remain.
- **FR-009**: File operations in long-path directories MUST NOT be
  meaningfully slower than the same operations on ordinary paths, and the
  revision MUST NOT slow down any operation on ordinary sub-260 paths.
- **FR-010**: All behavior for sub-260 paths MUST remain unchanged (zero
  functional regressions across the whole operation surface).

### Key Entities

- **Long-path Unicode test tree**: the established verification data set at
  `%LOCALAPPDATA%\Temp\salamander-test\010\long-paths\` (levels
  `L1-…ěščřžýáíé…`, `L2-…`, `L3-…`), regenerated if displaced; extended with
  boundary-length and edge-case names as needed.
- **Clipboard/drag transfer matrix**: {copy, cut, drag} × {source: ordinary,
  long, long-Unicode} × {target: ordinary, long, long-Unicode, external
  program} — the User Story 1 verification set.
- **Operation matrix**: the full set of user operations × path classes
  exercised for the no-crash guarantee (User Story 2), extending feature
  014's matrix.
- **Buffer/route audit inventory**: the completed enumeration of fixed-size
  path buffers and path-handling routes with per-site verdicts — the FR-008
  deliverable and the regression baseline.
- **External-limitation list**: the documented, minimal set of operations
  that degrade because an external facility imposes the limit (FR-007).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% of the clipboard/drag transfer matrix completes with the
  process alive and byte-for-byte-identical results with exact names; the
  previously failing Ctrl+C → Ctrl+V routes (out of and into the test tree)
  demonstrably succeed.
- **SC-002**: Zero crashes across the full operation matrix on the test tree
  — every cell ends in success or a clear bounded message.
- **SC-003**: 100% of in-program F5/F6 combinations (including recursion,
  collisions, cancellation) succeed; no "works in some cases" residue.
- **SC-004**: The audit inventory covers 100% of the enumerated population
  with explicit verdicts, and the repeatable static check reports zero
  unresolved crash-capable sites.
- **SC-005**: Copying a fixed file set inside the long-path tree takes no
  more than 10% longer than copying the identical set between ordinary
  directories on the same volume; operations on ordinary paths are no slower
  than before the revision.
- **SC-006**: Every operation that declines a long path is on the documented
  external-limitation list; zero spurious "too long" messages for paths the
  application handles itself.
- **SC-007**: Zero regressions on sub-260 paths across the whole operation
  surface.

## Assumptions

- Builds directly on features 004 and 010–015: panel navigation, the
  dynamic-path copy engine, viewer, rename, and display fixes remain in
  force; this feature revises and completes rather than re-does them. Prior
  specs, research notes, and the feature-014 audit material are the primary
  information sources, as the user requested.
- The user runs the **Release** build; crash symptoms there are hard process
  terminations. Release crash-dump forensics (the established WER + PDB
  symbolication method from features 011–014) is available if new crashes
  appear during verification.
- "Speed" is interpreted as: remove performance pathologies discovered during
  the revision and guarantee long-path operations are not meaningfully slower
  than ordinary ones (SC-005) — not as a general performance-optimization
  project beyond the file-operation surface.
- Clipboard interoperability with external programs is required on the
  producing side (the application must always hand over valid data); whether
  a given external program can *consume* paths beyond its own limits is that
  program's constraint and is documented, not worked around.
- The work proceeds autonomously per the established pattern: verification
  combines clean Debug and Release builds, the static exhaustion audit, the
  scripted operation/transfer matrices where the environment permits, and
  crash-dump forensics; a final interactive walkthrough on the test tree
  remains the user's follow-up.
- The long-path test tree is the canonical data set and will be regenerated
  or extended (boundary lengths, additional edge-case names) as part of
  verification.
