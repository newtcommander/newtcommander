# Feature Specification: Fix File Name Encoding in Find Results and Name Notices

**Feature Branch**: `042-fix-find-results-encoding`
**Created**: 2026-07-26
**Status**: Draft
**Input**: User description (two reports in one session):

1. "Panel vyhledávání nezobrazuje správně názvy souborů, když otevřu vyhledávání
   pomocí CTRL+F a dám vyhledat `🙂-d` v adresáři
   `C:\Users\pavel\AppData\Local\Temp\salamander-test\010`, vyhledá to správné
   adresáře, ale v seznamu výsledků je vypíše se špatnými znaky, viz obrázek
   `./temp/find_filenames.png`. Opět se asi jedná o špatnou práci s již
   několikrát řešenými problémy se zobrazováním těchto znaků. Všechny opravy
   verifikuj, nesmíš nic rozbít, nesmíš zanést žádnou regresní chybu, toto je
   VELMI citlivé téma, které řešíme stále dokola."
2. "Ještě ke specifikaci - došlo přesně k tomu, čeho se obávám, při vstupu do
   adresáře `C:\Users\pavel\AppData\Local\Temp\salamander-test` se zobrazuje
   pop-up okno s hlášením, že jsou v něm dva shodné názvy adresáře `č-dir`, ale
   v tomto popup okně se opět rozbilo kódování, resp. zobrazení názvu tohoto
   adresáře, viz obrázek `.\temp\regrese_warning.png`."

> The branch name predates the second report. The scope of this feature is both
> reported surfaces and the defect class they share, not the Find dialog alone.

## Problem Statement

Two separate surfaces were reported in one session. They look like different
bugs and they garble names in two visibly different ways, but they are the same
defect: **a file name is correct everywhere it is stored and is destroyed at the
last step before it is drawn.**

### Report 1 — the Find results list

Searching for `🙂-d` under `C:\Users\pavel\AppData\Local\Temp\salamander-test\010`
(`temp/find_filenames.png`):

| On disk                      | Shown in the results list     |
| ---------------------------- | ----------------------------- |
| `emoji-🙂-dir`               | `emoji-??-dir`                |
| `emoji-🙂-dir - Copy`        | `emoji-??-dir - Copy`         |
| `emoji-🙂-dir - Copyě 😍😍😍` | `emoji-??-dir - Copyě ??????` |

The search itself is correct — all four expected items are found, so the term the
user typed reached the engine intact. Only the display of the names is wrong.

The third row is the diagnostic one. `ě` survives, but each emoji becomes **two**
question marks — six for the three emoji. That is the signature of a conversion
down to the machine's legacy single-byte codepage: `ě` exists in this machine's
Central European codepage and is kept; emoji exist in no single-byte codepage
and are replaced, one question mark per unit of the internal representation,
which is why one emoji costs two. The loss is silent and unrecoverable — nothing
in the display tells the user what the item is really called.

The **Path** column of the same list is correct, including the very long path in
row 4. The two columns do not reach the screen the same way: the path is drawn by
the dialog through the modern text interface, while the name is handed to the
list control through a legacy single-byte interface. The application already
prefers the modern interface for this list and already supplies the name through
it, but for this list that preference is not taking effect, so the legacy route
is the one actually used.

### Report 2 — the duplicate-name notice

Entering `C:\Users\pavel\AppData\Local\Temp\salamander-test`, which contains two
directories whose names look identical but are stored with different Unicode
spellings, raises an informational notice (`temp/regrese_warning.png`):

```text
Tato složka obsahuje položky, jejichž názvy vypadají stejně, ale používají
odlišné zápisy v Unicode (např. předkomponované vs. rozložené diakritické znaky):

    ÄŤ-dir                     ← the directory is called č-dir

Systém Windows je považuje za odlišné soubory. Program Newt Commander
zachovává oba názvy přesně tak, jak jsou uložené.
```

