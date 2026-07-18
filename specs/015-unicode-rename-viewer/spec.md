# Feature Specification: Unicode in the Rename Field and the Internal Viewer

**Feature Branch**: `015-unicode-rename-viewer`
**Created**: 2026-07-18
**Status**: Draft
**Input**: User: (1) F2 rename shows `?` for special characters — e.g. the test
dir `emoji-🙂-dir` displays correctly in the panel but the F2 rename field
shows `emoji-??-dir`, and `Тест-Ελλάδα-测试` shows `????-??????-??`; deeply
analyze and fix. (2) In the same change, implement `./features/viewer.md` —
the F3 viewer must correctly detect and display text encodings (UTF-8 with/without
BOM, UTF-16 LE/BE, Windows-1250, ISO-8859-2, ASCII), render real Unicode
(Czech diacritics, other scripts, multi-byte and beyond-BMP characters),
show the actually-used encoding, and let the user switch encoding manually.

## Problem Statement

Two surfaces still render text through the legacy single-byte / ANSI path even
though file names and file content can be full Unicode (UTF-8 internally since
feature 004):

1. **F2 Rename field** — the Rename dialog (and the sibling Copy/Move/Edit-New
   dialogs) is created as an **ANSI window**. Its edit/combo controls therefore
   cannot hold characters outside the system ANSI code page: when the UTF-8
   name is placed into the field (via the wide `SetWindowTextW` path added in
   features 004/005) the system lossily converts it through the ANSI code page,
   so every non-ANSI character becomes a literal `?` (emoji → `??` because the
   emoji is a surrogate pair; Cyrillic/Greek/CJK letters each → `?`). The panel
   shows the name correctly because the panel window is already Unicode; the
   dialog was never converted.

2. **F3 Internal Viewer** — the viewer reads the file as raw bytes and paints
   them one byte per fixed column through the ANSI text API, optionally applying
   a 256-entry byte→byte code table. It has **no** notion of UTF-8, UTF-16, or a
   BOM. A UTF-8 file is therefore interpreted as single-byte (ISO-8859-2 /
   Windows-1250-like), so Czech diacritics and any multi-byte character are shown
   garbled, and the encoding named in the title can be a guess that does not
   match what was actually drawn.

Both are the same underlying defect class as the earlier long-path/encoding
work: text that is Unicode internally is squeezed back through an ANSI boundary
at the point it reaches the user.

## Clarifications

### Session 2026-07-18

- Q: Autonomous scope and verification (headless environment)? → A: Fully
  autonomous per the established pattern (features 011–014). Verification =
  clean Debug **and** Release build; static analysis of the render/transfer
  paths; encoding-detection unit reasoning against the generated test files; a
  final human interactive walkthrough (the environment cannot drive
  keystrokes). Generated test files (per `viewer.md`) are the viewer data set.
- Q: How much of the dialog layer is in scope for the `?` fix? → A: The
  Rename field is the reported case; the fix targets the shared Copy/Move/
  Rename/Edit-New dialog family (all display or edit UTF-8 file names through
  the same ANSI-window defect), because they share one class and one code path.
  Other dialogs are out of scope unless they reuse that class.
- Q: Viewer — replace the byte model wholesale, or decode-for-display? → A:
  Preserve the byte-offset seek/streaming model (needed for large files,
  search, and long lines) and add a decode-to-Unicode layer for rendering and
  column mapping. The active encoding is detected on open (BOM → UTF-8
  validation → legacy fallback) and is user-switchable; rendering uses the wide
  text API. Existing single-byte code tables remain available.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Rename field shows the real name (Priority: P1)

A user presses F2 on a file or directory whose name contains non-ANSI
characters (emoji, Cyrillic, Greek, CJK, or any Unicode letter). The rename
field shows the name exactly as the panel shows it — never `?` placeholders —
and the user can edit and confirm it.

**Why this priority**: Directly reported; a rename field that can't show the
name it is renaming is unusable for those files and risks the user
accidentally renaming to a `?`-mangled name.

**Independent Test**: F2 on `emoji-🙂-dir` and `Тест-Ελλάδα-测试` in the test
tree; the field shows the exact name; confirming without change leaves the name
untouched on disk.

**Acceptance Scenarios**:

1. **Given** a file/dir with non-ANSI characters, **When** the user presses F2,
   **Then** the rename field shows the exact name (no `?`).
2. **Given** the rename field with such a name, **When** the user confirms
   without editing, **Then** the on-disk name is unchanged.
3. **Given** the rename field, **When** the user types/pastes non-ANSI
   characters and confirms, **Then** the file is renamed to exactly those
   characters.
