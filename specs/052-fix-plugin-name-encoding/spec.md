# Feature Specification: Fix Plugin Name Encoding in Plugin Manager

**Feature Branch**: `052-fix-plugin-name-encoding`
**Created**: 2026-08-06
**Status**: Draft
**Input**: User description: "Kdyz otevru nastaveni spravce pluginu a mam prepnuto na cestinu, dostanu OPET spatne zobrazeni kodovani, viz soubor ./temp/plugins_strings.png se screenshotem. Toto jsme jiz opravovali. Alokuj nekolik nezavislych agentu a identifikuj pricinu, pripadne navrhny pro opravy tak, aby se jiz znovu neopakovalo a oprav to."

## Problem Statement

With the UI language set to Czech, the Plugins Manager window shows plugin
names as garbled text (e.g. "HromadnĂ© pĹ™ejmenovĂˇnĂ­" instead of
"Hromadné přejmenování"). The corruption pattern is UTF-8 text rendered as
if it were Central-European (CP1250) single-byte text.

Observed characteristics (from the user's screenshot `temp/plugins_strings.png`
and follow-up report):

- Plugin names of **not-loaded** plugins are garbled.
- Plugin names of **loaded** plugins display **correctly**.
- Strings from the main application language module (column headers such as
  "Název", "Načteno", "Umístění") display correctly.

The same class of defect was already fixed once in the past; it has returned.
This feature therefore has three goals: (1) restore correct display,
(2) identify and eliminate the root cause rather than the symptom, and
(3) add a durable guard so this class of corruption cannot silently ship again.

The root-cause investigation (see [investigation.md](investigation.md)) also
surfaced one further defect visible in the same dialog: the ZIP plugin's name
was machine-mistranslated as a postal-code term in four languages ("PSČ",
"Code postal", "Código postal"); correcting and future-proofing it is included
as a secondary goal.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Correct plugin names in every UI language (Priority: P1)

A user running Tandem Commander with the Czech (or any other shipped)
UI language opens Plugins Manager. Every listed plugin shows its name and
description with correct diacritics, regardless of whether the plugin is
currently loaded or not.

**Why this priority**: This is the visible defect. Garbled text in a core
management dialog makes the product look broken and untrustworthy.

**Independent Test**: Switch the UI to Czech, restart the application (so
plugin names come from the persisted cache, not from freshly loaded plugins),
open Plugins Manager, and visually verify all plugin names.

**Acceptance Scenarios**:

1. **Given** the UI language is Czech and no plugin has been loaded in this
   session, **When** the user opens Plugins Manager, **Then** every plugin
   name and description displays with correct Czech diacritics.
2. **Given** the UI language is Czech, **When** a plugin is loaded during the
   session and Plugins Manager is opened, **Then** its name displays
   identically to the not-loaded case (no difference between the two states).
3. **Given** any other shipped UI language with non-ASCII characters,
   **When** the user opens Plugins Manager, **Then** plugin names display
   correctly for that language as well.

---

### User Story 2 - Existing installations are correct immediately after update (Priority: P2)

A user whose installation currently shows garbled plugin names updates to
the fixed build. On the next start, Plugins Manager shows correct names —
with the user's existing stored configuration untouched (investigation
confirmed the persisted values are intact; the defect is display-only).

**Why this priority**: The fix must reach every affected installation with
zero user action, and must not rewrite or migrate user configuration to do
so — the stored data was never the problem.

**Independent Test**: Take a machine (or registry state) that reproduces the
garbled display today, install the fixed build without touching stored
configuration, and verify names display correctly on first start.

**Acceptance Scenarios**:

1. **Given** an existing per-user configuration from an affected build,
   **When** the user starts the fixed build, **Then** Plugins Manager shows
   correct names without any modification, repair, or migration of the
   stored values.
2. **Given** a fresh installation with no prior configuration, **When** the
   user starts the application and opens Plugins Manager, **Then** all names
   are correct from the first run.

---

### User Story 3 - The defect class cannot silently return (Priority: P3)

A developer changes translation data, translation tooling, or the plugin
metadata persistence code. If the change would reintroduce text corruption of
plugin names (or other translated strings on the same pathway), an automated
check fails before the change ships, pointing at the corrupted value.

**Why this priority**: This is the second time the same symptom appeared.
Without an automated guard, nothing prevents a third occurrence; manual
visual inspection of every language after every change is not sustainable.

**Independent Test**: Deliberately seed a corrupted (double-encoded) plugin
name into the pipeline or persisted state in a test environment and verify
the guard detects and reports it; verify the guard passes on healthy data.

**Acceptance Scenarios**:

1. **Given** healthy translation data and code, **When** the automated guard
   runs (as part of build or test), **Then** it passes.
2. **Given** a deliberately double-encoded plugin name introduced anywhere on
   the guarded pathway, **When** the guard runs, **Then** it fails and
   identifies the offending string/value.

---

### User Story 4 - Plugin identifier names are not mistranslated (Priority: P4)

A user browsing Plugins Manager in any shipped language sees plugin names
that mean the same thing as the original. Product identifiers such as "ZIP"
appear as recognizable identifiers, not as unrelated dictionary words
(today Czech/Slovak show "PSČ" — postal code — and French/Spanish show
"Code postal"/"Código postal" for the ZIP plugin).

**Why this priority**: Discovered during the same investigation and visible
in the same dialog; a wrong name misleads users about what a plugin does,
but it affects one plugin in four languages, so it ranks below the
encoding defect.