Here the localized Czech sentences around the name render **perfectly** —
`složka`, `vypadají`, `předkomponované` are all correct — and only the name is
mojibake: `č` becomes `ÄŤ`. That is the opposite failure of Report 1: not a lossy
conversion, but no conversion at all — the name's raw internal bytes drawn as if
they were legacy single-byte text.

The reason the frame survives and the name does not is that the notice is built
by splicing two texts of **different encodings** into one string: the localized
template comes from the language resource in the legacy encoding, the name comes
from the file system in the application's internal encoding. The notice window
does have a modern drawing path and prefers it, but it accepts that path only if
the *whole* composed string is valid in the internal encoding. A Czech template
in the legacy encoding is not, so the whole notice falls back to legacy drawing —
and in that fallback the template is right and the name is wrong.

This is the same failure shape feature 041 documented for the information line —
one part of a concatenated string in the wrong encoding poisons the conversion
for all of it — reappearing at a different call site.

### Why these are one feature

Both reports are instances of: *a file name, correct in storage, is handed to a
display surface that is not prepared to receive it.* The two visible flavours are
the two ways that can go wrong:

- **Lossy** — the name is converted down to a codepage that cannot hold it, and
  characters are replaced by `?` (Report 1).
- **Uninterpreted** — the name's bytes are drawn as if they were legacy text, and
  characters turn into `Ã`/`Ä`-style sequences (Report 2).

Feature 041 repaired one instance of the second flavour, in the information line,
and explicitly recorded that the application-wide version of this problem was too
large to take on at the time. The user's second report is the predicted
consequence: fixing surfaces one at a time means the next surface is the next
report. **The deliverable of this feature is therefore not only the two reported
fixes but a written inventory of the surfaces where a file name meets a display,
so that "which surfaces are known-correct" stops being guesswork — and an
automated guard that fails when the defect returns, so the next occurrence is
caught by the build rather than by the user.**

### A second symptom inside Report 1

Typing characters into the Find results list to jump to a matching item compares
the typed text against the stored names. On the legacy route the two are in
different encodings, so the jump silently fails for any name that is not plain
ASCII — the user types the first letters of an item they can see and nothing
happens. Same list, same cause; if it is not fixed alongside the display it
becomes the next report.

### History, and why a partial fix would be the third mistake

The Find results Name column has never been correct, but it has been wrong in two
different ways:

- Before 2026-07-26 it drew the raw internal bytes, so `á` appeared as `Ã¡` —
  the *uninterpreted* flavour, wrong for every accented character.
- Commit `01b1556` (feature 041, 2026-07-26) added a conversion on the legacy
  route. That repaired accented characters — which is why `ě` is correct in
  Report 1 — and converted the failure into the *lossy* flavour.

Each fix made the column correct for a wider set of characters while leaving the
route itself defective. A third fix that merely widens the set again would be the
same mistake a third time. The requirement below is that the route stops being
lossy, not that more characters happen to survive it.

## Clarifications

### Session 2026-07-26

- Q: How far does the sweep extend — which surfaces does this feature repair, versus record for later? → A: Fix both reported surfaces **and every surface in the main application that the inventory shows to have the same defect**. The inventory is produced first, so the cost is known before the fixing is committed to. Plugins are reviewed and their state recorded, but not changed — they keep the behaviour they have today.
- Q: How is the inventory enumerated, and how is its completeness established? → A: Both axes. A mechanical pass over the code enumerates every site where a name reaches a display route and carries the completeness argument; a walk of the user-reachable surfaces with the fixture set cross-checks it, catching sites the patterns miss and validating the recorded verdicts. Neither axis alone is sufficient — Report 2 was invisible from code inspection, Report 1 was invisible from the UI until someone searched for an emoji.
- Q: Display entry points shared with plugins — may their behaviour change for plugin callers? → A: No. Plugin-visible behaviour stays bit-identical. A shared entry point keeps its current interpretation for plugin callers, and the main application opts in explicitly at each call site it fixes. This matches what feature 041 shipped (a parallel entry point, opted into per site) after its wider attempt broke the Find dialog and was reverted. The accepted cost is that there is no single choke-point fix: the inventory becomes the work list.
- Q: SC-010 requires correctness independent of the machine's codepage, but FR-015 requires runtime verification and this machine is Central European — how is it proved? → A: Structurally. FR-002 forbids any name from passing through a legacy-codepage conversion, and the FR-016 guard proves that no such conversion exists on a name path. Codepage independence therefore follows from the design rather than from sampling locales — stronger than testing a handful of codepages, and it removes the conflict with FR-015 instead of managing it. No machine locale is changed.
- Q: How much localized verification does each repaired surface need? → A: Risk-based by failure mode. A surface that composes localized text **together with** a name is verified in all 9 shipped languages, because that combination is exactly what broke Report 2 and language is the variable. A surface that displays a bare name is verified once — it contains no localized text for the name to interact with, so repeating it per language costs 9× and proves nothing new.
- Q: How should the feature protect against this defect recurring, beyond one-time manual verification? → A: Automated guard plus manual verification — regression tests in the existing test project for the composition and conversion behaviour, **and** a mechanical source check that fails when a forbidden pattern reappears. A future change that reintroduces the defect must fail rather than ship. Manual verification in the running application remains required on top of this, not instead of it.

