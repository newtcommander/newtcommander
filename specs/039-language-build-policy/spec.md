# Feature Specification: Language Build Policy

**Feature Branch**: `039-language-build-policy`
**Created**: 2026-07-26
**Status**: Draft
**Input**: User description: "Pro nektere jazyky se nyni zobrazuji znaky v menu spatne, prozatim z buildu vypneme nasladujici jazyky: chinesesimplified, russian, ukrainian. Uprav to tak, abych mohl v konfiguracnim souboru urcovat, ktere jazyky pujdou do buildu, neco jako obdoba plugins.cfg."

## Overview

Feature 038 made the product shippable in 12 languages. It did not provide a way
to ship **fewer** than all of them.

Three of those languages — Simplified Chinese, Russian, Ukrainian — currently
render incorrectly in menus. Until that is diagnosed and fixed, they should not
reach users. Right now the only ways to achieve that are to delete their
translation source (destroying committed work) or to hand-delete build output
after every build (unreliable and easily forgotten).

This feature adds a **build policy for languages**, mirroring what
`plugins.cfg` already does for plugins: a maintainer marks each language on or
off in a configuration file, and the build honours that decision — building and
shipping exactly the enabled set, and removing any output belonging to a
disabled one.

Two things this feature is deliberately **not**:

- It does not fix the character rendering defect. That is a separate problem
  recorded here so it is not lost, but the requested outcome is to stop shipping
  the affected languages, not to repair them.
- It does not remove translation source. A disabled language keeps its committed
  `.slt` files and provenance sidecars, so re-enabling it later is a one-line
  change.
- It does not take over installer packaging. Shipping language files in the
  installable product is an open item of feature 038 and stays there; this
  feature's contribution is that the policy becomes the single place that
  decides which languages exist, so the installer has one source to follow
  rather than a list of its own to keep in sync.

## Clarifications

### Session 2026-07-26

- Q: What does disabling a language mean for the translation tooling? → A: Skip disabled languages by default, with an explicit opt-in to process one anyway.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Stop shipping a broken language (Priority: P1) 🎯 MVP

A maintainer notices that a language renders incorrectly. They mark it off in
the configuration file, rebuild, and that language is gone from the product — it
is no longer offered in the language chooser, and no file belonging to it is
left in the build output.

**Why this priority**: This is the entire point of the request. Simplified
Chinese, Russian, and Ukrainian are reaching users in a visibly broken state
today, and there is currently no supported way to stop that.

**Independent Test**: Mark a language off, run a full build, launch the product,
and confirm the language chooser does not list it and its language modules are
absent from the output tree.

**Acceptance Scenarios**:

1. **Given** a language marked off, **When** a full build runs, **Then** no
   language module is produced for it, for the application or for any plugin.
2. **Given** a build output that already contains a language's modules from an
   earlier build, **When** that language is then marked off and the build re-run,
   **Then** its existing modules are removed from the output rather than left
   behind.
3. **Given** a language marked off, **When** the user opens the language chooser,
   **Then** that language is not listed and cannot be selected.
4. **Given** a user whose saved configuration selects a language that has since
   been disabled, **When** the product starts, **Then** it guides them to choose
   an available language instead of failing.

---

### User Story 2 - Re-enable a language without redoing work (Priority: P2)

Once the rendering defect is fixed, the maintainer flips the same three
languages back on, rebuilds, and they return complete — with the translations
that were produced for them still intact.

**Why this priority**: Determines whether disabling is a reversible policy
decision or a destructive one. Without it, the P1 outcome could be achieved by
deleting files, which would throw away real work — including a Ukrainian
translation that does not exist anywhere else.

**Independent Test**: Disable a language, build, re-enable it, build again, and
confirm its language modules return with the same translated content as before.

**Acceptance Scenarios**:

1. **Given** a language that has been disabled and rebuilt, **When** it is
   re-enabled and the build re-run, **Then** its language modules are produced
   again from the committed translation source.
2. **Given** a language is disabled, **When** the maintainer inspects the
   repository, **Then** its translation source and per-entry provenance records
   are still present and unchanged.

---

### User Story 3 - The policy file is honest about mistakes (Priority: P3)

A maintainer mistypes a language name, or adds a translation directory without
registering it. The build stops with a message naming the problem, rather than
silently shipping the wrong set.

**Why this priority**: A policy file that fails silently is worse than none —
the whole point is to be able to trust what shipped. This mirrors the validation
`plugins.cfg` already performs, so the behaviour is familiar. It is P3 because
the product ships correctly without it as long as nobody makes a mistake.

**Independent Test**: Introduce each error in turn (unknown name, unregistered
directory, missing on/off value) and confirm the build fails with a message that
names the offending entry.

**Acceptance Scenarios**:

1. **Given** the policy file names a language that has no translation directory,
   **When** a build runs, **Then** it fails with a message naming that language.
2. **Given** a translation directory exists that the policy file does not
   mention, **When** a build runs, **Then** it fails with a message naming that
   directory.
3. **Given** an entry has a value that is neither on nor off, **When** a build
   runs, **Then** it fails with a message naming that entry and the accepted
   values.
4. **Given** every language is marked off, **When** a build runs, **Then** it
   succeeds and produces an English-only product.

---

### Edge Cases

- **Stale output**: a language disabled after having been built leaves modules
  in the output tree. If they are not removed, the product still finds and offers
  the language, so disabling would appear to do nothing.
- **The user's saved language disappears**: someone running in Russian upgrades
  to a build where Russian is disabled. The product must not fail; it already has
  a path for a missing language file and should use it.
- **All languages disabled**: must produce a working English-only product, not
  an error.
- **English**: is not a translation and is not subject to the policy — it is
  compiled from source and is always present.
