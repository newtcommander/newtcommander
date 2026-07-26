# Feature Specification: Fix UI Text Encoding in Language Selection and Rename Captions

**Feature Branch**: `043-fix-ui-text-encoding`
**Created**: 2026-07-26
**Status**: Draft
**Input**: User description: "Když chci přepnout jazyk aplikace, tak se již samotný název aktuálního jazyka v Možnosti → Konfigurace → Jazyk zobrazuje špatně, dále se zobrazují špatně názvy jazyků v pop-up okně s výběrem jazyků, viz screenshot `./temp/language_select.png`. Dále se špatně zobrazuje nadpis okna pro rychlé přejmenování souboru F2 v případě, že není použit anglický jazyk, ale např. čeština — textový vstup s názvem souboru je v pořádku, ale text field s nadpisem je rozbitý, např. pro přejmenování souboru `Тест-Ελλάδα-测试 +ěš` v adresáři `C:\Users\pavel\AppData\Local\Temp\salamander-test\010` se zobrazí špatně nadpis, viz soubor `./temp/rename_in_czech.png`. […] Zaměř se na všechna místa, která jsou spojena s kódováním zobrazení názvů souborů a adresářů napříč celým programem […] MUSÍŠ explicitně pro každé vytvořit test a zkontrolovat, že vše funguje správně a to pro kombinaci všech dostupných jazyků. NEEXISTUJE, aby bylo zobrazení na některém z již fungujících míst po této opravě rozbité."

## Problem Statement

Three more surfaces show the same defect this project has now repaired twice:
**a UTF-8 string is handed to a display call that interprets bytes as legacy
single-byte text**, so every non-ASCII character becomes mojibake.

### Report 1 — the language selection list (`temp/language_select.png`)

| On screen | Should be |
| --- | --- |
| `ÄŚeÅˇtina (ÄŚesko)` | `Čeština (Česko)` |
| `AngliÄŤtina (SpojenĂ© stĂˇty)` | `Angličtina (Spojené státy)` |
| `NÄ›mÄŤina (NÄ›mecko)` | `Němčina (Německo)` |
| `MaÄŹarÅˇtina (MaÄŹarsko)` | `Maďarština (Maďarsko)` |

Every language name in the list is destroyed. The dialog's own labels
("Výběr jazyka", "Vyberte jeden z nainstalovaných jazyků") and the note field
("Poznámka: Česká verze") are **correct** — only the list is wrong.

### Report 2 — the current language in Options ▸ Configuration ▸ Language

The same name, in the configuration page's read-only field, is destroyed the
same way.

### Report 3 — the Quick Rename caption (`temp/rename_in_czech.png`)

Renaming `Тест-Ελλάδα-测试 +ěš` with F2:

```
Přejmenovat adresář "Ð¢ÐµÑ<mojibake>" na      ← the caption
Тест-Ελλάδα-测试 +ěš                            ← the input field, correct
```

The localized words are correct and the name inside them is mojibake — while
the edit field directly below shows the very same name perfectly. Two controls,
two routes, one correct and one not.

### One cause, three routes

| Site | UTF-8 value comes from | Drawn through |
| --- | --- | --- |
| Language list (`dialogs2.cpp`) | `GetLanguageName()` | `ListView_SetItemText` — the legacy list-view call |
| Configuration field (`dialogs4.cpp`) | `GetLanguageName()` | `SetDlgItemText` — the legacy window-text call |
| Rename caption (`fileswn5.cpp`) | the file name | a composed subject drawn the legacy way |

### These are not regressions from feature 042 — and that matters

Establishing this was the first thing checked, because the user's standing
instruction is that nothing already working may break.

- `GetLanguageName()` began returning UTF-8 in commit `01b1556` — **feature
  041**, when 36 locale call sites moved to the UTF-8 locale wrappers. Its two
  consumers were never updated, so both have been broken since that commit.
- The rename caption composes a localized template with a file name, and file
  names have been UTF-8 since feature 004. It has been broken far longer.
- Feature 042 never touched `dialogs2.cpp`, and did not touch the line that
  builds the rename caption.

