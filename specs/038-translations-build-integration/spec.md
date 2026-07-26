# Feature Specification: Translations Build Integration

**Feature Branch**: `038-translations-build-integration`
**Created**: 2026-07-26
**Status**: Draft
**Input**: User description: "Cílem tohoto rozšíření je integrování všech dostupných překladů v ./translation do buildu tak, aby bylo možné v programu jazyky přepínat. Pro nové pluginy jako SFTP a MDView je potřeba nejprve překlady připravit (automaticky)."

## Overview

Newt Commander already contains everything needed to *run* in a language other
than English: the application scans a `lang` folder for language modules, offers a
language-selection dialog, remembers the choice, and asks plugins to load a
matching language module. What is missing is the *content* — the build currently
produces exactly one language module per shipped module (English), so the
selection dialog has nothing to choose from.

The repository ships translation source data for **10 languages** (Czech, Dutch,
French, German, Hungarian, Simplified Chinese, Romanian, Russian, Slovak,
Spanish) covering **23 modules**, but that data is not connected to the build in
any way. This feature connects it, fills the gaps in it, and extends it:

- existing human translations are wired into the build for every enabled module;
- everything they do not cover — the entire SFTP, MDView, Folders and Portables
  plugins, plus all text added to older modules since the historical data was
  produced — is translated automatically;
- **Ukrainian is added as a new, fully machine-translated language** derived from
  the English original;
- legacy product identity inside translated text is rewritten to Newt Commander
  and the predecessor vendor's web links are removed.

The result is a product the user can switch between **12 languages** (English plus
11 others), with every enabled plugin following the same choice.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Run the application in my own language (Priority: P1)

A user installs Newt Commander, opens the language chooser (either on first run
or later from the configuration), sees a list of all available languages with
their names and authors, picks one, and the entire application user interface —
menus, dialogs, messages, column headers, status texts — appears in that
language.

**Why this priority**: This is the headline outcome of the feature. Without the
main application being translated, nothing else matters; a translated plugin
inside an English shell is worse than no translation at all.

**Independent Test**: Build the product, launch it, open the language chooser,
confirm that all 12 languages are offered, select a non-English one, restart if
the product asks for it, and confirm the main window and a representative set of
dialogs are rendered in that language.

**Acceptance Scenarios**:

1. **Given** a freshly built product and no saved configuration, **When** the
   user starts the application for the first time, **Then** the language chooser
   lists all 12 shipped languages and each entry shows a readable language name.
2. **Given** the language chooser is open, **When** the user selects a language
   and confirms, **Then** the application user interface is presented in that
   language and the choice survives an application restart.
3. **Given** the user's Windows display language matches one of the shipped
   languages, **When** the application starts for the first time, **Then** that
   language is preselected without the user having to search for it.
4. **Given** any shipped language is active, **When** the user opens the
   configuration dialog, **Then** the currently active language is displayed and
   can be changed from there.

---

### User Story 2 - Plugins follow the language I chose (Priority: P2)

The user has switched the application to, say, German. When they open an archive,
compare files, browse an FTP site, or view a picture, every plugin's dialogs,
menu items and messages are also in German — without the application stopping to
ask which language each individual plugin should use.

**Why this priority**: Plugins provide much of the day-to-day surface area of the
product. Half-translated sessions (German shell, English archive dialogs) are the
single most visible quality problem this feature must avoid. It is separated from
P1 only because the application is already usable and demonstrably valuable with
P1 alone.

**Independent Test**: With a non-English language active, exercise each enabled
plugin and confirm no per-plugin language prompt is shown and its user interface
is in the chosen language.

**Acceptance Scenarios**:

1. **Given** the application runs in a shipped language, **When** any enabled
   plugin is loaded for the first time, **Then** the plugin's user interface
   appears in that same language and no language-selection prompt is shown for
   that plugin.
2. **Given** the application runs in a shipped language, **When** the user opens
   the plugin manager, **Then** every listed plugin reports the same active
   language.
