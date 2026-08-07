# Data Model: Contextual Re-translation of Machine-Translated UI Strings

**Feature**: 055-contextual-retranslation · **Date**: 2026-08-07

No new data structures are introduced; the feature operates on the formats
established by features 038/039/051. This document fixes their shapes and the
state transitions this feature performs.

## Translation entry

One translatable string at a fixed position in a module's `.slt` file.

| Field | Source | Notes |
|---|---|---|
| identity key | `(kind, section_number, entry_id)` | `kind` ∈ DIALOG/MENU/STRINGTABLE; dialog caption uses `entry_id = "#caption"`. Computed by `match.entry_key`; unique within a module. |
| English text | template `.slt` row | Exported from current `english.slg`; defines structure. |
| translated text | committed `.slt` row | The value this feature refreshes for machine entries. |
| state flag | last number of the `.slt` row | `1` = translated, `0` = untranslated (English fallback). |
| geometry | numbers of DIALOG control rows | Always template-derived, then widened from translated text (`translate.layout`). Never carried from the previous run. |

**Invariant (positional import)**: the output `.slt` reproduces the template's
sections and rows exactly — same count, same order. Only quoted text, the
state flag, and control geometry may differ.

## Provenance record (`.origin` sidecar)

JSON object next to each `.slt`: `{"KIND:section:entry_id": provenance}`.

| Provenance | Meaning | This feature's action |
|---|---|---|
| `human` | carried from a human translation (or pinned override) | **must not change** — text byte-identical before/after |
| `machine` | produced by machine translation | **the scope** — demoted to gap, re-sent with context |
| `english_fallback` | translation missing/rejected; English kept | none exist in scope today; a failed re-translation may newly produce this |
| `skip` | not translatable (empty, separator, placeholder, keep-English word) | untouched |

### State transitions performed by the run

```text
machine ──(context re-translation valid)──▶ machine   [text replaced]
machine ──(re-translation fails check())──▶ english_fallback  [text = English, reported]
machine ──(pinned override matches)──────▶ human      [text = override, as today]
human   ──────────────────────────────────▶ human      [byte-identical]
skip    ──────────────────────────────────▶ skip       [byte-identical]
```

## Usage context

Per-section English sentence built by `uicontext.build_contexts` — never
translated itself, sent as DeepL's `context` field (one request per section).

Composition: module **domain** clause (`_DOMAINS`) + section location (dialog
title / menu / string table) + per-row `"text" = role for <humanized symbol>`
items (≤40). This feature extends:

- `_DOMAINS`: from 7 entries to all 20 enabled modules (13 new one-clause
  descriptions per research.md R3).
- `_WORDS`: symbol-splitting vocabulary broadened beyond SFTP terms.

## Scope registries

| Registry | File | Role here |
|---|---|---|
| languages | `translations/languages.cfg` | default run = 8 `enabled = on` languages; disabled (russian, ukrainian, chinesesimplified) untouched |
| modules | `plugins.cfg` (+ salamand) | default run = 20 enabled modules **minus `sftp`** via new `--exclude-module` |
| overrides | `translations/ui-overrides.json` | unchanged; still wins over engine output |

## Coverage report (existing `Coverage` dataclass)

Per (language, module): total / human / machine / fallback / skip counts,
discarded, rebranded, widened, accelerator reassignments, validation failures,
duplicate accelerators; plus DeepL characters sent and quota remaining.
This feature adds no fields — SC-001/SC-005/SC-006 read these numbers.

## Validation rules (unchanged, applied to every new translation)

1. placeholders (`%s`, `%d`, …), escapes (`\n`, `\t`, …) preserved — protected
   via `tag_handling=xml` + verified by `validate.check` after `repair`;
2. accelerator marker count/plausibility preserved; duplicates within a
   dialog/menu auto-reassigned, leftovers reported;
3. shortcut label tails (`\tCtrl+C`) preserved verbatim;
4. failing any check ⇒ English fallback + report entry (never a broken string).
