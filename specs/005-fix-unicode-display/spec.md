# Feature Specification: Correct Display of Unicode File Names in Dialogs and Text Fields

**Feature Branch**: `005-fix-unicode-display`
**Created**: 2026-07-15
**Status**: Draft
**Input**: User description: "Navazujeme na poslední úpravy - tj. plná podpora Unicode. V testovacím adresáři C:\Users\pavel\AppData\Local\Temp\salamander-test jsou adresáře č-dir pro porovnání obě verze. Problém je, že když dám přejmenovat F2 tak v pop-up okně v editačním poli zobrazí pro první adresář: "cĚŚ-dir" a pro druhý pak "ÄŤ-dir". Tedy ani jedno není správně. V rámci tohoto rozšíření proveď detailní kontrolu všech dotčených částí aplikace, navrhni a proveď opravy."

## Problem Statement

Feature 004 gave Open Salamander full Unicode fidelity for file names:
the panels list names correctly and file operations preserve names
exactly as the operating system stores them. However, at least one
user-facing text surface was left behind: the **Rename dialog (F2)**.

The defect is reproducible with the prepared test data in
`C:\Users\pavel\AppData\Local\Temp\salamander-test`, which contains two
directories that both display as **`č-dir`** in the panel:

| Directory | Stored form | What F2 shows (wrong) |
|-----------|-------------|-----------------------|
| First `č-dir` | Decomposed: `c` + combining caron (U+0063 U+030C) | `cĚŚ-dir` |
| Second `č-dir` | Composed: `č` (U+010D) | `ÄŤ-dir` |

In both cases the dialog's edit field shows a garbled character
sequence (mojibake) instead of the real name. The pattern of the
garbling shows that the name — correct everywhere else — is being
misinterpreted on its way into the dialog's text field. A user who
confirms such a dialog risks renaming the item to the garbled text; a
user who wants to edit the name cannot, because the starting text is
already wrong.

Because the Rename dialog is unlikely to be the only such surface, this
feature mandates a **systematic audit of every place in the application
where a file or directory name is displayed in, or accepted from, a
text field** — dialogs, confirmation prompts, error messages, window
titles, status areas — followed by fixes to every surface found
defective, so that the Unicode support delivered by feature 004 is
consistent end to end.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Rename dialog shows and preserves the true name (Priority: P1)

A user selects a file or directory whose name contains accented or
non-Latin characters (in either composed or decomposed form), presses
F2, sees the exact name pre-filled in the edit field, and can confirm
or edit it. The name that ends up on disk is exactly what the user saw
and typed.

**Why this priority**: This is the reported, reproducible defect. The
rename flow is one of the most frequent file-manager operations, and a
garbled pre-filled name can lead to accidental destructive renames.

**Independent Test**: Using the two `č-dir` test directories, press F2
on each and verify the edit field shows `č-dir`; confirm without
changes and verify the name on disk is untouched.

**Acceptance Scenarios**:

1. **Given** a directory named `č-dir` stored in composed form,
   **When** the user presses F2, **Then** the edit field shows exactly
   `č-dir`.
2. **Given** a directory named `č-dir` stored in decomposed form,
   **When** the user presses F2, **Then** the edit field shows text
   visually identical to `č-dir`.
3. **Given** the Rename dialog pre-filled with an unmodified name,
   **When** the user confirms it, **Then** the item's name on disk is
   completely unchanged (including its composed/decomposed form) and no
   error is reported.
4. **Given** the Rename dialog open on `č-dir`, **When** the user edits
   the name to `řž-dir` and confirms, **Then** the directory is renamed
   to exactly `řž-dir`.

---

### User Story 2 - Every name-bearing text surface renders Unicode correctly (Priority: P2)

A user working with Unicode-named files sees the correct name in every
part of the application that presents the name as text: copy/move
dialogs, delete and overwrite confirmations, attribute dialogs, error
messages, window and dialog titles, the command line, status bars, and
equivalent surfaces in the bundled plugins.

**Why this priority**: The reported symptom is one instance of a class
of defect. Fixing only the Rename dialog would leave users encountering
the same garbling elsewhere, undermining trust in every prompt that
names a file.

**Independent Test**: With the test data set (composed, decomposed, and
non-Latin sample names), walk through each audited surface and verify
the displayed name matches the panel display.

**Acceptance Scenarios**:

1. **Given** a Unicode-named file, **When** the user starts a copy or
   move (F5/F6), **Then** the dialog shows the source name and
   pre-filled target correctly.
2. **Given** a Unicode-named file, **When** a confirmation or error
   prompt referencing the file appears (delete, overwrite, access
   denied), **Then** the name in the prompt is rendered correctly.
3. **Given** the completed audit inventory of name-bearing surfaces,
   **When** each surface is exercised with the sample names, **Then**
   no surface shows garbled text.

---

### User Story 3 - Names entered by the user are applied with full fidelity (Priority: P3)

A user types or pastes a new name containing characters outside the
system code page (for example Cyrillic, Greek, CJK, or emoji on a Czech
Windows system) into any editable name field, and the resulting file or
directory carries exactly that name.