3. **Given** the user changes the application language, **When** the application
   is restarted, **Then** all plugins pick up the new language automatically.

---

### User Story 3 - New plugins are translated too (Priority: P2)

SFTP, MDView, Folders and Portables were written after the historical translation
data was produced, so no human translation exists for them. The user must
nonetheless find them presented in the language they chose — an SFTP connect
dialog in Czech inside a Czech application — and must never be interrupted by a
"choose a language for this plugin" prompt.

**Why this priority**: SFTP and MDView are flagship Newt Commander plugins and are
enabled in the default build. A missing language module for them produces an
error prompt on every first load, which is a functional defect, not just a
cosmetic one. Shares P2 with User Story 2 because both close the same gap; it is
listed separately because it is delivered by a different mechanism and can be
shipped independently.

**Independent Test**: With a non-English language active, open the SFTP connect
dialog and the MDView viewer and confirm both are presented in that language,
with no prompt and no layout damage.

**Acceptance Scenarios**:

1. **Given** the application runs in any shipped language, **When** the SFTP or
   MDView plugin is loaded, **Then** a language module for the active language is
   found and loaded, and no prompt or error message appears.
2. **Given** a plugin whose translation was produced automatically, **When** the
   user opens its dialogs, **Then** the text is in the active language, every
   control is readable and fully inside the dialog, and every keyboard
   accelerator still works.

---

### User Story 4 - Ukrainian, produced from scratch (Priority: P3)

A Ukrainian-speaking user finds Ukrainian in the language chooser and can run the
whole product — application and every enabled plugin — in Ukrainian, even though
no Ukrainian translator has ever contributed to the project.

**Why this priority**: Adds reach to a new audience and proves the automatic
translation path end-to-end on a language with no human baseline at all. It is
P3 because the product is complete and shippable with the 11 other languages; the
mechanism it depends on is already delivered by User Story 3.

**Independent Test**: Select Ukrainian in the language chooser and run a full
working session across the application and each enabled plugin.

**Acceptance Scenarios**:

1. **Given** a default build, **When** the user opens the language chooser,
   **Then** Ukrainian is listed alongside the other languages and is
   selectable.
2. **Given** Ukrainian is active, **When** the user works with the application and
   any enabled plugin, **Then** the user interface is presented in Ukrainian
   throughout, with no per-plugin language prompt.
3. **Given** Ukrainian is active, **When** the user inspects the language details,
   **Then** the entry identifies the translation as machine-produced and points at
   where to contribute improvements.

---

### User Story 5 - One command rebuilds every language (Priority: P3)

A maintainer changes an English string, adds a dialog, or updates a plugin
version, then runs the ordinary build command. All language modules for all
languages and all enabled modules are regenerated, remain loadable by the
product, and the maintainer is told — in plain numbers — how much of each
language is human-translated and how much was produced automatically.

**Why this priority**: Delivers long-term maintainability rather than immediate
end-user value. The product ships correctly without it (languages could be
produced once by hand), but every subsequent change would silently rot them.

**Independent Test**: Run a full clean build twice and confirm identical language
outputs; then change one English string, rebuild, and confirm the affected
language modules are regenerated and the coverage report reflects the change.

**Acceptance Scenarios**:

1. **Given** a clean checkout, **When** the maintainer runs the standard full
   build command, **Then** every shipped language for every enabled module is
   produced without any manual step, extra tool download, or interactive prompt.
2. **Given** an English string is added or changed, **When** the build is run
   again, **Then** the affected language modules are rebuilt and the build
   reports the resulting translation coverage per language.
3. **Given** a module's version number changes, **When** the build is run,
   **Then** its language modules carry the matching version and are still
   accepted by the product at runtime.
4. **Given** a plugin is switched off in the build policy, **When** the build is
   run, **Then** no language modules are produced or shipped for that plugin,
   even if translation data exists for it.

---

### Edge Cases

- **Stale translation entries**: the historical data describes dialogs, menus and
  strings that no longer exist in the current product. These must be ignored
  without failing the build and must be reported as such.
