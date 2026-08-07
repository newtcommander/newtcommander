# Feature Specification: Contextual Re-translation of Machine-Translated UI Strings

**Feature Branch**: `055-contextual-retranslation`
**Created**: 2026-08-07
**Status**: Draft
**Input**: User description: "Nyní provedeme revizi a případnou úpravu automaticky přeložených textů stejně jako jsme to udělali v SFTP pluginu - tedy ty texty, které byly přeloženy automaticky pomocí DeepL budou znovu přeloženy, tj. revidovány pomocí překladu s kontextem - stejně jako jsme to dělali v případě SFTP pluginu. Tedy do překladu jde nejen slovo, resp. výraz pro přeložení, ale i kontext, kde se výraz použitý tak, aby DeepL lépe tedy přesněji výraz přeložililo. Týká se to pouze automaticky přeložených slov/výrazů, resp. těch které byly pomocí automatického překladu doplněny. Plugin SFTP je již hotový."

## Background

When the translation pipeline was first run across the whole product
(feature 038), gaps that no human translation covered were filled by machine
translation of the **bare string alone**. Isolated UI words translate badly
without knowing where they appear: "Host:" became the Czech word for a
talk-show presenter, "Key file" became the adjective "key", "&New" became
"a news item". Feature 051 fixed the method — every string is now sent
together with a description of *where* it is used (which dialog or menu, what
kind of control, its neighbouring labels, and what the module does) — but that
context-aware method was applied only to the SFTP plugin.

Every other module still ships the context-less machine translations from
feature 038. Each translated entry's provenance (human, machine, or English
fallback) is recorded in per-module sidecar files, so the affected entries are
precisely identifiable. This feature re-translates exactly those entries with
the context-aware method, across all shipped modules and languages, leaving
human translations untouched.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Non-English UI reads correctly in every module (Priority: P1)

A user running Tandem Commander in Czech, German, French, Dutch, Hungarian,
Romanian, Slovak, or Spanish opens dialogs, menus, and messages anywhere in the
product — the file manager itself and any shipped plugin — and the labels read
as a native speaker would write them for that place in the UI. Words that were
previously translated as an unrelated sense of the same English word (the
"talk-show host" class of error) now carry the meaning their location demands.

**Why this priority**: This is the entire point of the feature — the shipped
product currently contains hundreds of location-inappropriate translations per
language, and they are the most visible quality defect a non-English user
meets. Everything else in this spec exists to deliver this safely.

**Independent Test**: Switch the UI to Czech, open a dialog of any non-SFTP
plugin that previously showed a known-bad machine translation, and confirm the
label now matches its UI role. Can be spot-checked per language without any
knowledge of the pipeline.

**Acceptance Scenarios**:

1. **Given** a translation entry that was machine-translated without context in
   feature 038, **When** the re-translation is run and the product is rebuilt,
   **Then** that entry's text is the result of context-aware translation (or a
   deliberate pinned override), not the old context-less output.
2. **Given** the re-translated product in an enabled language, **When** a
   reviewer spot-checks previously mistranslated one-word labels, buttons, and
   column headings, **Then** the wording matches the string's UI role and
   module domain.
3. **Given** the same English word appearing in two different UI locations with
   different meanings, **When** both are re-translated, **Then** each location
   may receive a different translation appropriate to its own context.

---

### User Story 2 - Human translations are never touched (Priority: P1)

A translator (or the project) has invested in human translations inherited
from the predecessor product. After the re-translation run, every entry whose
provenance is "human" is byte-for-byte identical to before, as is every entry
marked as not-to-be-translated ("skip") and every English-fallback entry.
Pinned overrides (e.g. the plugin name "ZIP") continue to take precedence.

**Why this priority**: The operation rewrites committed translation source in
bulk. Damaging human work would be an irreversible quality loss (the machine
cannot reproduce it) and would violate the trust that lets this operation run
at all. It is co-equal P1: US1 without this guarantee is a net loss.

**Independent Test**: Diff the committed translation source before and after
the run; verify with the provenance sidecars that every changed entry had
"machine" provenance and no entry with "human" or "skip" provenance changed.

**Acceptance Scenarios**:

1. **Given** the committed translation files and their provenance sidecars,
   **When** the re-translation runs, **Then** only entries recorded as
   "machine" change text, and the human/skip/fallback population is unchanged.
2. **Given** a pinned override for a specific entry, **When** that entry is
   re-translated, **Then** the pinned text still wins in the committed output.
3. **Given** the SFTP plugin (already contextually translated in feature 051),
   **When** the re-translation runs over the product, **Then** the SFTP
   module's translations are not re-sent and not changed.

---

### User Story 3 - Maintainer runs the operation safely and repeatably (Priority: P2)

The maintainer previews the exact scope before spending anything: how many
entries per language and module qualify, and roughly how many characters the
translation service will bill. They then run the re-translation — for
everything, or restricted to one language or module — within the service's
free-tier quota, and receive a report of what was done: entries re-translated,
entries that fell back to English because the new translation failed
validation, layout adjustments made, and characters spent.

**Why this priority**: The operation spends a metered external quota and
rewrites ~3,300 committed entries; without preview, scoping, and reporting it
is not safely operable. It is P2 only because it serves the P1 outcomes.

**Independent Test**: Run the preview mode and confirm it reports scope and
cost without network access or file writes; run a single-module scope and
confirm only that module's files change.

**Acceptance Scenarios**:

1. **Given** the committed translation state, **When** the maintainer runs a
   preview, **Then** the qualifying entry counts and character estimate are
   reported and no files are written and no network is used.
2. **Given** a run restricted to one language or one module, **When** it
   completes, **Then** only that scope's files changed.
3. **Given** a character budget cap, **When** the cumulative spend would
   exceed it, **Then** the run stops at a language boundary, leaving every
   already-processed language complete and consistent.
4. **Given** a completed run, **When** the maintainer reads the report,
   **Then** it states per language: entries re-translated, new English
   fallbacks, layout adjustments, and characters spent and remaining.

---

### Edge Cases

- **Re-translation fails validation** (placeholder, accelerator, or shortcut
  label not preserved): the entry keeps English, its provenance records the
  fallback, and the report lists it — same policy as the original pipeline.
- **New translation is longer than the control**: the control is widened to
  fit, as the pipeline already does; the report counts widenings.
- **New translations collide on a keyboard accelerator** within one dialog or
  menu: resolved automatically where possible by moving the marker; residual
  duplicates are reported for human rewording.
- **Context-aware result equals the old context-less result**: the entry is
  simply confirmed; harmless, expected for unambiguous strings.
- **Identical (context, text) pair appears in several modules**: translated
  once and reused, so the quota is not spent twice.
- **A module has no machine-provenance entries** for some language: its files
  for that language are not meaningfully changed by the run.
- **Disabled languages and disabled modules**: untouched by an unrestricted
  run, per the established language/plugin policy; explicitly naming a
  disabled language remains the documented opt-in and works for this
  operation too.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST identify, per (language, module) pair, exactly
  the translation entries whose recorded provenance is "machine", and select
  only those for re-translation.
- **FR-002**: Every re-translated string MUST be sent together with its usage
  context — the module's domain, the containing dialog/menu/string-table, the
  role of the control, and its neighbouring labels — using the same
  context-aware method established for the SFTP plugin in feature 051.
- **FR-003**: Entries with provenance "human" or "skip" MUST remain
  byte-for-byte unchanged, and existing English-fallback entries MUST NOT be
  re-sent for translation.
- **FR-004**: Pinned per-entry overrides MUST continue to take precedence over
  any re-translation result.
- **FR-005**: The default scope MUST be all enabled languages × all enabled
  modules **excluding the SFTP plugin** (already contextually translated);
  disabled languages and modules are excluded from unrestricted runs, with the
  existing explicit-naming opt-in preserved.
- **FR-006**: All existing output validation MUST apply to the new
  translations — placeholders, accelerator markers, and shortcut labels must
  survive translation; an entry failing validation keeps English and its
  provenance records the fallback.
- **FR-007**: Layout consequences of new text MUST be handled as the pipeline
  already does: controls widened to fit overflowing text, duplicate keyboard
  accelerators within a dialog or menu reassigned automatically, and
  unresolvable duplicates reported.
- **FR-008**: Provenance sidecars MUST be updated so that after the run they
  accurately describe every entry's origin, and the committed translation
  files MUST remain structurally valid for the positional import (no rows
  added, removed, or reordered relative to the current template).