**What this boundary includes**: any place in the main application where a stored
file or directory name is turned into text the user sees — list columns, notices
and message boxes, captions, status lines, dialog fields — and which the
inventory finds broken in either of the two flavours described above.

**What it excludes**, and why:

- **Plugins** — reviewed and recorded, not modified, and *not changed indirectly
  either*. Each plugin dialog needs its own runtime scenario to verify, which
  feature 041 already recorded as unverified work; changing them without that
  verification would violate FR-015. Because plugins call the same display entry
  points the main application does, "not modified" has to mean behaviour as well
  as source — see FR-014 and FR-014a. Whatever the review finds becomes a
  recorded candidate for a future feature.
- **The application-wide text-encoding change** feature 041 identified and
  deferred. This feature repairs surfaces that are demonstrably broken; it does
  not convert the application's text handling wholesale. If the inventory shows
  the remaining surfaces cannot be fixed individually at acceptable risk, that
  finding is recorded as the argument for that future feature rather than acted
  on here.
- **Surfaces the inventory finds broken by a *different* cause** — recorded with
  the reason, not fixed here, so this feature does not turn into an open-ended
  audit.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Found names read correctly (Priority: P1)

A user presses Ctrl+F, searches a directory tree, and reads the names of the
items that were found. Every name is shown exactly as the file system stores it,
whatever script or symbols it uses.

**Why this priority**: The results list is the only place the Find dialog shows
*what* it found. If the name is unreadable the result is unusable — the user
cannot tell two similar results apart, cannot copy the name, and cannot trust
that the tool found the right thing.

**Independent Test**: Run the reported search and compare each row of the Name
column against the directory listing on disk.

**Acceptance Scenarios**:

1. **Given** the reported directory and the search term `🙂-d`, **When** the user
   reads the results list, **Then** the names are shown as `emoji-🙂-dir`,
   `emoji-🙂-dir - Copy`, `emoji-🙂-dir - Copyě 😍😍😍` and the fourth match, with
   no `?` that is not on disk.
2. **Given** a results list containing names in Latin, Greek, Cyrillic and CJK
   scripts, **When** the user reads them, **Then** every name matches the file
   system exactly.
3. **Given** any found item, **When** the user compares its name in the results
   list with the same item's name in a file panel, **Then** the two are identical
   character for character.
4. **Given** a name that is already correct today — plain ASCII, or accented
   Latin the machine's codepage covers — **When** it appears in the results list,
   **Then** it still displays correctly (no regression).
5. **Given** a name containing a sequence that genuinely cannot be interpreted,
   **When** it appears in the results list, **Then** only that character shows as
   `�`, and the rest of the name and every other column of the row are intact.

---

### User Story 2 - The duplicate-name notice reads correctly (Priority: P1)

A user enters a directory holding two entries whose names look identical but are
stored with different Unicode spellings. The notice explaining this shows the
name it is talking about, correctly.