4. **Given** the Copy/Move target dialog and Edit-New-File dialog, **When** a
   path or name with non-ANSI characters is shown or entered, **Then** it is
   shown/stored correctly (no `?`).

---

### User Story 2 - Viewer detects and displays text encodings correctly (Priority: P1)

A user opens a text file with F3. The viewer determines the encoding (BOM
first, then UTF-8 validation, then a legacy fallback), decodes the content to
Unicode, and renders it correctly — including Czech diacritics, other European
scripts, and multi-byte/beyond-BMP characters. The title/status shows the
encoding actually used.

**Why this priority**: The second explicit request (`viewer.md`); the current
viewer garbles every UTF-8 file with non-ASCII content.

**Independent Test**: Open the generated reference text (`Příliš žluťoučký kůň
úpěl ďábelské ódy.` plus other-script and beyond-BMP samples) in each variant
(UTF-8, UTF-8+BOM, UTF-16 LE, UTF-16 BE, Windows-1250, ISO-8859-2, ASCII) and
confirm correct rendering and the correct reported encoding.

**Acceptance Scenarios**:

1. **Given** a UTF-8 file without BOM containing Czech diacritics, **When**
   opened in the viewer, **Then** it renders correctly and is reported as UTF-8.
2. **Given** a UTF-8 file with BOM, **When** opened, **Then** it is recognized
   by the BOM and renders correctly.
3. **Given** UTF-16 LE and UTF-16 BE files, **When** opened, **Then** they
   render correctly.
4. **Given** a Windows-1250 or ISO-8859-2 file with Czech diacritics, **When**
   opened (or switched to that encoding), **Then** it renders correctly.
5. **Given** any open file, **When** the user switches the encoding manually,
   **Then** the text re-renders immediately, the reported encoding updates, and
   the file on disk is unchanged.
6. **Given** the reported encoding, **When** the file was decoded, **Then** the
   reported encoding equals the one actually used to decode (never a discarded
   preliminary guess).

---

### User Story 3 - No regressions in existing viewing/renaming (Priority: P2)

Existing behavior is preserved: ASCII files view identically; binary files are
still detected and shown in hex (not auto-forced to text); viewer search,
scrolling, navigation, very long lines, large files, and existing keyboard
shortcuts keep working; renaming ASCII names behaves exactly as before.

**Why this priority**: The change touches the core render/transfer paths; it
must not degrade the common cases or performance.

**Independent Test**: Open a large file, a binary file, and an ASCII file;
search, scroll, and navigate; rename an ASCII file — all behave as before.

**Acceptance Scenarios**:

1. **Given** a binary file, **When** opened, **Then** it is shown as hex, not
   auto-interpreted as text.
2. **Given** a large file, **When** opened, **Then** open time is not
   significantly worse and no full extra in-memory copy is created beyond what
   the current streaming model uses.
3. **Given** viewer search/scroll/long-line handling, **When** used on decoded
   text, **Then** they work as before.
4. **Given** an ASCII file name, **When** renamed, **Then** behavior is
   identical to before.

---

### Edge Cases

- Emoji / beyond-BMP characters (surrogate pairs) in names and in file content.
- A name that is valid UTF-8 vs. one that is not (transitional/foreign bytes):
  the not-valid case must degrade without crashing and without silent data loss.
- Mixed ASCII + Unicode content; very long lines containing multi-byte
  characters; a multi-byte sequence split across the viewer's sliding-buffer
  boundary (must not corrupt or misreport).
- UTF-16 with an odd byte count / a lone surrogate; invalid UTF-8 sequences.
- A file that is valid UTF-8 by luck but is actually binary (binary detection
  must still win where appropriate).
- Manual encoding switch back and forth; switching on a huge file.
- Rename confirm of a canonically-equivalent (NFC/NFD) name must preserve the
  stored form (existing feature-005 behavior must be kept).

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The Rename field MUST display a file/dir name with any Unicode
  characters exactly as stored (as the panel shows it), never substituting `?`.
- **FR-002**: The user MUST be able to type/paste and commit non-ANSI
  characters in the Rename field, producing exactly those characters on disk;
  committing an unchanged name MUST NOT alter the on-disk name (including
  NFC/NFD-equivalent names, per feature 005).
- **FR-003**: The Copy/Move target dialog and the Edit-New-File dialog (sharing
  the same dialog class/path) MUST display and accept Unicode paths/names
  without `?` substitution.
