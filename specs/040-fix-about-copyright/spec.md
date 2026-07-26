# Feature Specification: Fix About Dialog Copyright Notice

**Feature Branch**: `040-fix-about-copyright`
**Created**: 2026-07-26
**Status**: Draft
**Input**: User description: "Oprav chybu zobrazeni Copyright poznámky v About app dialogu. Např. v češtině se zobrazuje Copyrigth 1997-2026 New Commander Authors a pod tim Autorská práva 2026 Newt Commander Autoři. Takže vypadl Open Salamander. Prostě v About App musí napevno bez překladu zůstat pod sebou, na prvním místě: 2026 Newt Commander Authors a pod tím 1997-2026 Open Salamander Authors, tedy obě zůstanou vždy v angličtině, ale budou prohozeny, ne prvním místě bude Newt Commander pod ním Open Salamander. Ted bez lokalizace."

## Problem Statement

The About dialog shows two copyright lines. Both are currently taken from the
active language module, so every translation is free to alter them — and every
shipped translation has. The predecessor's attribution is destroyed and the
year is wrong.

Observed today (Czech, the user's example):

| Line | Displayed | Should be |
|------|-----------|-----------|
| 1 | `Copyright © 1997-2023 Newt Commander Authors` | Open Salamander attribution, year 2026 |
| 2 | `Autorská práva © 2026 Autoři Newt Commander` | untranslated English |

Three independent defects are visible in that one screenshot:

1. **Lost attribution** — "Open Salamander Authors" was replaced by "Newt
   Commander Authors" in line 1, so the dialog credits Newt Commander twice and
   the predecessor project not at all.
2. **Wrong year** — line 1 says `1997-2023` instead of `1997-2026`.
3. **Partial translation** — line 2 is translated ("Autorská práva", "Autoři
   Newt Commander") while line 1 is not, so the two lines do not even look like
   a pair.

This is not a Czech-only problem. All 11 translation sources carry a corrupted
line 1, and 7 of them additionally translate line 2:

| Language | Line 1 as shipped | Line 2 as shipped |
|----------|-------------------|-------------------|
| Czech | `Copyright © 1997-2023 Newt Commander Authors` | translated |
| Dutch | `Copyright © 1997-2023 Newt Commander Authors` | English |
| French | `Copyright © 1997-2023 Newt Commander Authors` | English |
| German | `Copyright © 1997-2023 Newt Commander Authors` | English |
| Hungarian | `Copyright © 1997-2023 Newt Commander Authors` | translated |
| Romanian | `Copyright © 1997-2023 Newt Commander Authors` | translated |
| Slovak | `Copyright © 1997-2023 Newt Commander Authors` | translated |
| Spanish | `Copyright © 1997-2023 Newt Commander Authors` | English |
| Chinese (Simplified) * | `版权所有©1997-2023 Newt Commander Authors` | translated |
| Russian * | `© 1997-2023 Newt Commander Authors` | translated |
| Ukrainian * | `Авторські права © 1997–2026 Автори «Newt Commander»` | translated |

`*` currently disabled by the language build policy, but the source is retained
and would ship the same defect the moment it is re-enabled.

A legal attribution notice is not translatable content and must never have been
routed through the translation pipeline in the first place. The splash screen
already gets this right — it paints a fixed, build-time English string. The
About dialog is the outlier.

## Clarifications

### Session 2026-07-26

- Q: The startup splash screen shows the same two notices in the opposite order and already hard-codes them in English. Should it be reordered to match the About dialog? → A: Yes — reorder the splash screen too (Newt Commander line first, Open Salamander second). The executable's file-version `LegalCopyright` value stays as it is.
- Q: Should the displayed lines carry the word `Copyright` or only the `©` symbol? → A: The word on both lines — `Copyright © 2026 Newt Commander Authors` and `Copyright © 1997-2026 Open Salamander Authors`. Both the About dialog and the splash screen use these two literals verbatim, from one shared definition.
- Q: What happens to the two now-dead copyright entries in the 11 translation sources? → A: Empty the caption of both controls in the English resource and in every translation source. The controls remain, so their positional slot and per-language geometry are preserved, and the runtime supplies the text — matching how `IDC_ABOUT_OPENSAL` and `IDC_ABOUT_LOGO` are already handled in the same dialog.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Correct attribution in every language (Priority: P1)

A user running Newt Commander in any shipped user-interface language opens
**Help → About Newt Commander**. They see exactly two copyright lines, one
above the other, both in English, both correct: the Newt Commander notice on
top and the Open Salamander notice directly beneath it.

**Why this priority**: This is the whole defect. Shipping a product whose About
box strips the predecessor project's attribution is both wrong and a licensing
credit problem under GPLv2. Nothing else in this feature matters if this is not
fixed.

**Independent Test**: Launch the application under each shipped language, open
the About dialog, read the two lines. Fully testable on its own; delivers the
entire user-visible value of the feature.

**Acceptance Scenarios**:

1. **Given** the UI language is Czech, **When** the user opens the About
   dialog, **Then** the first copyright line reads
   `Copyright © 2026 Newt Commander Authors` and the second reads
   `Copyright © 1997-2026 Open Salamander Authors`.
2. **Given** the UI language is English, **When** the user opens the About
   dialog, **Then** the two lines are identical to the Czech case, in the same
   order.
3. **Given** any other shipped UI language (German, French, Dutch, Hungarian,
   Romanian, Slovak, Spanish), **When** the user opens the About dialog,
   **Then** the two lines are byte-identical to the English case.
4. **Given** the About dialog is open, **When** the user reads the two lines
   top to bottom, **Then** the Newt Commander notice appears first and the Open
   Salamander notice second.
5. **Given** any shipped UI language, **When** the About dialog is displayed,
   **Then** neither line is clipped, wrapped, truncated with an ellipsis, or
   overlapped by another control.

---

### User Story 2 - Translations can no longer break the notice (Priority: P2)

A translator (human or the machine-translation tooling) regenerates or edits a
translation. Whatever they write for the About dialog's copyright entries, the
running application still shows the correct English notice.

**Why this priority**: Story 1 alone could be satisfied by hand-correcting 11
translation files, but the same corruption would reappear on the next
regeneration — the translation source is regenerated wholesale from the English
template, and machine translation has already proven it will rewrite brand names
and years. Making the notice structurally untranslatable is what stops this
from recurring.

**Independent Test**: Deliberately put wrong text into one language's
translation source for the About copyright entries, build that language, run
under it, and confirm the About dialog still shows the correct English lines.

**Acceptance Scenarios**:

1. **Given** a translation source containing arbitrary text for the About
   dialog copyright entries, **When** that language is built and the About
   dialog is opened, **Then** the displayed notice is the correct English text,
   unaffected by the translation source.
2. **Given** the copyright year rule requires an update (a new year boundary),
   **When** a maintainer updates the build-time definition of that line,
   **Then** the About dialog reflects the change in every language with no
   translation work.

---

### Edge Cases

- **A translated language module resizes or repositions the two lines.** Layout
  geometry for these controls comes from the same per-language data as the text.
  The fixed English text is longer than some translations assumed, so a
  language module carrying narrower geometry could clip it. The notice must
  render in full under every shipped language.
- **A language is re-enabled later.** Chinese (Simplified), Russian and
  Ukrainian are currently disabled by the language build policy but their
  sources are retained. Re-enabling any of them must not reintroduce the
  defect.
- **Dark and light theme.** The About dialog is theme-aware. Both lines must
  remain legible against both backgrounds, as the surrounding text already is.
- **Non-Latin UI language.** The notice stays in Latin script regardless of the
  UI script. It must render without mojibake or missing glyphs under a
  CJK/Cyrillic UI language.
- **The `©` character.** The notice contains a non-ASCII symbol; it must
  display as `©` and not as a substituted or garbled character in any language.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The About dialog MUST display exactly two copyright lines, one
  directly above the other.
- **FR-002**: The first (upper) copyright line MUST read
  `Copyright © 2026 Newt Commander Authors`.
- **FR-003**: The second (lower) copyright line MUST read
  `Copyright © 1997-2026 Open Salamander Authors`.
- **FR-004**: Both lines MUST be displayed in English regardless of the active
  user-interface language, and MUST be byte-identical across all shipped
  languages.
- **FR-005**: The content of both lines MUST NOT be sourced from any language
  module or translation file — no translation, present or future, correct or
  corrupted, may change what the user sees.
- **FR-006**: Both lines MUST derive from a single build-time definition per
  holder, so that a future copyright-year update touches one place per holder
  and is reflected in every place the notice is displayed. Each definition MUST
  be a complete, self-contained line including the `Copyright ©` prefix — the
  current split, where the Newt Commander part omits the word because it was
  written as a continuation of one concatenated string, MUST be restructured so
  neither line depends on the other to read correctly.
- **FR-007**: Both lines MUST render in full — not clipped, wrapped, truncated
  or overlapped — under every shipped language, in both light and dark theme.
- **FR-008**: The About dialog MUST otherwise keep its current appearance:
  version line, product wordmark, artwork, web link, licence lines, theme
  colours and dialog size are unchanged by this feature.
- **FR-009**: The two copyright controls MUST carry an empty caption in the
  English resource and in every translation source, so no translation round —
  human or machine — has any text to translate for them. The controls
  themselves MUST remain, preserving their positional slot in the translation
  archive and their per-language geometry.
- **FR-009a**: After the change, no translation source may contain a non-empty
  About-dialog copyright string. This MUST hold for all 11 sources, including
  the 3 currently disabled by the language build policy.
- **FR-010**: The corrected notice MUST apply to every language that ships
  today and to every language currently disabled by the language build policy,
  should it later be re-enabled.
- **FR-011**: The startup splash screen MUST show its two copyright lines in the
  same order as the About dialog — Newt Commander first, Open Salamander second
  — and MUST use the same two literals from the same definitions as the About
  dialog, so the word `Copyright` appears on both splash lines as well. The
  splash screen's existing behaviour is otherwise unchanged: it stays
  English-only, hard-coded, and independent of the active language.
- **FR-012**: The executable's file-version `LegalCopyright` value MUST remain
  unchanged by this feature: `Copyright © 1997-2026 Open Salamander Authors, ©
  2026 Newt Commander Authors`. It is a metadata field, not a displayed notice.
  Restructuring the display definitions under FR-006 MUST NOT alter it.

### Key Entities

- **Copyright notice (Newt Commander part)**: `Copyright © 2026 Newt Commander
  Authors`. Fixed English, build-time constant, first line.
- **Copyright notice (Open Salamander part)**: `Copyright © 1997-2026 Open
  Salamander Authors`. Fixed English, build-time constant, second line.
- **Shipped language set**: the languages currently enabled by the language
  build policy (English plus 7 translations), and the 3 retained-but-disabled
  translations.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In 100% of shipped user-interface languages, the About dialog's
  two copyright lines are identical to the English original — 0 languages
  showing a translated, reordered, mis-attributed or mis-dated notice.
- **SC-002**: "Open Salamander Authors" is present in the About dialog in 100%
  of shipped languages (today: 0%).
- **SC-003**: The copyright years shown are `2026` on the first line and
  `1997-2026` on the second, in 100% of shipped languages (today the Open
  Salamander range ends in 2023 in 10 of the 11 translations).
- **SC-004**: Corrupting a translation source's About copyright entries and
  rebuilding that language produces no change in what the About dialog displays.
- **SC-004a**: Searching all 11 translation sources for a non-empty About-dialog
  copyright string returns 0 hits (today: 22 — two per language).
- **SC-005**: Both lines are fully readable — no clipping or truncation — in
  every shipped language and in both light and dark theme, verified visually.
- **SC-006**: A maintainer can change a displayed copyright year by editing one
  build-time definition per holder, with no edit to any translation file, and
  both the About dialog and the splash screen pick the change up.
- **SC-007**: The splash screen and the About dialog list the two copyright
  holders in the same order — a user who sees both in one session observes no
  contradiction.

## Assumptions

- **Order is a deliberate branding decision**, not an accident: the current
  product is credited first, the predecessor second. This reverses the order
  the English resource has today.
- **The dialog keeps two separate single-line controls** rather than merging the
  notice into one line; the user explicitly asked for one above the other.
- **Scope is the main application's About dialog plus the startup splash
  screen's line order.** Plugin about boxes, the executable's file-version
  resource, and the licence/web lines in the same dialog are not part of this
  feature.
- **The two lines stay non-selectable static text**, as they are today; no
  hyperlink or copy-to-clipboard behaviour is added.
- **Translation sources are not deleted or renumbered.** The translation import
  is strictly positional, so removing entries would shift every later entry.
  Emptying the captions (FR-009) keeps both slots in place and therefore leaves
  the positional structure untouched.
- **Per-language geometry is retained but may need guarding.** Emptying a
  caption does not empty the `x,y,cx,cy` fields that travel with it, and the
  merge tooling is able to width-fit controls. A language module could therefore
  still carry geometry too narrow for the fixed English text; FR-007 is what
  must hold, and how to guarantee it is a planning decision.
- **Copyright years follow the project rule** (up to 2026 → Open Salamander
  Authors; 2026 onward → Newt Commander Authors) and match the values already
  defined for the splash screen and the version resource.