So this is old damage newly noticed, not new damage. That distinction is worth
stating plainly, because the reason it went unnoticed is the same reason it will
keep happening: **none of it is visible in an English build.**

### Why feature 042's guard did not catch any of this

Feature 042 shipped a build guard for exactly this class. It missed all three,
and understanding why is the whole point of this feature:

- The guard's composed-message rule requires the result to reach a **message
  box**. Two of these reach a list view and a dialog field instead.
- The guard's rule looks for a localized template as the *format string*. The
  language list has no template at all — it is a bare UTF-8 value handed
  straight to a legacy call.

The guard was written around the shape of the two defects then in hand, so it
recognises those two shapes and nothing else. A guard that only detects the
bugs already found is a regression test, not a guard. Widening it so it
describes the *defect* — a UTF-8 value reaching a legacy display call — rather
than two examples of it, is a first-class deliverable here.

### Scale, measured

Three independent surveys of the main application (excluding plugins) were run
before this spec was finalised. The reported three surfaces are the visible part
of a substantially larger set.

**The composed-caption family** — a localized `LoadStr` template plus a UTF-8
name, held in a `CTruncatedString`. **Ten** sites, none using `LoadStrU8`:

| Site | User action |
| --- | --- |
| `fileswn5.cpp:2385` | **F2 Quick Rename** — reported |
| `fileswn8.cpp:475` | **F5 Copy / F6 Move / F8 Delete** (disk) — reported |
| `fileswna.cpp:95` | **F5 / F6 / Delete** on a plugin file system |
| `fileswn7.cpp:471` | F5 copy out of an archive, F8 delete from archive |
| `fileswn7.cpp:1384` | Pack files (Alt+F5) |
| `fileswn7.cpp:1588` | "add to existing archive?" confirmation |
| `fileswn7.cpp:1724` | Unpack archive (Alt+F6) |
| `fileswn5.cpp:408` | NTFS compress / encrypt confirmation |
| `finddlg2.cpp:1949` | Find log → Ignore |
| `plugins3.cpp:538` | any plugin's subject label |

The consumers of these captions (`dialogs3.cpp`, `msgbox.cpp`) already *have* a
correct wide path — it is simply never taken, because the ANSI template makes
the composed string invalid UTF-8 and the strict conversion falls back. One
exception: `plugins3.cpp:547/550` has **no wide path at all** and is therefore
broken even in an English build.

**Legacy list-view calls**: 33 sites examined, **1** carries UTF-8 — the
reported language list (`dialogs2.cpp:883`). The rest are ASCII, `LoadStr`, or
already use the existing `SalListViewSetItemTextU8` helper.

**Legacy window-text and drawing calls**: 162 sites examined, ~63 are the
deliberate ANSI fallback of an existing wide path and ~78 are `LoadStr`/ASCII.
The remainder are confirmed defects, including the overwrite-confirmation
dialog's size/date/time fields (F5/F6), the Find window caption and status bar,
the drag image, the volume-information and occupied-space dialogs, and the
reported configuration field.

**A whole additional trigger surfaced**: `NumberToStr()` and
`PrintDiskSize()` splice the locale thousands and decimal separators, which
feature 041 made UTF-8. In Czech that separator is a non-breaking space, so
**every formatted number** passed to a legacy sink renders as `1Â 234Â 567`.
That affects roughly a dozen further fields.

### One regression from feature 042, found and fixed here

The survey also caught a mistake made by the previous feature.
`dialogs5.cpp:832` composes a plugin's name into a template; feature 042
converted that template to `LoadStrU8` because the substituted argument is a
local variable `name`, a copy of the ANSI `p->Name` that its classifier did not
recognise as plugin metadata. The result is a message mixed the *other* way
round — the plugin name renders and the localized words do not. Reverted, and
annotated so it is not converted again. A sweep confirmed this was the **only**
such misclassification among the 84 sites feature 042 converted.

## Clarifications

### Session 2026-07-26