- **Text that no longer fits**: an automatically produced or historical
  translation may be longer than the control that holds it, causing clipped or
  overlapping controls — especially in dialogs whose layout changed since the
  translation was made.
- **Accelerator collisions**: automatically produced text may assign the same
  keyboard accelerator to two items in one menu or dialog, breaking keyboard
  navigation.
- **Placeholders and formatting**: messages contain substitution slots and
  shortcut labels; an automatic translation that drops, reorders or corrupts them
  produces broken or misleading messages at runtime.
- **Version mismatch**: the product refuses to load a language module whose
  version does not exactly match the module it belongs to. Every produced
  language module must therefore be version-locked to the exact build it ships
  with, and a mismatch must be caught at build time, not by the end user.
- **Missing language for one plugin only**: if a single plugin lacks the active
  language, the product falls back to per-plugin selection. The shipped set must
  be complete enough that this never happens in a default build.
- **Translation data for disabled plugins**: seven modules with translation data
  (Automation, CheckVer, MMViewer, Nethood, UnCHM, UnMIME, UnRAR) are disabled in
  the default build policy and must be skipped silently.
- **Previously selected language disappears**: a user upgrading from a build
  where they selected a language that is no longer shipped must be guided to pick
  another language rather than shown a hard error.
- **Non-Latin scripts**: Russian, Ukrainian and Simplified Chinese text must
  render correctly in menus, dialogs, list columns and message boxes on a
  Latin-script Windows installation.
- **Legacy product identity in translated text**: the historical translations
  contain the previous product name, previous vendor name, and links to the
  previous vendor's forum, all of which must be removed or rewritten.
- **Inflected product name**: several target languages decline nouns, so the old
  product name may appear inside a sentence in a grammatical form that a plain
  substitution cannot handle cleanly.
- **Absent or partial translation source**: if translation data is missing or
  unreadable, the build must still produce a working English-only product and say
  clearly what was skipped.

## Requirements *(mandatory)*

### Functional Requirements

#### Shipping languages

- **FR-001**: The product MUST ship **12 languages**: English, the 10 languages
  that have human translation data (Czech, Dutch, French, German, Hungarian,
  Simplified Chinese, Romanian, Russian, Slovak, Spanish), and Ukrainian.
- **FR-002**: All 12 languages MUST ship regardless of how much of each is
  human-translated; coverage is reported (FR-014) but never used to withhold a
  language from users.
- **FR-003**: The product MUST ship a language module for the main application in
  every shipped language.
- **FR-004**: The product MUST ship a language module for **every enabled
  plugin** in **every shipped language**, so that the set of languages offered is
  identical for the application and for each plugin.
- **FR-005**: The product MUST NOT ship language modules for plugins that are
  disabled by the build policy.
- **FR-006**: Each shipped language module MUST carry the language name, author
  credit, and contact information that the language chooser displays, and MUST
  declare the language it represents so the product can match it against the
  user's Windows display language.
- **FR-007**: Each shipped language module MUST be version-locked to the exact
  application or plugin build it ships with, so the product accepts it at runtime.

#### Producing translations automatically

- **FR-008**: Every piece of user-visible text in the current product that has no
  human translation for a given language MUST be translated automatically into
  that language. This covers, in full, the SFTP, MDView, Folders and Portables
  plugins, all Ukrainian text for every module, and all text added to other
  modules since the historical translations were produced.
- **FR-009**: Ukrainian MUST be produced entirely by automatic translation from
  the English original, to the same completeness as any other shipped language.
- **FR-010**: Automatically produced translations MUST be stored as translation
  source in the repository alongside the human-authored data, so they are
  reviewable, diffable and correctable by hand.
- **FR-011**: Automatically produced translations MUST be marked as
  machine-produced and distinguishable from human-authored ones, so a translator
  can find and revise exactly those.
- **FR-012**: Automatically produced translations MUST preserve every non-textual
  element of the original text — keyboard accelerator markers, substitution
  placeholders, formatting sequences, escape sequences and shortcut-key labels —
  so that menus, keyboard navigation and formatted messages continue to work.