- **Plugin modules**: disabling a language must remove it from every plugin's
  language directory too, not only the application's.
- **A disabled language is still authored**: a maintainer wants to prepare or
  correct a translation before enabling it. Tooling skips disabled languages
  unless explicitly asked for one, so budget is never spent by accident and
  preparation is never blocked.
- **Partial builds**: building a single module or a single language must respect
  the policy the same way a full build does.

## Requirements *(mandatory)*

### Functional Requirements

#### The policy

- **FR-001**: A maintainer MUST be able to mark each shipped language as enabled
  or disabled in a configuration file kept in the repository, without editing any
  build script or source file.
- **FR-002**: The build MUST produce language modules only for enabled languages,
  for the application and for every enabled plugin alike.
- **FR-003**: The build MUST remove from its output any language module
  belonging to a language that is not enabled, so that disabling a language takes
  effect on the next build rather than requiring a clean build.
- **FR-004**: Disabling a language MUST NOT alter or remove its translation
  source or its per-entry provenance records.
- **FR-005**: Re-enabling a language MUST restore it to the state it would have
  been in had it never been disabled, with no re-translation required.
- **FR-006**: Simplified Chinese, Russian and Ukrainian MUST be disabled by
  default as of this feature; the remaining eight languages stay enabled.

#### Validation

- **FR-007**: The build MUST fail, naming the offending entry, when the policy
  file lists a language that has no translation source, when a translation
  directory is not listed in the policy file, or when an entry's value is not one
  of the accepted values.
- **FR-008**: The build MUST succeed and produce an English-only product when
  every language is disabled.
- **FR-009**: The build MUST report which languages it built and which it skipped,
  so the shipped set is visible without inspecting the output tree.

#### Product behaviour

- **FR-010**: The product MUST offer exactly the enabled languages in its
  language chooser, plus English.
- **FR-011**: A user whose saved language is no longer present MUST be guided to
  select an available one rather than being shown a failure.

#### Tooling

- **FR-012**: Translation authoring tools MUST skip disabled languages by
  default, so that neither translation effort nor external translation budget is
  spent on a language that will not ship.
- **FR-013**: Translation authoring tools MUST provide an explicit way to process
  a disabled language on request, so a maintainer can prepare or correct a
  translation before enabling it.

### Key Entities

- **Language policy**: the record of which languages ship. One entry per
  language, each enabled or disabled. Lives alongside the existing language
  registry so a language is described in exactly one place.
- **Language**: as established by feature 038 — display name, language
  identifier, credits, contact, provenance. This feature adds its enabled state.
- **Language module**: the shipped artefact providing one language's text for
  one module. Produced only for enabled languages; removed from output when its
  language is disabled.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A maintainer can change which languages ship by editing one file,
  with no other change to the repository.
- **SC-002**: After a build with Simplified Chinese, Russian, and Ukrainian
  disabled, the language chooser offers exactly 9 entries (8 languages plus
  English), and the output tree contains no module for the three disabled ones.
- **SC-003**: Disabling a language that was previously built and rebuilding
  removes its modules from the output in that same build — no clean build needed.
- **SC-004**: Re-enabling the three languages and rebuilding restores them with
  byte-identical translated content to what they had before being disabled.
- **SC-005**: Every policy-file error — unknown language, unregistered directory,
  invalid value — fails the build with a message naming the specific entry at
  fault.
- **SC-006**: A build with all languages disabled produces a working English-only
  product.
- **SC-007**: The build output states which languages were built and which were
  skipped.
- **SC-008**: Asking the translation tooling to process every language touches
  only the enabled ones; naming a disabled language explicitly processes it.

## Known defect recorded, not fixed here

Simplified Chinese, Russian, and Ukrainian render incorrectly in menus. This
feature only stops shipping them; the cause is not investigated and no fix is
attempted.

What is known: the three affected languages are exactly those written in
non-Latin scripts (Cyrillic and CJK), while the eight Latin-script languages are
unaffected. This makes a character-encoding or font-selection problem far more
likely than a defect in the translations themselves — the same text may well
display correctly in dialogs while failing in menus, which would narrow it
further. Verifying that is the natural first step whenever the defect is picked
up, and it should be confirmed rather than assumed.

Re-enabling the three languages is a one-line change once it is fixed.

## Assumptions

- The switch belongs in the existing language registry
  (`translations/languages.cfg`), not in a second file. The registry already
  validates itself against the translation directories, and a separate list of
  language names would have to be kept in sync with it — introducing exactly the
  class of mistake FR-007 exists to catch. The result is the same
  maintainer-facing outcome the request asked for: one file, one line per
  language, on or off.
- "Disabled" means *not shipped*. Translation source stays committed, and
  disabling is a reversible policy decision rather than a deletion.
- The default and the opt-in of FR-012/FR-013 apply to every authoring tool that
  iterates languages, not only to machine translation, so that a maintainer never
  has to remember which tools honour the policy and which do not.
- English is out of scope: it is compiled from source rather than translated, and
  is always present.
- Installer packaging is out of scope, and nothing in the installer contradicts
  the policy today: `src/setup/` carries no per-language file list at all, so
  there is no second list to fall out of sync. When feature 038 wires language
  files into the installable product, it derives the set from this policy rather
  than restating it.
- The eight remaining languages (Czech, German, French, Dutch, Hungarian,
  Romanian, Slovak, Spanish) are unaffected and stay enabled.
- The rendering defect affects menus as reported; whether it also affects other
  surfaces is unverified and is left to whoever investigates it.
- No change to how the product loads or selects languages at runtime is needed —
  the existing behaviour for a missing language file already covers the
  disabled case.