- **FR-009**: The maintainer MUST be able to preview the run (qualifying entry
  counts and character estimate) with no network access and no file writes,
  and to restrict a real run to one language or one module.
- **FR-010**: The operation MUST be quota-aware: identical (context, text)
  pairs deduplicated before sending, an optional character budget cap that
  stops cleanly at a language boundary, and a report of characters spent and
  remaining.
- **FR-011**: A completed run MUST report, per language and in total: entries
  re-translated, new English fallbacks, layout adjustments (widenings,
  accelerator reassignments, residual duplicates), and quota usage.
- **FR-012**: After the re-translated source is committed, a full product
  build MUST produce all enabled language modules without translation-import
  errors, and the build itself MUST remain fully offline (the translation
  service is never contacted at build time).
- **FR-013**: The user-visible translation refresh MUST be recorded in the
  project changelog for the release that ships it, per the constitution's
  release-documentation rule.

### Key Entities

- **Translation entry**: one translatable string at a fixed position in a
  module's translation file — identified by module, section (dialog / menu /
  string table), and row; carries the English source text and the translated
  text.
- **Provenance record**: the per-entry origin held in a sidecar next to each
  module's translation file — one of *human*, *machine*, *english_fallback*,
  or *skip*. The selector for this whole feature: "machine" entries are the
  scope, everything else is the do-not-touch set.
- **Usage context**: the English description sent alongside a string —
  module domain, containing section, control role, neighbouring labels. Built
  per section so neighbours disambiguate one-word strings; guidance only,
  never itself translated.
- **Language / module registries**: the committed policy files that say which
  languages and modules ship; they define the default scope of the run.
- **Pinned override**: a committed per-entry text that outranks any
  translation result (e.g. the plugin name "ZIP").
- **Coverage report**: the per-(language, module) outcome summary — totals by
  provenance, fallbacks, layout adjustments, quota spend.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% of machine-provenance entries in the default scope
  (≈3,257 entries across the 8 enabled languages, all enabled modules except
  SFTP) are re-translated with context; the completion report and provenance
  sidecars agree that none were missed.
- **SC-002**: Zero entries with "human" or "skip" provenance differ from their
  pre-run text, verified by diffing the committed translation source against
  the provenance sidecars.
- **SC-003**: In a spot-check of at least 20 re-translated entries per
  maintainer-readable language (Czech and Slovak) drawn from the historically
  worst categories — one-word labels, buttons, column headings — at least 90%
  read correctly for their UI location, and none of the documented
  "wrong-sense" errors (talk-show-host class) remain in checked entries.
- **SC-004**: A full product build after the commit produces all 8 enabled
  language modules with zero translation-import errors, and the application
  starts and displays each language without missing-resource failures.
- **SC-005**: New English fallbacks introduced by the run remain under 2% of
  re-translated entries per language, and every one is listed in the report.
- **SC-006**: The whole default-scope run completes within a single free-tier
  monthly quota of the translation service, with spend visible in the report.

## Assumptions

- The context-aware translation method, validation rules, layout handling, and
  provenance tracking established in features 038/051 are the accepted
  mechanism; this feature is a scoped re-run of translation content, not a
  redesign of the pipeline.
- The SFTP plugin is complete (feature 051) and is excluded from the default
  scope; re-processing it would spend quota to reproduce existing output.
- Only "machine"-provenance entries are in scope. Existing English-fallback
  entries stay English: they were never filled with a machine translation, and
  the user scoped this feature to texts that were. Rescuing fallbacks with
  context-aware retries is a possible follow-up, not part of this feature.
- Disabled languages (Russian, Ukrainian, Simplified Chinese) are out of the
  default scope per the language build policy — they do not ship, and quota
  spent on them is waste. Their committed source is untouched; when one is
  re-enabled, the same operation can be run for it by naming it explicitly.
- The translation service and its free-tier character quota remain available
  to the maintainer; the operation is developer-run and offline from the
  build's perspective, so build reproducibility is unaffected.
- Full linguistic review of all 8 languages is out of scope; quality is
  verified by the provenance-scoped diff, validation gates, and spot-checks in
  maintainer-readable languages.
