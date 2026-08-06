# Feature Specification: SFTP Dialogs — Ephemeral Quick Connect, Empty Bookmarks, Untruncated Localized Texts

**Feature Branch**: `053-sftp-connect-dialog`
**Created**: 2026-08-06
**Status**: Draft
**Input**: User description: "Nyni provedeme úpravy v připojovacím pop-up okně pro SFTP plugin (CTRL+SHIFT+S). U rychlého připojení se z principu nebude nic ukládat do příště - opravdu to slouží poze k rychlému přopojení, tedy je nutné vždy vyplnit vše znovu. Rozhodně se nemohou ukládat hesla. U rychlého připojení tedy volba pro uložení hesel bude zakázána. Naopak při vytvoření nové položky není nutnné nic vyplňovat, novou položku půjde vytvořit čistě jako prázdnou - nyní je vyžadována adresa. Dále opravíme překlady - především v češtině. Položka \"Soubor s\" bude \"Soubor\", položka \"Heslo ke\" bude \"Heslo\", položka \"Počáteční\" bude \"Výchozí adresář\", stejně tak oprav i případné další jazyky."

## Problem Statement

The SFTP plugin's connect dialog (Ctrl+Shift+S) has three separate defects.
The first two are specific to that dialog; the third turned out to affect the
plugin's other dialogs as well:

1. **Quick Connect remembers everything, including secrets.** It behaves like
   a hidden bookmark: the address, user, key file, path and even the *password*
   are carried over to the next session. That contradicts what quick connect
   is for — a one-off connection — and storing its password is a needless
   secret at rest for a connection the user never asked to save.
2. **A new bookmark cannot be created empty.** Creating a bookmark is refused
   unless an address is filled in, so a user cannot set up an entry name first
   and complete the details later.
3. **Labels are cut off throughout the plugin's dialogs.** Controls have a
   fixed width sized for English, so longer translations are visually truncated
   mid-word — in Czech the key-file label reads "Soubor s", the passphrase
   label "Heslo ke", the initial-path label "Počáteční". It is not limited to
   the connect dialog: measured across the eight shipped languages, **26
   distinct controls in six of the plugin's nine dialogs** are too narrow for
   their text (connect, configuration, permissions, host key, symlink,
   owner/group; the rename and logs dialogs are fine). The worst cases lose more
   than a third of the text — a configuration checkbox has 118 units for text
   needing about 208. Two further display defects came out of the same
   measurement: the password prompt overflows once its message is composed at
   run time, and the configuration dialog contains two controls that already
   overlap. The translated texts themselves are correct; only their display is
   wrong.

## Clarifications

### Session 2026-08-06

- Q: Should the fix change the translated label texts (shorter wording), or
  only their display? → A: Display only — the localized texts stay as they
  are; the dialog must show each one in full.
- Q: Where should the dialog get the room for the full label texts? → A: Widen
  the dialog — the label column grows, input fields shift right and keep their
  current width, so the whole dialog becomes wider (never at the cost of field
  usability).
- Q: Does the display fix cover only the connect dialog, or every SFTP plugin
  dialog? → A: Every dialog of the SFTP plugin. Measurement across all eight
  shipped languages then put the real scope at 26 distinct controls in six of
  the nine dialogs (connect, configuration, permissions, host key, symlink,
  owner/group).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Quick Connect leaves nothing behind (Priority: P1)

A user opens the connect dialog, picks Quick Connect, types a host, a user
name and a password, and connects. When they later open the dialog again — in
the same session or after restarting the application — Quick Connect is empty:
nothing they typed was kept, and no password from it exists anywhere in stored
settings. Saving a password is not even offered for Quick Connect.

**Why this priority**: This is both the behaviour the user asked for and a
security improvement — a secret that is never stored cannot leak. Everything
else in this feature is convenience or cosmetics.

**Independent Test**: Use Quick Connect with a password, close the
application, reopen it, open the dialog and select Quick Connect — all fields
are empty and the save-password option is unavailable; inspecting stored
settings shows no Quick Connect data.

**Acceptance Scenarios**:

1. **Given** Quick Connect is selected, **When** the user looks at the
   password and passphrase save options, **Then** both are visibly unavailable
   and cannot be turned on.
2. **Given** a user connected via Quick Connect with all fields filled,
   **When** they reopen the dialog and select Quick Connect, **Then** every
   field is empty (no address, user, key file, path or secret).
3. **Given** a user connected via Quick Connect, **When** the application is
   closed and restarted, **Then** no Quick Connect values — and specifically
   no password or passphrase — are present in stored settings.