- **FR-013**: Dialogs whose text was replaced MUST remain fully usable: every
  control readable, no control clipped, overlapped, or pushed outside the dialog,
  in every shipped language.

#### Handling drift between translation data and the current product

- **FR-014**: The build MUST ignore translation entries that no longer correspond
  to anything in the current product, without failing.
- **FR-015**: The build MUST report, per language, how much of the product is
  human-translated, how much was produced automatically, and how many source
  entries were discarded as obsolete.
- **FR-016**: The product MUST continue to indicate to the user when the active
  language is not a fully human-reviewed translation, and point them at where to
  contribute improvements.
- **FR-017**: Human-authored translation data MUST remain the authoritative
  source: where a human translation exists it MUST be used in preference to an
  automatic one, and the build MUST NOT overwrite human-authored data.

#### Product identity in translated text

- **FR-018**: Every reference to the predecessor product name or predecessor
  vendor name MUST be replaced with the Newt Commander identity wherever it
  appears in shipped text — dialogs, menus, messages, and the metadata shown in
  the language chooser.
- **FR-019**: Every web address pointing at the predecessor vendor's sites,
  forums or support channels MUST be removed from shipped text and replaced with
  the Newt Commander project's own addresses.
- **FR-020**: The names of the original human translators MUST be preserved and
  credited; only their predecessor-vendor contact links are replaced with the
  Newt Commander project address.
- **FR-021**: Replacement of product and vendor names MUST produce grammatically
  acceptable text in each target language, including languages that decline nouns;
  where a mechanical substitution cannot, the surrounding phrase MUST be corrected.

#### Build behaviour

- **FR-022**: All language modules MUST be produced by the standard build
  command, with no additional manual step, no interactive prompt, and no
  dependency the repository does not already provide or install for itself.
- **FR-023**: Automatic translation MUST NOT run as part of an ordinary build:
  its results are prepared once, committed, and thereafter consumed by the build
  like any other translation source.
- **FR-024**: The build MUST be repeatable: two clean builds of the same sources
  MUST produce equivalent language modules.
- **FR-025**: If translation data is missing, incomplete or unreadable, the build
  MUST still succeed and produce a working English product, reporting exactly
  what was skipped.
- **FR-026**: The build MUST fail with a clear message if it produces a language
  module the product would reject at runtime (for example a version mismatch),
  rather than shipping it.

#### Delivery

- **FR-027**: All 12 languages MUST be included in the installable product, so
  that language switching works from a normal installation and not only from a
  developer build.

### Key Entities

- **Language**: One of the 12 languages the product can present itself in. Has a
  display name, a language identifier used to match against the user's Windows
  settings, an author credit, a contact link, and an origin (human-authored,
  machine-produced, or mixed).
- **Module**: A separately versioned unit of the product that owns its own user
  interface text — the main application, or one plugin.
- **Language module**: The shipped artefact that provides one Language's text for
  one Module. Version-locked to its Module. English is always present.
- **Translation source**: The translation data held in the repository for one
  Language and one Module, covering dialogs, menus and message strings plus
  translator credits. Entries are marked as human-authored or machine-produced.
- **Coverage report**: The build's per-language summary of how much text is
  human-translated, machine-produced, or discarded as obsolete.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A user can switch the product between any of the 12 shipped
  languages using only the in-product language chooser — no file editing, no
  reinstall.
- **SC-002**: The language chooser offers 12 languages, and offers the same 12
  languages for every enabled plugin.
- **SC-003**: In a default build with any non-English language active, a user can
  complete a normal working session — browse both panels, copy, move, delete,
  rename, search, open an archive, view a file, connect over SFTP, view a
  Markdown file, open the configuration dialog — without encountering a single
  language-selection prompt or missing-language error.
- **SC-004**: For each of the 12 shipped languages, at least 99% of the text a
  user encounters during that normal working session is presented in the chosen
  language rather than falling back to English.