**Why this priority**: Equal to User Story 1 — separately reported, and worse in
one respect: the whole purpose of this notice is to tell the user *which* name is
ambiguous. A garbled name makes the message not merely ugly but self-defeating,
and it appears unprompted on entering a directory rather than only when the user
went looking.

**Independent Test**: Enter `C:\Users\pavel\AppData\Local\Temp\salamander-test`
and read the notice.

**Acceptance Scenarios**:

1. **Given** the reported directory, **When** the notice appears, **Then** the
   name is shown as `č-dir` and no `Ä`, `Å`, `Ã` or `â€` sequence appears
   anywhere in the notice.
2. **Given** the same notice, **When** the user reads the explanatory sentences
   around the name, **Then** they are correct in the user's language — the
   surrounding text must not regress while the name is repaired.
3. **Given** a duplicate pair whose names use emoji or CJK characters, **When**
   the notice appears, **Then** the name is shown exactly, with no `?`
   substitution.
4. **Given** the application running in each shipped language, **When** the
   notice appears, **Then** both the localized text and the name are correct.

---

### User Story 3 - Type-to-search reaches non-ASCII names (Priority: P2)

With the Find results list focused, a user types the first letters of an item
they can see and the selection jumps to it.

**Why this priority**: Same list and same cause as User Story 1, and it is the
normal way to reach one result among many. It ranks below the two display fixes
because a correct display is what makes the list usable at all, but leaving it
unfixed means knowingly shipping a second symptom of a cause we just repaired.

**Independent Test**: In a results list containing non-ASCII names, type the
leading characters of one and observe the selection.

**Acceptance Scenarios**:

1. **Given** a results list containing `Тест-Ελλάδα-测试 +ěš`, **When** the user
   types the leading characters of that name, **Then** the selection moves to it.
2. **Given** a results list containing `emoji-🙂-dir` and `emoji-🙂-dir - Copy`,
   **When** the user types `emoji-`, **Then** the selection moves to the first,
   and typing again advances to the next match.
3. **Given** a results list of plain-ASCII names, **When** the user types leading
   characters, **Then** the jump behaves exactly as it does today (no regression).
4. **Given** the user types characters matching no item, **When** the search
   completes, **Then** the selection does not move and no error appears.

---

### User Story 4 - Every remaining affected surface is found and repaired (Priority: P2)

A user works through the application — browsing panels, running file operations,
opening dialogs, reading errors and confirmations — with files whose names use
any script. Wherever a name appears, it is correct. Behind that, a written
inventory records every place a name is composed into displayed text and what
was found and done there.

**Why this priority**: This is the user's actual complaint — "toto je VELMI
citlivé téma, které řešíme stále dokola." Two reports in one session, on two
different surfaces, is the evidence that fixing one surface at a time does not
converge. Repairing the class rather than the instances is what stops the
recurrence; the inventory is what lets a reviewer confirm nothing was missed and
turns every remaining gap into a decision on record. It ranks below the two
reported defects only because those are what the user is blocked on today.

**Independent Test**: Read the inventory, pick any surface at random, exercise it
in the running application against the fixture set, and confirm the inventory's
claim about it.

**Acceptance Scenarios**:

1. **Given** the completed inventory, **When** a reviewer looks for any place in
   the main application that combines a file name with other text for display,
   **Then** it appears in the inventory with a verdict of verified-correct,
   fixed, or deferred with a stated reason.
2. **Given** a surface the inventory found broken by the same defect as the two
   reports, **When** the feature ships, **Then** it is fixed and its fix is
   verified in the running application.
3. **Given** any surface the inventory marks "verified correct", **When** it is
   exercised against the fixture set, **Then** it does display correctly.
4. **Given** any surface the inventory marks "deferred", **When** a reader
   consults it, **Then** the reason and the conditions for revisiting it are
   stated, so the deferral is a decision on record rather than an oversight.
5. **Given** the plugins shipped with the application, **When** the feature
   ships, **Then** each has been reviewed for this defect and its state recorded,
   and none has been modified.

