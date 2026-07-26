# Phase 1 Data Model: Language Build Policy

**Feature**: 039-language-build-policy
**Date**: 2026-07-26

This feature adds one field to one existing entity. The model below states what
that field means, what enforces it, and what derives from it.

---

## Entity: Language policy

Not a new file. The policy **is** `translations/languages.cfg` — the registry
feature 038 introduced — with one field added per record. There is exactly one
place where a language is described, and exactly one place where it is switched
on or off.

| Property | Value |
|---|---|
| Location | `translations/languages.cfg` (repository root-relative) |
| Format | INI-style sections, `[folder]` + `key = value` |
| Encoding | UTF-8 **without** BOM (unchanged; the readers assume it) |
| Comments | `#` to end of line (unchanged) |
| Readers | `src/vcxproj/read_languages.ps1`, `tools/translate/config.py` |

---

## Entity: Language (extended)

An existing record. Fields carried over from feature 038 are listed for context;
only `enabled` is new.

| Field | Type | Required | New | Meaning |
|---|---|---|---|---|
| *(section name)* | folder name | yes | no | Directory under `translations/`; also the `.slg` base name |
| `langid` | integer | yes | no | Windows LANGID written to `VarFileInfo\Translation` |
| `display_name` | text | yes | no | Shown in the language chooser |
| `author` | text | yes | no | Original translator credits, preserved verbatim |
| `web` | text | yes | no | Contact address shown in the chooser |
| `comment` | text | yes | no | Native-language description |
| `helpdir` | text | yes | no | Help subdirectory (`ENGLISH` for every language) |
| `origin` | `human` \| `mixed` \| `machine` | yes | no | Drives `SLGIncomplete` |
| **`enabled`** | **`on` \| `off`** | **yes** | **yes** | **Whether this language is built and shipped** |

### `enabled` semantics

- **`on`** — the language is built for every enabled module and kept in the
  output tree.
- **`off`** — no language module is produced; any existing module for it is
  removed from the output on the next build. Translation source and `.origin`
  sidecars are untouched (FR-004).

`enabled` is **required**, not defaulted. A section without it is a validation
error naming that section. This mirrors `plugins.cfg`, where an unlisted plugin
is an error rather than a silent default (see research.md D1).

### Initial values (FR-006)

| Language | `enabled` | Reason |
|---|---|---|
| czech, german, french, dutch, hungarian, romanian, slovak, spanish | `on` | Latin script, render correctly |
| chinesesimplified, russian, ukrainian | `off` | Non-Latin script, menus render incorrectly (known defect, not fixed here) |

Eight enabled, three disabled, out of eleven registered.

---

## Validation rules

Enforced identically by both readers. V1–V3 already exist and are unchanged;
V4–V5 are new.

| ID | Rule | Failure message names | Requirement |
|---|---|---|---|
| V1 | LANGIDs are unique across all records | both offending sections | (038) |
| V2 | Every registered language has a `translations/<folder>/` directory | the language | FR-007 |
| V3 | Every `translations/<folder>/` directory is registered | the directory | FR-007 |
| V4 | Every record has every required field, `enabled` included | the section and the missing fields | FR-007 |
| V5 | `enabled` is `on` or `off` | the section, the bad value, and the accepted values | FR-007 |

**Validation runs over every record, enabled or not.** A malformed disabled
section is still a malformed section — otherwise disabling a language would be a
way to hide a broken record, and re-enabling it would fail at the worst moment.

**All languages disabled is valid** (FR-008 / SC-006): zero enabled languages is
a legal policy that yields an English-only product. Only zero *registered*
languages is an error, which is the pre-existing `no languages defined` check.

---

## Entity: Language module (derived)

Unchanged in nature; this feature changes only which ones exist.

| Property | Value |
|---|---|
| File | `<language folder>.slg` |
| Location | `<out>\lang\` (app) and `<out>\plugins\<plugin>\lang\` (19 plugins) |
| Produced by | `build_langs.ps1`, from `translations/<lang>/<module>.slt` |
| Exists when | its language is enabled **and** a `.slt` for that (module, language) pair exists |
| Removed when | its language is not enabled — including languages removed from the registry entirely |

`english.slg` is **not** a language module in this sense: MSBuild compiles it
from `.rc` sources, `build_langs` never produces it, and reconciliation always
keeps it.

### Reconciliation rule (FR-003)

For every `lang` directory in the output tree:

> Keep `english.slg` and `<folder>.slg` for each **enabled** language.
> Delete every other `*.slg`.

Stated positively rather than as "delete the disabled ones" so that a language
renamed or dropped from the registry is also cleaned up — matching what the
plugin policy already does for unknown output directories.

---

## Relationships

```text
languages.cfg
  ├── [folder] record ──enabled──> included in the build matrix?
  │                          │
  │                          ├── yes ──> translations/<folder>/<module>.slt
  │                          │              └──> <out>/…/lang/<folder>.slg
  │                          │                      └──> offered by the chooser
  │                          │
  │                          └── no  ──> source retained, no .slg,
  │                                       existing .slg reconciled away
  │
  └── (all records) ────────> validation V1..V5, regardless of enabled
```

The build matrix is `enabled languages × enabled modules`, where enabled modules
come from `plugins.cfg` (unchanged). Disabling a language removes a row;
disabling a plugin removes a column. Neither touches source.

---

## State transitions

| From | To | Trigger | Effect on output | Effect on source |
|---|---|---|---|---|
| enabled | disabled | `enabled = off` + any build | modules removed from every `lang` dir | none |
| disabled | enabled | `enabled = on` + full build | modules produced again from committed `.slt` | none |
| disabled | disabled | any build | modules stay absent | none |
| registered | unregistered | record deleted | modules removed (reconciliation catches unknown `.slg`) | directory would fail V3 |

Re-enabling is byte-for-byte reversible (SC-004) because the inputs never
changed: `english.slg` from the same source revision plus the same committed
`.slt` yield the same `.slg`.