4. **Given** an installation whose settings already contain Quick Connect
   values saved by an earlier version, **When** the user runs the updated
   version, **Then** those values (including any stored secret) are removed
   without the user doing anything.
5. **Given** a saved bookmark with a stored password, **When** the user
   switches between it and Quick Connect, **Then** the bookmark keeps its own
   stored password and its save options remain available.

---

### User Story 2 - A bookmark can be created empty (Priority: P2)

A user wants to prepare a bookmark before knowing the server details. They
create a new bookmark, give it a name, and it is created — with no address, no
user and no other detail filled in. They can complete it later, and only when
they actually try to connect are the missing details required.

**Why this priority**: Removes an arbitrary obstacle in a workflow the user
described; independent of US1 and of the label work.

**Independent Test**: Create a new bookmark with all fields empty, give it a
name, confirm it appears in the list and survives closing and reopening the
dialog; then select it and attempt to connect and confirm the missing address
is reported at that point.

**Acceptance Scenarios**:

1. **Given** all connection fields are empty, **When** the user creates a new
   bookmark and names it, **Then** the bookmark is created and appears in the
   list under that name — with no error about a missing address.
2. **Given** an empty named bookmark exists, **When** the dialog is closed and
   reopened, **Then** the bookmark is still there with its name and empty
   fields.
3. **Given** an empty bookmark is selected, **When** the user tries to
   connect, **Then** the missing address is reported and no connection is
   attempted.
4. **Given** an empty bookmark is selected, **When** the user fills in the
   details and saves, **Then** the bookmark keeps them.

---

### User Story 3 - Localized texts are fully visible in every SFTP dialog (Priority: P3)

A user with the Czech (or any other shipped) UI opens any of the SFTP
plugin's dialogs — connect, configuration, permissions, symlink — and reads
every label, checkbox and button caption in full, with no word cut off
mid-way. The translated texts are unchanged; the dialogs simply give them the
room they need, so "Soubor s klíčem:" and "Maximální velikost protokolu (KB):"
read as the complete texts they already are.

**Why this priority**: Cosmetic and comprehension, not function; but the
truncation makes texts genuinely ambiguous — "Soubor s" and "Heslo ke" read as
unfinished sentences, and one configuration label loses nearly half its text.

**Independent Test**: Open each SFTP dialog in Czech and visually confirm every
text renders completely; repeat for each other shipped language and confirm
nothing is truncated there either.

**Acceptance Scenarios**:

1. **Given** the Czech UI, **When** the user opens the connect dialog,
   **Then** every label is shown in full — including "Soubor s klíčem:",
   "Heslo ke klíči:" and "Počáteční cesta:" — with no clipped characters.
2. **Given** the Czech UI, **When** the user opens the plugin's configuration,
   permissions and symlink dialogs, **Then** every label, checkbox and caption
   in them is shown in full as well.
3. **Given** any shipped UI language, **When** the user opens any SFTP dialog,
   **Then** no text in it is visually truncated — each is fully readable,
   including its keyboard-accelerator underline.
4. **Given** the adjusted dialogs, **When** the user uses keyboard
   accelerators, **Then** each accelerator still reaches its own control and no
   two controls in a dialog share one.
5. **Given** the adjusted dialogs, **When** they are displayed in any shipped
   language, **Then** controls stay aligned, none overlaps another, and input
   fields are no narrower than they are today.

---

### Edge Cases

- Quick Connect selected while a master password is configured: no secret is
  stored, so the user must never be prompted to unlock a password store on
  behalf of Quick Connect.
- The user switches from a bookmark (with save options on) to Quick Connect
  and back: the save options must follow the selected entry, never leak the
  bookmark's state into Quick Connect or vice versa.
- Quick Connect used repeatedly within one session: each new open starts
  empty; the previous attempt is not offered again, even after a failed
  connection.
- An empty bookmark that is duplicated or renamed stays valid and empty.
- A bookmark whose name is left blank: the name is the only thing that
  identifies an otherwise-empty entry, so blank names are rejected as they are
  today.
- Texts in languages much longer than English (French is worst at 23 clipped
  controls, then Spanish and Romanian) must fit as well, so the fix cannot be
  sized to Czech alone.
- Non-Latin-script languages (Russian, Ukrainian, Chinese) are currently not
  shipped but their translation source is retained; the fix should not make
  re-enabling them harder.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Quick Connect MUST NOT persist any of its values between
  openings of the dialog or between application runs; its fields MUST start
  empty every time it is selected.
- **FR-002**: Quick Connect MUST NOT store a password or a passphrase
  anywhere, and the options to save them MUST be unavailable (visibly disabled
  and impossible to turn on) whenever Quick Connect is selected.