---

### Edge Cases

- A name whose characters lie outside the machine's legacy codepage — the
  reported case, and the primary one.
- A name mixing representable and non-representable characters
  (`emoji-🙂-dir - Copyě 😍😍😍`) — no character may be lost, and the
  representable ones must not regress.
- Localized text and a file name in the same message — the case that broke
  Report 2. Both halves must be correct simultaneously, in every shipped
  language.
- A name containing an unpaired surrogate or otherwise uninterpretable sequence —
  exactly one `�` per offending character, nothing else affected.
- A name at or beyond the classic path length limit — long-path results must keep
  displaying as they do today.
- A Find results list with tens of thousands of rows — scrolling stays as
  responsive as it is today, with no visible per-row cost.
- A machine whose legacy codepage is not Central European — the fix must not be
  specific to one codepage, and must be correct where even `ě` is
  unrepresentable.
- Sorting Find results by the Name column — order must follow the real names, not
  the substituted ones.
- Copying a found item's name to the clipboard, and opening or focusing a found
  item in a panel — the name used must be the real one.
- A name long enough to be truncated in a notice or a column — truncation must
  not split a character in half.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The Find results list MUST display every file and directory name
  exactly as the file system stores it, for all scripts and symbols, with no
  character substituted or dropped.
- **FR-002**: No display surface MAY pass a file name through a conversion that
  can lose characters. A name that cannot survive a display route is a defect in
  the route, not a case to be absorbed by substitution. This does not conflict
  with FR-005: FR-002 governs a route too narrow to carry a well-formed name
  (never acceptable), FR-005 governs stored data that is itself malformed (where
  `U+FFFD` is the correct answer). A `?` produced by a narrow route and a `�`
  produced by malformed input are different outcomes with different causes.
- **FR-003**: The duplicate-name notice MUST show the name exactly as stored,
  while its surrounding localized text remains correct in every shipped language.
- **FR-004**: Where a file name is combined with localized text into one message,
  the whole message MUST be composed in a single encoding, so that neither half
  can corrupt the other.
- **FR-005**: When a stored name contains a sequence that genuinely cannot be
  interpreted, the system MUST show `�` for that character only, leaving the rest
  of the name and the rest of the message intact — the behaviour established by
  feature 041 and what Windows Explorer shows.
- **FR-006**: Type-to-search within the Find results list MUST match against the
  real names, so typing the leading characters of any visible item — including
  non-ASCII ones — selects it.
- **FR-007**: Sorting Find results by name MUST order by the real names.
- **FR-008**: Names carried out of the results list — to the clipboard, to a
  panel when opening or focusing a result, and to any action invoked on a result
  — MUST be the real names, unaffected by how they are displayed.
- **FR-009**: The feature MUST produce a written inventory of every place in the
  main application where a file name is composed into displayed text, each marked
  verified-correct, fixed, or deferred with a stated reason and revisit
  condition. The inventory MUST be produced before the repairs it drives, so its
  size is known before the work is committed to.
- **FR-009a**: The inventory MUST be built along two axes. A mechanical pass over
  the source enumerates every site where a name reaches a display route and
  carries the completeness argument. A walk of the user-reachable surfaces
  against the fixture set cross-checks that enumeration, and any surface the walk
  finds that the mechanical pass missed MUST be added to the inventory *and* the
  pattern that missed it recorded, so the gap in the method is visible.
- **FR-009b**: Each inventory entry MUST record which axis found it and how its
  verdict was reached, so a reader can tell an entry confirmed in the running
  application from one classified by inspection alone.
- **FR-009c**: Each inventory entry MUST record whether the surface composes
  localized text together with a name or displays a bare name, because that
  classification determines its verification cost: the former is verified in all
  9 shipped languages, the latter once.
- **FR-010**: Every surface the inventory finds broken by the same defect as the
  two reports MUST be fixed, and each fix MUST be verified in the running
  application. A surface found broken by a different cause MUST be recorded with
  that cause and is out of scope here.
