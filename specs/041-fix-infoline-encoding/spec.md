# Feature Specification: Fix Information Line Encoding

**Feature Branch**: `041-fix-infoline-encoding`
**Created**: 2026-07-26
**Status**: Draft
**Input**: User description (from `features/oprava_kodovani_spodni_panel.md`): "Oprav chybu zobrazení názvu souboru ve spodním panelu. Pokud název souboru obsahuje např. diakritiku, např. soubor: `Epizoda IV – Nová naděje (Despecialized) - původní kinodabing  CZ dabing.mkv`, ve spodním panelu zobrazí špatně, viz soubor ./temp/spodni_panel.png."

## Problem Statement

The information line at the bottom of a panel shows the focused item's details.
When the item's name contains characters outside plain ASCII, the whole line is
rendered as mojibake.

Reported (`temp/spodni_panel.png`), focused file
`Epizoda IV – Nová naděje (Despecialized) - původní kinodabing  CZ dabing.mkv`:

```text
Epizoda IV â€" NovÃ¡ nadÄ›je (Despecialized) - pÅ¯vodnÃ­ kinodabing  CZ dabing.mkv: 1 948 456 197, …
```

The garbling is the classic signature of UTF-8 bytes being drawn as if they were
single-byte ANSI text: `–` → `â€"`, `á` → `Ã¡`, `ě` → `Ä›`, `ů` → `Å¯`,
`í` → `Ã­`.

### Why the panel above is correct but the line below is not

The file panel renders each column as its own string, so the name column
contains nothing but the name — well-formed text that displays correctly. The
information line concatenates everything into a **single** string: name, size,
date, time, attributes. If any one of those parts is not in the application's
text encoding, the entire line — including the name — falls back to the legacy
rendering path and every non-ASCII character in it is destroyed.

On this machine the poisoning part is the **thousands separator** in the size.
The Czech locale's group separator is a non-breaking space, which the
application obtains from the operating system in the legacy single-byte form.
That single byte cannot occur on its own in the application's text encoding, so
the line is rejected as malformed and the fallback takes over. The size
`1 948 456 197` is therefore not merely cosmetic — it is what breaks the
file name next to it.

This also means the defect is **data-dependent and locale-dependent**, not
specific to one file:

- any locale whose number, date or time formats contain a non-ASCII character
  can poison the line;
- any file name with diacritics then renders as garbage;
- a file smaller than 1000 bytes has no thousands separator, so the same file
  name may display correctly in one directory and wrongly in another.

### A second, less obvious manifestation