**Why this priority**: Display and input are two directions of the same
round trip. Correct display with lossy input would still corrupt names,
but this direction is less frequently exercised than display.

**Independent Test**: Rename a file to `Тест-测试-🙂` via F2 and verify
the name on disk matches exactly; repeat via the create-directory (F7)
field.

**Acceptance Scenarios**:

1. **Given** the Rename dialog, **When** the user enters a name with
   characters outside the system code page and confirms, **Then** the
   item on disk carries exactly the entered name.
2. **Given** the create-directory dialog (F7), **When** the user enters
   a Unicode name, **Then** the created directory carries exactly the
   entered name.

---

### Edge Cases

- Composed and decomposed variants of the same visual name exist side
  by side (as in the test directory): each F2 invocation must show and
  preserve the specific item's stored form, and renaming one must never
  affect the other.
- Confirming the Rename dialog without edits on a decomposed name must
  not silently convert it to the composed form (or vice versa) — the
  operation must be a true no-op, consistent with feature 004's
  "follow the operating system, no silent normalization" decision.
- Names combining multiple scripts (Latin + Cyrillic + CJK + emoji) in
  a single name must display and round-trip correctly.
- Text pasted into a name field from the clipboard must retain full
  Unicode fidelity.
- Unicode names at or near the maximum name length, and inside long
  paths (feature 004 territory), must behave the same as short ASCII
  names in every audited surface.
- Sample names containing characters with no representation in any
  legacy code page (e.g. emoji) must not be replaced by `?` or dropped
  anywhere in the audited surfaces.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Every application surface that displays a file or
  directory name as text MUST render the name identically to how the
  file panels render it, for any Unicode name the file system can
  store.
- **FR-002**: The Rename dialog MUST pre-fill the exact stored name of
  the selected item, regardless of the name's form (composed,
  decomposed, non-Latin scripts, characters outside the system code
  page).
- **FR-003**: Confirming the Rename dialog with the pre-filled name
  unmodified MUST leave the item's name unchanged in every respect,
  including its composed/decomposed form, and MUST NOT report an error.
- **FR-004**: Any name entered or edited by the user in a name field
  (rename, new directory, copy/move target, and equivalents) MUST be
  applied to the file system with full Unicode fidelity, including
  characters outside the system code page.
- **FR-005**: The team MUST perform and document a systematic audit of
  all application surfaces (including bundled plugins) where file or
  directory names are displayed in or accepted from text fields,
  classify each as correct or defective, and fix every defective
  surface within this feature.
- **FR-006**: No surface in the display-or-input round trip may
  silently normalize, substitute, or drop characters of a name.
- **FR-007**: Existing behavior for names consisting only of unaccented
  Latin characters MUST remain unchanged (no regressions in the
  rename, copy, move, create, and delete flows).

### Key Entities

- **File/Directory Name**: A Unicode string exactly as stored by the
  file system; the same visual name may exist in composed or decomposed
  form, and these are distinct names.
- **Name-bearing text surface**: Any UI element that presents a name as
  text (read-only: labels, titles, prompts, status lines) or accepts a
  name as text (editable: rename field, new-directory field, target
  path field). Each surface is either *correct* or *defective* with
  respect to Unicode rendering.
- **Audit inventory**: The documented list of all name-bearing text
  surfaces produced by FR-005, with a verdict and resolution for each.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Pressing F2 on each of the two `č-dir` test directories
  shows a pre-filled name visually identical to `č-dir` in 100% of
  attempts; confirming without edits changes nothing on disk.
- **SC-002**: 100% of the surfaces in the audit inventory render the
  full sample-name set (composed, decomposed, multi-script, emoji,
  outside-code-page) without any garbled, substituted, or missing
  characters.
- **SC-003**: Renaming and creating items using names typed with
  characters outside the system code page succeeds with an exact match
  between typed and stored name in 100% of tested cases.
- **SC-004**: All rename/copy/move/create/delete flows exercised with
  plain ASCII names behave exactly as before the change (zero
  regressions in the existing verification suite).

## Assumptions

- The two test directories were verified on 2026-07-15: the first is
  the decomposed form (`c` U+0063 + combining caron U+030C), the second
  the composed form (`č` U+010D). The observed garbling is consistent
  with the correct internal name being misinterpreted on its way into
  the dialog's text field — confirming the defect is in the UI text
  path, not in file-system handling delivered by feature 004.
- Scope covers the core application and the 35 bundled plugins'
  user-visible text surfaces, mirroring the scope decision recorded in
  feature 004; third-party plugins built against the legacy interface
  remain covered by the existing detect-and-refuse behavior.
- Rendering of file *content* (viewer text encoding) remains out of
  scope, as decided in feature 004; only file/directory names and paths
  are in scope.
- File-system operations themselves (create, rename, copy, move,
  delete) already preserve Unicode names correctly per feature 004;
  this feature addresses the presentation and input of names in the
  user interface. If the audit reveals a fidelity defect deeper than
  the UI text path, fixing it is in scope, since FR-003/FR-004 are
  end-to-end requirements.
- The audit and fixes target the current development baseline
  (branch `ai-main` after feature 004 completion).