- **FR-011**: The plugins shipped with the application MUST each be reviewed for
  this defect and their state recorded. No plugin may be modified by this
  feature — they keep the behaviour they have today.
- **FR-012**: All behaviour correct today MUST remain correct: plain ASCII names,
  accented names already representable on the machine's codepage, long-path
  results, the Find Path column, the searched-directory progress text, and every
  dialog and notice in every shipped language.
- **FR-013**: The surfaces repaired by feature 041 — the panel information line
  and the selection summary — MUST be re-verified after this change and shown to
  be unaffected.
- **FR-014**: The plugin interface MUST remain unchanged, in both shape and
  behaviour. No plugin may require recompilation or modification because of this
  feature, and no plugin's output may *look* different afterwards. A display
  entry point shared with plugins MUST keep its current interpretation for plugin
  callers.
- **FR-014a**: Where a shared entry point must behave differently for the main
  application, that difference MUST be opted into explicitly at each call site —
  never by changing the shared default. A caller that does not opt in MUST behave
  exactly as it does today.
- **FR-015**: Every fix MUST be verified in the running application against the
  reported scenarios and the fixture set, and the evidence recorded, before the
  feature is considered complete. A surface repaired under FR-010 counts as done
  only once it has been exercised in the running application.
- **FR-016**: The feature MUST leave behind automated protection against
  recurrence, consisting of both:
  (a) regression tests in the existing test project covering the composition and
  conversion behaviour these fixes depend on; and
  (b) a mechanical source check that fails when a forbidden pattern reappears —
  a name passed through a conversion that can lose characters, or a name composed
  into a message built in a different encoding.
  A future change that reintroduces either reported defect MUST fail
  automatically rather than ship. The check MUST cover every name-carrying
  display path, because SC-010 rests on it: proving no legacy-codepage conversion
  exists is what makes codepage independence a property of the design rather than
  a claim about one machine.
- **FR-017**: The automated protection of FR-016 MUST itself be demonstrated to
  work: reintroducing each reported defect MUST make it fail. A guard that has
  never been observed failing is not evidence.

### Key Entities

- **Stored name**: A file or directory name as the file system holds it. Correct
  by definition; the reference every display is checked against.
- **Display surface**: Any place a stored name is turned into text the user sees
  — a list column, a notice, a caption, a status line. The unit the inventory
  (FR-009) enumerates.
- **Composed message**: Localized text with a name spliced into it. The
  construct that failed in Report 2, and the one FR-004 governs.
- **Found item**: One result of a Find search — name, containing path, size,
  date, time, attributes. The name is what this feature repairs; the other fields
  are in scope only in that they must not regress.
- **Fixture set**: `C:\Users\pavel\AppData\Local\Temp\salamander-test` and its
  `010` subtree — Latin with diacritics, Greek, Cyrillic, CJK, emoji, a
  long-path subtree, and the canonically-equivalent `č-dir` pair that triggers
  the duplicate-name notice.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Both reported scenarios reproduce zero defects — the four found
  names match the file system exactly with no stray `?`, and the duplicate-name
  notice reads `č-dir` with no `Ä`/`Ã` sequence anywhere.
- **SC-002**: Across the fixture set, 100% of names shown by any surface touched
  by this feature display identically to the same names in a file panel — no
  exceptions, no script-dependent failures.
- **SC-003**: Type-to-search selects the intended item on the first attempt for
  100% of fixture names, ASCII and non-ASCII alike.
- **SC-004**: Every surface that composes localized text together with a name is
  correct in all 9 shipped languages, with the localized text and the name
  simultaneously right in each — including both reported surfaces. Surfaces
  showing a bare name are verified once, language being irrelevant to them.
- **SC-005**: Zero regressions — every scenario verified by feature 041 (the
  reported file, the 11 script fixtures, the size-threshold cases, the
  unpaired-surrogate case, the selection summary in all shipped languages, the
  truncation sweep) passes unchanged after this feature.
- **SC-006**: The inventory accounts for every place in the main application that
  composes a name into displayed text; a reviewer spot-checking any three entries
  finds the recorded verdict correct in all three.