- Q: Which surfaces does this feature repair, versus record? → A: The three reported ones, plus every site the inventory shows to carry a UTF-8 value into a legacy display call, within the main application. Plugins are reviewed and recorded but never modified, and no shared entry point changes behaviour for plugin callers — the boundary feature 042 established is carried forward unchanged.
- Q: How is "nothing already working may break" enforced? → A: Every surface repaired by features 004, 005, 010, 041 and 042 is re-verified after this change, in every shipped language, and the evidence recorded. A repair that fixes one surface and breaks another is a failure of this feature, not a trade-off.
- Q: How much language coverage does verification need? → A: All 9 shipped languages for every surface that shows localized text or a localized-plus-name composition. English alone proves nothing here — English resources are pure ASCII, which is precisely why all three defects survived this long.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Language names are readable (Priority: P1)

A user opens the language selection dialog to switch the application's language,
and reads the list of installed languages.

**Why this priority**: This is the reported defect, and it is self-defeating in a
specific way: the list a user consults *to choose a language* is unreadable in
exactly the languages they might be choosing between. A user who cannot read
`Čeština` cannot confidently pick it.

**Independent Test**: Open the language selection dialog and compare each row
against the language's real name.

**Acceptance Scenarios**:

1. **Given** the language selection dialog is open, **When** the user reads the
   list, **Then** each language reads correctly — `Čeština (Česko)`,
   `Angličtina (Spojené státy)`, `Němčina (Německo)`,
   `Maďarština (Maďarsko)` — with no `Ä`, `Å`, `Ă`, `Ř` or `ˇ` sequence
   anywhere.
2. **Given** the same dialog, **When** the user reads its labels, the note field
   and the path column, **Then** all of them are still correct (they are today
   and must remain so).
3. **Given** the application running in each of the 9 shipped languages,
   **When** the dialog is opened, **Then** every language name is correct in
   every one of them.
4. **Given** the user selects a language and confirms, **When** the application
   restarts in that language, **Then** the selection took effect as before.

---

### User Story 2 - The configured language reads correctly (Priority: P1)

A user opens Options ▸ Configuration ▸ Language and reads which language is
currently active.

**Why this priority**: Same value, same cause, separately reported, and it is the
field that tells the user what is currently set. It is grouped with User Story 1
only in cause, not in location — a fix to one does not automatically fix the
other, because they use different display calls.

**Independent Test**: Open the configuration page and read the language field.

**Acceptance Scenarios**:

1. **Given** the configuration page is open with Czech active, **When** the user
   reads the language field, **Then** it reads `Čeština (Česko)` exactly.
2. **Given** each of the 9 shipped languages in turn, **When** the page is
   opened, **Then** the field is correct in every one.
3. **Given** the rest of the configuration page, **When** the user reads it,
   **Then** every other control is unchanged.

---

### User Story 3 - The Quick Rename caption reads correctly (Priority: P1)

A user presses F2 on a file or directory whose name contains non-ASCII
characters and reads the caption above the input field.

**Why this priority**: Separately reported, and the most jarring of the three
because the correct name is visible two centimetres below the broken one, in the
same dialog. It also affects the most common operation of the three.

**Independent Test**: Press F2 on `Тест-Ελλάδα-测试 +ěš` in a localized build and
compare the caption against the edit field beneath it.

**Acceptance Scenarios**:

1. **Given** the file `Тест-Ελλάδα-测试 +ěš` is focused, **When** the user
   presses F2 in a Czech UI, **Then** the caption reads
   `Přejmenovat adresář "Тест-Ελλάδα-测试 +ěš" na` with both the Czech words and
   the name correct.
2. **Given** the same dialog, **When** the user compares the caption with the
   edit field below it, **Then** the name is identical in both.
3. **Given** each of the 9 shipped languages, **When** the dialog opens on a
   non-ASCII name, **Then** the caption is correct in every one.
4. **Given** a name long enough to be truncated in the caption, **When** the
   dialog opens, **Then** the truncation does not split a character in half.
5. **Given** the user confirms the rename, **When** the operation runs, **Then**
   it renames to exactly the typed name, as before.

---

### User Story 4 - The class is swept, and the guard is widened to describe it (Priority: P2)

Every place in the main application where a UTF-8 value reaches a legacy display
call is found, recorded, and repaired; and the automated guard is widened so it
describes the defect rather than the three examples currently in hand.