- **SC-005**: Zero dialogs in any shipped language contain clipped, overlapping,
  or out-of-bounds controls, verified against a checked list of the most
  frequently used dialogs of the application and of each enabled plugin.
- **SC-006**: Every accelerator key and keyboard shortcut that works in English
  also works in every shipped language, with no duplicate accelerators within a
  single menu or dialog.
- **SC-007**: Zero user-visible strings in any shipped language contain the
  predecessor product name, predecessor vendor name, or a predecessor vendor web
  address.
- **SC-008**: Ukrainian reaches the same functional completeness as the other
  shipped languages — same modules covered, same session completed, same zero
  prompts — despite having no human translator.
- **SC-009**: A maintainer produces the complete 12-language product with one
  build command, and two clean builds of unchanged sources produce equivalent
  results.
- **SC-010**: The build reports translation coverage per language, and a
  maintainer can tell from that report alone which languages and which modules
  need translator attention.
- **SC-011**: Adding a new English string and rebuilding requires no manual work
  in any language, and the new string appears in every language module.

## Clarifications

### Q1: How should text without a human translation be supplied? — **Resolved**

**Decision**: Machine-translate the gaps. Every missing dialog, menu and string is
translated automatically into each target language, stored as translation source
in the repository, and marked as machine-produced so translators can find and
revise it. This covers the four untranslated plugins in full and every string
added to older modules since the historical translations were made. Accelerators,
placeholders and formatting must survive translation, and dialog layouts must be
adjusted where translated text no longer fits.

*Recorded in FR-008, FR-010 through FR-013.*

### Q2: How should legacy product identity inside translated text be handled? — **Resolved**

**Decision**: Rewrite everywhere and drop the old links. Predecessor product and
vendor names are replaced with the Newt Commander identity throughout shipped
text and language-chooser metadata; predecessor forum and support URLs are
removed outright rather than repointed, and translator contact information is
replaced with the Newt Commander project address. Original translator names stay
credited. Substitutions must read grammatically in languages that decline nouns.

*Recorded in FR-018 through FR-021.*

### Q3: Which languages should be shipped in the installable product? — **Resolved**

**Decision**: Ship all of them, and add one. All 10 existing languages ship
regardless of coverage, plus English, plus **Ukrainian as a new language produced
entirely by automatic translation from the English original** — 12 languages
total. Coverage is reported to maintainers but never gates what users receive.

*Recorded in FR-001, FR-002, FR-009, FR-027 and User Story 4.*

## Assumptions

- The product's existing language-selection machinery — first-run chooser,
  configuration page, saved choice, matching against the Windows display
  language, per-plugin fallback, and the "translation is incomplete" notice — is
  correct and is reused as-is. This feature supplies content for it, and does not
  redesign it.
- Changing the active language may continue to require an application restart, as
  it does today. Live language switching is out of scope.
- The 10 language folders present in the repository constitute the complete set of
  existing human translations. No new human translations are commissioned by this
  feature; everything beyond them is produced automatically.
- The historical translation data was produced against an older release of the
  predecessor product and against older plugin versions, so a substantial share of
  each language will be machine-produced rather than human-authored. This is an
  expected, reported outcome rather than a defect.
- English remains the source language, the input to automatic translation, and the
  last-resort fallback.
- Machine-produced translations are accepted as shippable quality for this
  feature; human review is a later, ongoing activity that the marking required by
  FR-011 exists to support.
- Only the 19 plugins enabled in the current build policy are in scope. The 7
  disabled plugins that have translation data are skipped, and re-enabling one
  later is expected to pick its translations up automatically.
- Help files (the HTML user manual) are not part of this feature; only the
  application and plugin user interfaces are translated.
- Translator-facing tooling for producing *new* human translations (a translator
  workflow, exports for translators, a public translation portal) is out of scope.
- Character encoding and font support for the shipped scripts, including Cyrillic
  and Simplified Chinese, is provided by the operating system; no font bundling is
  required.