**Independent Test**: Switch to each affected language and verify the ZIP
plugin's displayed name is a recognizable ZIP identifier; verify machine
re-translation of plugin display names cannot silently reintroduce this.

**Acceptance Scenarios**:

1. **Given** any shipped UI language, **When** the user opens Plugins
   Manager, **Then** the ZIP plugin's name reads as a ZIP identifier, never
   as a postal-code term.
2. **Given** a future machine-translation refresh of translation data,
   **When** plugin display names are processed, **Then** identifier-type
   names are protected from being replaced by unrelated meanings.

---

### Edge Cases

- User switches UI language (e.g. English → Czech) after plugin names were
  cached under the previous language: the displayed names must be correct
  text in some shipped language — never corrupted bytes.
- A plugin that has never been loaded since installation (name known only
  from the persisted cache).
- Stored plugin names were verified intact — the fix must not rewrite,
  migrate, or "repair" them; display must be correct with existing data
  as-is.
- Names containing only ASCII characters (e.g. "Mapa disku") — correct in
  both states today, and must remain unaffected by the fix.
- A plugin loaded mid-session — its row must display identically before and
  after the load (today loading a plugin visibly "heals" its row, which is
  itself a symptom of the inconsistency).
- Currently disabled non-Latin-script languages (Russian, Ukrainian,
  Simplified Chinese) — the fix must not assume Central-European characters
  only, so re-enabling those languages later does not reintroduce the bug.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Plugins Manager MUST display plugin names and descriptions with
  correct characters (including diacritics) in every shipped UI language, for
  both loaded and not-loaded plugins.
- **FR-002**: Plugin metadata that is persisted between sessions MUST
  round-trip exactly: the text written must be byte-for-byte recoverable and
  render identically to the text obtained from a freshly loaded plugin.
- **FR-003**: The root cause of the corruption MUST be identified and removed
  at its source; the fix MUST NOT merely re-convert or patch up corrupted
  text at display time.
- **FR-004**: The fix MUST work with existing persisted configuration as-is
  (stored values were verified intact); no data migration, repair, or user
  action may be required, and stored values MUST NOT be rewritten by the fix.
- **FR-005**: An automated, repeatable check MUST exist that detects this
  class of defect (translated text rendered through a mismatched-encoding
  pathway) and fails visibly (build or test failure) when it occurs. The
  check MUST describe the defect class, not just currently known instances,
  and MUST fail loudly when it cannot run at all.
- **FR-006**: The investigation MUST document why the previous fix did not
  prevent recurrence (what the earlier fix covered, what it missed), so the
  new guard demonstrably closes that gap. *(Satisfied by
  [investigation.md](investigation.md).)*
- **FR-007**: The ZIP plugin's display name MUST be the identical identifier
  "ZIP" in every shipped language (user decision 2026-08-06 — this also
  replaces German "ZIP-Archiv"); no language may show an unrelated
  dictionary translation such as "PSČ"/"Code postal"/"Código postal".
- **FR-008**: The translation tooling MUST protect identifier-type plugin
  names from machine mistranslation in future translation refreshes.

### Key Entities

- **Plugin metadata record**: the per-plugin information shown in Plugins
  Manager (name, description, version, location, loaded state); exists both
  as live data from a loaded plugin's language module and as a persisted
  per-user cached copy used when the plugin is not loaded.
- **Translated string**: a piece of UI text originating in per-language
  translation source data, delivered through built language modules, and in
  some cases persisted in per-user configuration.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: With the Czech UI active, 100% of shipped plugins show
  correctly spelled names and descriptions in Plugins Manager — both before
  and after any plugin is loaded, and both before and after an application
  restart.
- **SC-002**: An installation that displayed garbled names shows 100%
  correct names after updating to the fixed build, with zero manual steps
  and its stored configuration left byte-identical.
- **SC-003**: A deliberately introduced instance of this defect class (a
  non-ASCII translated name pushed through the guarded pathway) is caught by
  the automated guard in 100% of attempts, before the change reaches a
  release build.
- **SC-004**: A written root-cause record exists that names the failing
  stage, explains the earlier fix's gap, and maps each preventive measure to
  the failure it blocks.
- **SC-005**: The ZIP plugin's displayed name is exactly "ZIP" in 100% of
  shipped languages.

## Assumptions

- Root cause is confirmed, not hypothesized: three independent investigation
  traces (runtime path, build/translation pipeline, regression history) agree
  that translation sources, built language modules, and persisted
  configuration are all intact, and that the defect is confined to how the
  application renders the persisted plugin name when the plugin is not
  loaded — the same value flows through two text pathways with mismatched
  encoding assumptions depending on load state. Full evidence with code
  references: [investigation.md](investigation.md).
- The durable fix is understood to require defining a single encoding
  contract for cached plugin metadata (the work features 042/043 explicitly
  deferred), not merely patching the one visible display site; the concrete
  design belongs to the planning phase.
- Czech is the reproduction language, but the defect and fix are treated as
  language-independent: the same guarantee applies to all shipped languages,
  including non-Latin-script languages that may be re-enabled later.
- Correct behaviour for a language switch mid-lifecycle is that cached names
  may lag until refreshed, but must never display as corrupted bytes; full
  language-switch name refresh semantics stay as they are today unless the
  root cause requires changing them.
- Migrating or reading configuration of other products (Open Salamander,
  Newt Commander) remains out of scope per the project constitution.
- The screenshot `temp/plugins_strings.png` is representative: version and
  location columns are unaffected; only translated free-text fields corrupt.