The same line is also used for the selection summary ("N bytes in M files, K
directories selected"). That text combines a localized message with the same
number formatting, so in a localized user interface the summary's own words are
expected to be garbled too, even when no file name is involved.

## Clarifications

### Session 2026-07-26

- Q: Should the fix cover only the information line, or also other surfaces broken today by the same cause? → A: Fix the cause, and include any other surface a short targeted investigation shows to be broken by it. Not an open-ended audit — the investigation is bounded and whatever it finds is either fixed or explicitly recorded as not affected.
- Q: The shared number formatting is also reachable by plugins. May its behaviour change for them too? → A: Yes — fix it for every caller including plugins (the plugin ABI itself is unchanged, only the encoding of the returned content), **and** additionally review the plugins in this repository for any that depend on the previous behaviour, fixing whatever the review finds.
- Q: What does the line show when a field genuinely cannot be represented? → A: Replace only the offending characters with the standard replacement character (`U+FFFD`), leaving the rest of that field and the whole rest of the line intact. Nothing is silently dropped, and the behaviour matches what Windows Explorer shows for such a name.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - File names with diacritics read correctly (Priority: P1)

A user browses a directory containing files whose names use diacritics, dashes
or any other non-ASCII characters, and moves the focus onto one. The
information line at the bottom of the panel shows the name exactly as the file
system stores it, followed by its size, date, time and attributes.

**Why this priority**: This is the reported defect. The information line is one
of the two places a user reads the full name of the focused item, and today it
is unreadable for a large share of real-world files — anything with a Czech,
German, French, Polish or other accented character, or a typographic dash.

**Independent Test**: Focus the reported file in a panel and read the
information line. The name must match the panel above it character for
character.

**Acceptance Scenarios**:

1. **Given** the focused file is
   `Epizoda IV – Nová naděje (Despecialized) - původní kinodabing  CZ dabing.mkv`,
   **When** the user looks at the information line, **Then** the name is shown
   exactly, with `–`, `á`, `ě`, `ů`, `í` intact and no `Ã`, `Ä`, `Å` or `â€`
   sequences anywhere in the line.
2. **Given** any focused item, **When** the user compares the name in the
   information line with the name in the panel, **Then** the two are identical.
3. **Given** a file larger than 999 bytes in a locale whose thousands separator
   is not a plain space, **When** the user focuses it, **Then** both the size
   and the name are shown correctly.
4. **Given** a file smaller than 1000 bytes with an accented name, **When** the
   user focuses it, **Then** the name is shown correctly (this case must not
   regress — it may already work today).
5. **Given** the user changes the focus between several items with accented
   names, **When** each becomes focused, **Then** every one of them displays
   correctly.
6. **Given** a name containing a character that genuinely cannot be
   represented, **When** the user focuses it, **Then** only that character is
   shown as `�` and the rest of the name, the size, the date, the time and the
   attributes are all still correct.

---

### User Story 2 - The selection summary reads correctly (Priority: P2)

A user selects several files and reads the summary in the same line: how many
bytes, in how many files and directories.

**Why this priority**: Same line, same root cause, and in a localized user
interface the summary's own words are affected even without any file name. It
is a second visible symptom rather than a separate defect, so it ranks below
the reported case but must be fixed with it.

**Independent Test**: In a localized user interface, select files totalling more
than 999 bytes and read the summary.

**Acceptance Scenarios**:

1. **Given** a localized user interface, **When** the user selects files
   totalling more than 999 bytes, **Then** the summary text is fully readable in
   that language, with no garbled characters.
2. **Given** the same selection, **When** the user reads the byte count,
   **Then** the number and its separators are formatted as the operating
   system's regional settings prescribe.

---

### Edge Cases

- **Sizes below the separator threshold.** A file under 1000 bytes produces no
  thousands separator. Such names may already display correctly today, and must
  continue to.
- **Locales whose date or time format contains non-ASCII characters.** The size
  is the trigger observed in the report, but the date and time parts are
  obtained the same way and can carry the same problem in other regional
  settings.
- **Size shown in kilobytes/megabytes.** That formatting uses a decimal
  separator, obtained the same way as the thousands separator, and can poison
  the line identically.
- **A user-defined information line layout.** The content of this line is
  configurable; the fix must hold for any arrangement of the available fields,
  not only the default one.
- **Names the file system permits but the display cannot represent.** Such a
  name shows the standard replacement character in place of the offending
  characters only (FR-003a); the surrounding text and the other fields stay
  correct. This is what Windows Explorer does for the same name, so the two
  agree.
- **Archives and plugin file systems.** When browsing inside an archive or a
  plugin file system, the line's content may come from the plugin instead. The
  reported defect is on a real disk directory; behaviour inside plugins must at
  minimum not regress.
- **Very long names.** The line truncates with an ellipsis. Truncation must
  never split a character — a replacement character in the line must always
  mean the name really contains something unrepresentable (FR-003a), never that
  the line was cut in the wrong place.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The information line MUST display the focused item's name exactly
  as the file system stores it, for any name the file system permits.
- **FR-002**: The name shown in the information line MUST be character-for-
  character identical to the name shown for the same item in the panel above.
- **FR-003**: No part of the information line may cause another part to be
  rendered incorrectly. A field the application cannot represent MUST NOT
  corrupt the remaining fields.
- **FR-003a**: A character that genuinely cannot be represented MUST be shown
  as the standard replacement character (`U+FFFD`, `�`). Only the offending
  characters are replaced — the rest of that field, and the whole rest of the
  line, MUST still display correctly. Nothing may be silently dropped.
- **FR-004**: The size, date and time in the information line MUST be formatted
  according to the operating system's regional settings, and MUST themselves be
  displayed correctly — including separators that are not plain ASCII.
- **FR-005**: The selection summary shown in the same line MUST be fully
  readable in every shipped user-interface language.
- **FR-006**: FR-001 through FR-005 MUST hold regardless of the operating
  system's regional settings, and regardless of which fields the user has
  configured the information line to show.
- **FR-007**: Truncating the line to fit the available width MUST NOT split a
  character.
- **FR-008**: No other place that displays numbers, dates, times or file names
  may regress as a side effect. This explicitly includes the panel columns, the
  directory line, dialogs that report sizes and counts, and the same line when
  its content is supplied by a plugin.
- **FR-009**: The fix MUST NOT depend on the user's regional settings being
  changed, on a particular code page being active, or on any new configuration
  option. The user does nothing and the line is correct.
- **FR-010**: The defect MUST be fixed at its cause, not patched at the one
  observed symptom, so that any other display fed by the same locale-derived
  text is corrected along with the information line.
- **FR-011**: A bounded investigation MUST establish which other surfaces
  combine locale-derived text with user-supplied or localized text in one
  string. Every surface it identifies MUST be either fixed in this feature or
  explicitly recorded as unaffected, with the reason. Surfaces the
  investigation does not identify are out of scope; this is not an
  open-ended audit of the whole application.
- **FR-012**: The corrected formatting MUST apply to every caller, including
  plugins that obtain it through the published plugin interface. The plugin
  interface itself — its shape, its version and its binary compatibility —
  MUST NOT change; only the encoding of the content it returns does.
- **FR-013**: The plugins in this repository MUST be reviewed for any that
  depend on the previous behaviour of that formatting, and any such dependency
  MUST be fixed as part of this feature. The review's outcome MUST be recorded
  per plugin, including those found not to be affected.

### Key Entities

- **Information line**: the single line of text below a panel showing the
  focused item's details, or a summary when items are selected. Its layout is
  user-configurable.
- **Item name**: the file or directory name as the file system stores it. The
  reference value against which the display is judged.
- **Locale-derived text**: the number separators and the date and time formats
  the application obtains from the operating system's regional settings. These
  are the parts observed to be in the wrong encoding.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: The file named in the report displays its name correctly in the
  information line — 0 garbled characters, where today every non-ASCII
  character in the line is garbled.
- **SC-002**: For a directory of test files covering diacritics, typographic
  dashes and mixed scripts, 100% of names shown in the information line match
  the names shown in the panel.
- **SC-003**: The defect does not reappear when the file size crosses the
  thousands-separator threshold: the same accented name displays correctly at
  999 bytes and at 1 000 000 bytes.
- **SC-004**: In every shipped user-interface language, the selection summary
  for a multi-file selection is fully readable — 0 garbled characters.
- **SC-005**: Sizes, dates and times in the information line match what the
  operating system's regional settings prescribe, verified against at least one
  locale whose separators are not plain ASCII.
- **SC-006**: No regression in any other display of names, numbers, dates or
  times — verified across the panel columns, the directory line and the
  size-reporting dialogs.
- **SC-007**: Every surface the FR-011 investigation identifies is accounted
  for: 0 surfaces left in the "known broken by this cause but not addressed"
  state at the end of the feature.
- **SC-008**: Every plugin in this repository has a recorded review outcome
  (affected and fixed, or not affected and why) — 0 plugins unreviewed.
- **SC-009**: The plugin interface version is unchanged and every shipped
  plugin still loads and runs against the rebuilt application.

## Assumptions

- **The reported "spodní panel" is the panel's information line** — the line
  showing `<name>: <size>, <date>, <time>, <attributes>` directly above the
  command line, as in the supplied screenshot. The command line and the
  function-key bar below it are not part of this feature.
- **The correct value is what the file system stores.** The panel already shows
  it correctly, so the panel is the reference for what the information line must
  match.
- **Regional settings are the operating system's to decide.** This feature
  makes the line display them correctly; it does not change how numbers, dates
  or times are formatted, and adds no option to override them.
- **The line's configurability stays as it is.** No field is added, removed or
  reordered, and the default layout is unchanged.
- **Plugin-supplied content is out of scope as a source.** When a plugin
  composes the line's content itself, the plugin owns the encoding of what it
  supplies. This feature must not make such cases worse. Note this is distinct
  from FR-012/FR-013, which concern the shared formatting the application hands
  *to* plugins — that is in scope.
- **Backward compatibility is judged at the interface, not the bytes.**
  Constitution principle II protects the plugin interface's shape and binary
  compatibility. Correcting the encoding of a string the interface returns is a
  bug fix within that contract, not a break of it: the previous bytes were
  malformed for any non-ASCII locale, so no correct plugin could have depended
  on them.
- **Test material exists.** `temp/` already contains the reported file, so the
  primary scenario is reproducible without creating fixtures; additional names
  covering other scripts will be needed for SC-002.