- **SC-006a**: The user-interface walk finds no affected surface that the
  mechanical pass missed — or, where it does, every such surface is added to the
  inventory and the blind spot in the search method is recorded alongside it.
- **SC-007**: Every surface the inventory marked as carrying the reported defect
  displays names correctly afterwards — 100% of them, each demonstrated in the
  running application, none left "fixed but unverified".
- **SC-008**: Every shipped plugin appears in the review record with a stated
  finding, and the plugin binaries are unchanged by this feature.
- **SC-008a**: A plugin exercised before and after the change produces visually
  identical message boxes and dialogs — demonstrated on at least one plugin that
  displays file names, so "plugin behaviour unchanged" is observed rather than
  assumed.
- **SC-009**: The Find results list scrolls a 10,000-item result set with no
  perceptible delay, unchanged from today.
- **SC-010**: Correctness does not depend on the machine's regional settings.
  This is established structurally, not by sampling locales: no name passes
  through a legacy-codepage conversion anywhere (FR-002), and the FR-016 guard
  demonstrates the absence of any such conversion on a name path. The criterion
  is met when the guard covers every name-carrying display path and passes.
- **SC-011**: A user can read any displayed name, copy it, and use it to locate
  the file — the displayed text is always the real name.
- **SC-012**: Reverting either reported fix in isolation makes the automated
  protection fail, demonstrated for both — so the guard is known to detect the
  defect rather than assumed to.
- **SC-013**: The automated protection runs as part of the ordinary build and
  test cycle, requiring no manual step a future contributor could skip or forget.

## Assumptions

- **The `U+FFFD` policy follows feature 041**: substitute the replacement
  character for the offending character only, never for a whole field or message.
  That decision was taken and validated in 041 and is not reopened.
- **The search engine is correct and is not touched.** The report confirms the
  right items are found; only their presentation is wrong. Search matching,
  filters and traversal are out of scope.
- **The duplicate-name detection itself is correct and is not touched.** The
  notice fires on the right condition; only its rendering of the name is wrong.
- **The emoji shown as a hollow box in the "Names" input field of
  `find_filenames.png` is font fallback, not an encoding defect** — the search
  term reached the engine intact, as the correct results prove. It is not treated
  as part of this defect.
- **Fixtures already exist** under
  `C:\Users\pavel\AppData\Local\Temp\salamander-test` and are sufficient for
  verification, including the canonically-equivalent pair needed for User
  Story 2.
- **Verification happens in the running application**, not by inspection alone.
  The instruction "Všechny opravy verifikuj" is treated as a hard requirement
  (FR-015), consistent with how feature 041 was closed out.
- **Localized builds are part of verification, not an afterthought.** During 041
  an attempted broader change broke the Find dialog and was reverted; Report 2 is
  a defect only visible when localized text and a name meet. Every change here is
  checked in a localized user interface, not only in English.
- **This feature does not attempt the application-wide text-encoding change**
  that feature 041 identified as needed and deferred. That remains its own future
  feature. This one repairs the two reported surfaces and every main-application
  surface the inventory shows to have the same defect, and records the rest — see
  Clarifications for the exact boundary. The deferral thereby becomes a decision
  on record instead of a surprise.
- **The inventory comes before the repairs it drives** (FR-009). If it turns out
  larger than expected, that is a finding to act on — the scope can be re-cut
  with the real number in hand rather than discovered halfway through.

## Dependencies

- Builds on feature 041 (`041-fix-infoline-encoding`), which established the
  `U+FFFD` substitution policy, the lenient display conversion, and the
  single-encoding message composition helper that Report 2 needs — and which last
  modified the code path behind Report 1.
- Builds on feature 004 (`004-long-paths-unicode`), which introduced the modern
  display route the Find results list is supposed to be using, and which
  introduced the duplicate-name notice of Report 2.
- Builds on feature 005 (`005-fix-unicode-display`), which established how names
  are presented in dialogs.
- Requires the fixture directory to remain in place for verification.