**Why this priority**: This is the user's actual instruction — sweep everything
connected with displaying file and directory names across the whole program. It
ranks below the three reported defects only because those are what is blocking
today. The guard widening is what stops a fourth report: the existing guard
passed cleanly while all three of these defects were present, which is precise
evidence that it describes too narrow a shape.

**Independent Test**: Read the inventory, pick any surface at random, exercise it
in a localized build against the fixture set, and confirm the recorded verdict.
Separately, reintroduce each of the three defects and confirm the widened guard
fails.

**Acceptance Scenarios**:

1. **Given** the completed inventory, **When** a reviewer looks for any main
   application site that passes a UTF-8 value to a legacy display call, **Then**
   it appears with a verdict of verified-correct, fixed, or deferred with a
   stated reason.
2. **Given** the widened guard, **When** any of the three reported defects is
   reintroduced, **Then** the build fails — demonstrated for each separately.
3. **Given** the widened guard, **When** it runs on the repaired tree, **Then**
   it passes with every remaining finding annotated with a reason.
4. **Given** the shipped plugins, **When** the feature ships, **Then** each has
   been reviewed and recorded, and none modified.

---

### User Story 5 - Nothing that worked before is broken (Priority: P1)

Every surface repaired by earlier features still works, in every shipped
language.

**Why this priority**: P1 and non-negotiable — the user's words were that it must
not happen that a previously working place is broken by this fix. This is a
stated acceptance condition of the feature, not a quality aspiration, and it
ranks with the reported defects rather than after them.

**Independent Test**: Walk the full regression matrix — the surfaces of features
004, 005, 010, 041 and 042 — in every shipped language.

**Acceptance Scenarios**:

1. **Given** the Find results list (feature 042), **When** the reported search is
   run, **Then** all names still display exactly.
2. **Given** the duplicate-name notice (feature 042), **When** it appears in each
   of the 9 languages, **Then** name and localized text are both still correct.
3. **Given** the panel information line (feature 041), **When** the reported
   `.mkv` is focused, **Then** it still displays correctly.
4. **Given** the panel file lists, the Find Path column and long-path results,
   **When** exercised against the fixture set, **Then** all still display
   correctly.
5. **Given** every dialog touched by this feature, **When** exercised in each of
   the 9 languages, **Then** no control that was correct before is wrong after.

---

### Edge Cases

- A language whose name contains characters outside the machine's codepage
  (Greek, Cyrillic, CJK language names) — must display, not degrade to `?`.
- A file name mixing scripts (`Тест-Ελλάδα-测试 +ěš`) in a composed caption.
- A name long enough to be truncated in a caption — no split characters.
- A malformed stored name reaching any repaired surface — one `U+FFFD` per
  offending character, nothing else affected.
- A localized template combined with a name, where the template's language has
  characters the name's script does not, and vice versa.
- The language list before any language module is loaded (first run).
- Plugin-supplied text reaching a repaired shared surface — must be unchanged.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The language selection list MUST display every installed
  language's name exactly, in every shipped language of the user interface.
- **FR-002**: The configuration page's language field MUST display the active
  language's name exactly, in every shipped language.
- **FR-003**: The Quick Rename caption MUST display the file or directory name
  exactly, with its surrounding localized text also correct, in every shipped
  language.
- **FR-004**: The name in the Quick Rename caption MUST be identical to the name
  in the edit field of the same dialog.
- **FR-005**: No display surface MAY interpret a UTF-8 value as legacy
  single-byte text, and none MAY convert a name through a lossy codepage
  conversion. Carried forward unchanged from feature 042 (FR-002 there).
- **FR-006**: Where a name is combined with localized text, the whole composed
  string MUST be in one encoding. Carried forward from feature 042.
- **FR-007**: When a value genuinely cannot be interpreted, the system MUST show
  `U+FFFD` for the offending character only, leaving everything else intact.
- **FR-008**: Truncation of any repaired caption MUST NOT split a character or a
  surrogate pair.
- **FR-009**: The feature MUST produce a written inventory of every main
  application site passing a UTF-8 value to a legacy display call, each marked
  verified-correct, fixed, or deferred with a stated reason. The inventory MUST
  precede the repairs it drives.
- **FR-010**: Every site the inventory finds carrying this defect MUST be fixed
  and verified in the running application.