- **FR-003**: A secret typed for a Quick Connect session MUST be usable for
  that connection attempt and MUST NOT outlive it.
- **FR-004**: On first run of the updated version, any Quick Connect values
  left in stored settings by an earlier version — including stored secrets —
  MUST be removed automatically, with no user action.
- **FR-005**: Saved bookmarks MUST keep their existing behaviour: their
  values persist and their save-password/passphrase options remain available
  and effective.
- **FR-006**: Creating a new bookmark MUST succeed with every connection field
  empty; only a non-empty bookmark name is required.
- **FR-007**: Connection details (address, valid port) MUST be required at the
  moment the user attempts to connect, not when a bookmark is created or
  saved.
- **FR-008**: The translated texts MUST NOT be reworded by this feature; the
  existing localized wording is kept and only its display is corrected.
- **FR-009**: No text in any SFTP plugin dialog may be visually truncated in any
  shipped UI language; each MUST be fully readable at the standard dialog font
  and DPI. This includes text the dialog composes at run time (the password
  prompt), not only text authored in the dialog.
- **FR-010**: No two controls in a dialog may overlap after the change (the
  configuration dialog contains such a pair today), keyboard accelerators MUST
  stay unique within each dialog, and each MUST still activate its own control.
- **FR-011**: Input fields MUST keep at least their current width; the room
  for the texts comes from making a dialog larger, not from shrinking its
  input fields.
- **FR-012**: The dialogs MUST remain in the product's house style — standard
  themed controls, fixed layout, labels beside their fields as today — so they
  stay visually consistent with the rest of the application.

### Key Entities

- **Quick Connect entry**: the always-present first choice in the connect
  dialog's entry list, representing a one-off connection; after this feature
  it holds no data beyond the currently open dialog.
- **Bookmark**: a named, persisted connection entry; may be empty apart from
  its name, and may carry stored secrets when the user opts in.
- **Dialog text**: any label, checkbox caption or button caption in an SFTP
  plugin dialog; exists per shipped language and must render completely.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: After any number of Quick Connect uses, stored settings contain
  zero Quick Connect values and zero secrets attributable to Quick Connect —
  verifiable by inspecting stored settings.
- **SC-002**: 100% of dialog openings show an empty Quick Connect form.
- **SC-003**: A user can create a named bookmark with zero fields filled in,
  in one attempt, with no error message.
- **SC-004**: In 100% of shipped UI languages, every text in every SFTP plugin
  dialog is fully visible — zero clipped controls, down from 26 today — nothing
  overlaps, no two controls in a dialog share a keyboard accelerator, and no
  input field is narrower than today.
- **SC-005**: An installation carrying Quick Connect data from an earlier
  version has none of it left after one run of the updated version, with zero
  manual steps.

## Assumptions

- "Nothing is saved" applies to the Quick Connect entry only. Bookmarks,
  host-key trust records, window geometry, and other plugin settings are
  unaffected — the user's wording contrasts quick connect with saved entries,
  not with all persistence.
- The port field may still show the protocol's default value for an empty
  entry; a default is not "remembered user input". Likewise an empty bookmark
  is allowed to carry that default port.
- The password/passphrase save options are *disabled* for Quick Connect rather
  than hidden, so the user can see that the choice exists but does not apply —
  consistent with how the dialog already greys out inapplicable controls.
- Which entry the dialog highlights when it opens is not changed by this
  feature; only Quick Connect's *field contents* must be empty. If the dialog
  reopens on Quick Connect, its fields are blank.
- Removing stale Quick Connect data from settings is treated as a security
  fix, not a migration: nothing is converted or preserved, the values are
  simply dropped.
- The truncation is structural, not a translation error: a control has a fixed
  width sized for English and its neighbour starts immediately to its right, so
  there is no free space a longer translation could occupy. Satisfying FR-009
  therefore requires the affected areas to fit the longest shipped translation.
- Scope is the SFTP plugin's own dialogs. Truncation elsewhere in the product
  is out of scope for this feature, and an automated product-wide guard against
  it is recorded as possible follow-up work rather than done here (it would
  report hundreds of pre-existing findings, which is its own feature).
- Per-language control geometry is data the build already produces from
  translation source; adjusting it is not "changing the localization" in the
  sense of FR-008, which concerns the wording only.
- Three defects found during measurement are deliberately **not** fixed here,
  because each would require changing text and therefore new or re-translated
  strings: three duplicate keyboard accelerators in the English connect dialog
  (all shipped languages are already free of duplicates), a hardcoded English
  "Close" caption the connect dialog sets at run time, and a hardcoded English
  "(unnamed)" fallback in the entry list. They are recorded for follow-up.