- **FR-004**: On opening a text file, the Viewer MUST determine the encoding in
  this order: (1) BOM if present; (2) else validate whether the content is
  valid UTF-8 and if so use UTF-8; (3) else it MAY apply legacy encoding
  detection; (4) else use a defined default encoding, with manual override
  available.
- **FR-005**: The Viewer MUST support at least UTF-8, UTF-8+BOM, UTF-16 LE,
  UTF-16 BE, Windows-1250, and ISO-8859-2, and MUST NOT remove any encoding it
  already supports.
- **FR-006**: The Viewer MUST convert decoded content to the application's
  internal Unicode form and render it with the Unicode text path; it MUST NOT
  paint raw file bytes through the system ANSI character set.
- **FR-007**: The Viewer MUST report (title and/or status) the encoding
  actually used to decode the displayed content, not a discarded preliminary
  guess.
- **FR-008**: The user MUST be able to change the encoding of the open file
  without closing it; on change the Viewer MUST re-decode the original bytes,
  update the display, and update the reported encoding; the change MUST NOT
  modify the file on disk.
- **FR-009**: Binary detection MUST remain effective — obviously binary files
  MUST NOT be auto-shown as text; the text/binary decision MUST cooperate with
  the new encoding detection.
- **FR-010**: The change MUST preserve viewer search, selection, scrolling and
  navigation, very-long-line handling, large-file opening, and existing
  keyboard shortcuts; encoding detection MUST NOT create an unnecessary full
  in-memory copy where streaming/mapped reading exists, and MUST not scan the
  whole of a very large file merely to detect (a bounded sample MAY be used,
  but later bytes MUST NOT cause inconsistent display).
- **FR-011**: Both the Rename/dialog change and the Viewer change MUST behave
  identically to before for pure-ASCII names and files (zero regressions).
- **FR-012**: The implementation SHOULD ship reference test files per
  `viewer.md` (the reference Czech sentence in each listed encoding plus
  other-language and beyond-BMP samples, invalid-UTF-8, and a binary sample).

### Key Entities

- **Rename/Copy/Move/Edit-New dialog family**: the shared dialog class whose
  ANSI-window nature is the cause of the `?` substitution.
- **Viewer active encoding**: the encoding chosen on open (or by the user) that
  governs decoding and is reported to the user.
- **Decoded display line**: the Unicode form of a file line used for rendering
  and column/hit-test mapping, derived from the byte range via the active
  encoding.
- **Reference test files**: the encoding-variant sample files.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% of the panel's displayable file/dir names (including
  `emoji-🙂-dir`, `Тест-Ελλάδα-测试`, Czech-diacritics names) appear identically
  in the F2 Rename field — zero `?` substitutions.
- **SC-002**: A no-op F2 confirm on any such name leaves the on-disk name
  byte-identical; a rename to typed non-ANSI characters produces exactly those
  characters.
- **SC-003**: For each of the 8 reference text variants (UTF-8, UTF-8+BOM,
  UTF-16 LE, UTF-16 BE, Windows-1250, ISO-8859-2, ASCII, invalid-UTF-8) the
  viewer renders the reference Czech sentence correctly (or, for invalid-UTF-8,
  degrades to a legacy/default encoding without garbage-forever) and names the
  encoding actually used.
- **SC-004**: Manual encoding switch re-renders within the same interaction and
  never writes to disk (file mtime/size unchanged).
- **SC-005**: Binary sample opens as hex; ASCII file and large file open with
  no visible behavior/perf change; viewer search/scroll unaffected.
- **SC-006**: Clean Debug **and** Release x64 build with no new warnings in the
  changed files; pure-ASCII rename and view unchanged.

## Assumptions

- Builds on feature 004/005: file names are UTF-8 internally; the panel and the
  in-place quick-rename edit are already Unicode; wide helpers
  (`SalU8ToW`/`SalWToU8`/`SalSetWindowTextU8`/`SalComboAddStringU8`, etc.) exist.
  The dialog family simply needs to become Unicode windows so those helpers stop
  being lossy.
- The `CDialog` infrastructure already supports Unicode windows
  (`DialogBoxParamW` when the Unicode flag is set); making a specific dialog
  Unicode is a supported, existing capability, not new infrastructure.
- The viewer keeps its sliding-buffer byte model; decoding to Unicode happens
  for rendering and column mapping. Single-byte legacy code tables remain for
  legacy encodings; UTF-8/UTF-16 are added as first-class detected encodings.
- The environment is headless (no keystroke automation); verification is by
  build, static analysis, encoding reasoning on the generated files, and a final
  human walkthrough.
- Default encoding when detection is inconclusive follows the existing viewer
  default-conversion setting.