- **FR-011**: The automated guard MUST be widened to describe the defect rather
  than the examples: it MUST detect a UTF-8 value reaching a legacy list-view
  call, a legacy window-text call, or a composed caption — not only a
  message box.
- **FR-012**: The widened guard MUST be demonstrated to fail when each of the
  three reported defects is reintroduced, separately.
- **FR-013**: All behaviour correct today MUST remain correct — specifically the
  surfaces repaired by features 004, 005, 010, 041 and 042 — verified in every
  shipped language.
- **FR-014**: The plugin interface MUST remain unchanged in shape and behaviour.
  No plugin may be modified, and no plugin's output may look different.
- **FR-015**: Every fix MUST be verified in the running application, in every
  shipped language for surfaces where language can matter, and the evidence
  recorded, before the feature is considered complete.
- **FR-016**: Every repaired surface MUST have an explicit automated test, so
  that its correctness is asserted rather than observed once.

### Key Entities

- **Displayed value**: Any string shown to the user that originates as UTF-8 — a
  file name, a path, a locale-derived name, a composed caption.
- **Legacy display call**: A window-text, list-view or drawing call that
  interprets its input as single-byte text. The route that destroys the value.
- **Language name**: The human-readable name of an installed language, derived
  from the operating system's locale data and therefore UTF-8.
- **Composed caption**: Localized template plus a substituted value, shown as a
  dialog caption or subject line.
- **Fixture set**: `C:\Users\pavel\AppData\Local\Temp\salamander-test` and its
  `010` subtree, plus the 9 shipped language modules.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: All three reported defects reproduce zero times: language names,
  the configuration field and the rename caption all read correctly.
- **SC-002**: Language names are correct in **9 of 9** shipped languages — 100%,
  no exceptions.
- **SC-003**: The Quick Rename caption is correct in **9 of 9** shipped
  languages, for names in Latin, Greek, Cyrillic, CJK and emoji.
- **SC-004**: The name in the rename caption matches the name in the edit field
  character for character, for 100% of fixture names.
- **SC-005**: Zero regressions: every surface verified by features 041 and 042
  passes unchanged, in every shipped language.
- **SC-006**: The widened guard fails when each of the three reported defects is
  reintroduced — demonstrated three times, separately.
- **SC-007**: The widened guard would have caught all three defects on the tree
  as it stood before this feature — verified by running it against that tree.
- **SC-008**: Every repaired surface has an automated test that fails if the
  surface regresses.
- **SC-009**: The inventory accounts for every main application site in the
  class; a reviewer spot-checking any three entries finds the verdict correct.
- **SC-010**: Every shipped plugin appears in the review record, and plugin
  binaries are unchanged.

## Assumptions

- **These three defects predate feature 042 and are not caused by it.** Verified
  against the commit history before writing this spec. Reports 1 and 2 date from
  feature 041 (`01b1556`); Report 3 is older still.
- **The `U+FFFD` policy and the plugin boundary follow features 041 and 042
  unchanged.** Neither is reopened.
- **English builds cannot reveal this defect class.** English resources are pure
  ASCII, and ASCII is valid UTF-8. Every verification that could plausibly depend
  on the interface language is done in all 9 shipped languages.
- **The existing feature 042 guard is a starting point, not a finished
  instrument.** It passed cleanly while all three of these defects were present.
  Widening it is treated as a defect fix in its own right.
- **Fixtures already exist** and cover Latin with diacritics, Greek, Cyrillic,
  CJK, emoji and long paths.
- **Verification happens in the running application**, not by inspection alone,
  consistent with how features 041 and 042 were closed out.

## Dependencies

- Builds on feature 042 (`042-fix-find-results-encoding`) — the guard, the
  `LoadStrU8` composition rule, and the plugin boundary.
- Builds on feature 041 (`041-fix-infoline-encoding`) — the UTF-8 locale
  wrappers whose output these three surfaces mishandle, and the `U+FFFD` policy.
- Builds on features 004 and 005 — UTF-8 names and their presentation in dialogs.
- Requires the fixture directory and all 9 shipped language modules
  (`build.cmd full`) for verification.
