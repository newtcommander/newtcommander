# Data Model: Complete Revision of File Name and Path Display Encoding

**Feature**: `010-fix-filename-encoding` | **Date**: 2026-07-17

This feature manipulates no persistent business data; its "model" is
the set of string artifacts flowing to the screen, the audit inventory
that tracks them, and the config-version gate. Entities below map
1:1 to the spec's Key Entities.

## 1. File/Directory Path (existing, unchanged)

| Attribute | Value |
|-----------|-------|
| Representation | `char*` / `char[]`, **UTF-8** (feature 004 contract) |
| Producers | `CFilesWindow::GetPath()` (`src/fileswnd.h:479,555` — `Path[SAL_MAX_PATH_UTF8]`), archive/FS composites (`DirectoryLineSetText`, `src/fileswn1.cpp:1662-1739`), plugin FS paths |
| Validation | Strict UTF-8; invalid sequences → legacy-ANSI fallback rendering (never crash, never garble further) |
| Invariant | Rendered glyphs must equal the panel rendering of the same string (FR-001) |

## 2. Application-stored display string (existing, load path changes)

| Attribute | Value |
|-----------|-------|
| Instances | Packer titles `CPackerConfigData::Title` (`src/zip.h:301`), unpacker titles, archiver titles `CArchiverConfig` (`src/pack.h:437`), custom packer commands/args |
| Origin (defaults) | `LoadStr(IDS_DP_*)` from `.slg` resources — UTF-8, localized (Czech contains diacritics) via `CPackerConfig::AddDefault` (`src/packers.cpp:240-271`) |
| Origin (persisted) | Registry REG_SZ via `SalRegQueryValueExW8` → UTF-8 (`src/salamdr6.cpp:2306`) |
| Persistence | REG_SZ via `SalRegSetValueExW8` — UTF-16 in registry, UTF-8 in memory |
| Round-trip invariant | save → restart → reload yields byte-identical UTF-8 (FR-004) |
| **State transition (new)** | On config load with `ConfigVersion` < UTF-8 baseline: section entries are **discarded and rebuilt from current defaults** (clarification 2026-07-17); on `ConfigVersion` ≥ baseline: loaded verbatim |

## 3. Display surface (audit subject)

One row per UI location that renders a Path or Stored String.

| Field | Type / domain | Notes |
|-------|---------------|-------|
| `id` | short key (e.g. `A2`, `B11`, `P2-pack-combo`) | reuse 005 audit IDs where they exist |
| `area` | chrome / dialog / menu / tooltip / plugin-`<name>` | grouping for walkthrough |
| `location` | `file:line` + function | e.g. `stswnd.cpp:1004 CStatusWindow::Paint` |
| `string source` | Path / Stored String / resource text | resource-only text is out of audit scope |
| `api` | draw or control API used | `ExtTextOutA→W`, `CB_ADDSTRING→SalComboAddStringU8`, … |
| `verdict` | `correct` \| `defective` \| `out-of-scope` | from manual walkthrough (clarification: manual protocol) |
| `resolution` | commit/fix reference or rationale | required when verdict = defective |

Lifecycle: `candidate` (from research sweep) → `verified-correct` or
`defective` → `fixed` (with re-verification). Inventory file:
`specs/010-fix-filename-encoding/surface-inventory.md`.

## 4. CStatusWindow text model (reworked in P1)

| Member | Today | After |
|--------|-------|-------|
| `Text` (`char*`) | UTF-8 bytes, drawn raw via `ExtTextOutA` | kept as UTF-8 source of truth (producers/consumers unchanged) |
| **`TextW` (new, `WCHAR*`)** | — | converted once in `SetText`/`SetSubTexts`; NULL ⇒ invalid UTF-8 ⇒ legacy ANSI route |
| `TextLen` | bytes | **WCHAR count** (`TextLenW`); byte length retained only where UTF-8 consumers need it |
| `AlpDX[]` | per-byte advances (`GetTextExtentExPointA`) | per-WCHAR advances (`GetTextExtentExPointW`) |
| `HotTrackItems[].Offset/Chars`, `EllipsedChars`, `SubTexts` | byte units | **WCHAR units**; `GetHotText`/drag handlers convert back via `SalWToU8` |

Invariant: ellipsis insertion and hot-track hit-testing never split a
UTF-16 code unit pair (surrogates) — mirror `CTruncatedString`
(`src/salamdr4.cpp:157+`) semantics.

## 5. Config version gate (new state transition)

| Attribute | Value |
|-----------|-------|
| Constant | `THIS_CONFIG_VERSION` `src/mainwnd2.cpp:143` — bump 104 → 105 ("UTF-8 packer-config baseline") |
| Loaded state | `Configuration.ConfigVersion` (`src/cfgdlg.h:178-179`) |
| Gate location | packer-section load orchestration `src/mainwnd2.cpp:2880-2977` (Custom Packers, Custom Unpackers, Predefined Archivers, Archive Associations) |
| Transition | `ConfigVersion < baseline` → skip `Load()` of section entries, `DeleteAllPackers()` + `AddDefault()` (rebuild defaults); `≥ baseline` → load normally |
| Precedent | plugin gate `ConfigVersion >= 6` at `mainwnd2.cpp:2876`; fall-through `switch (SalamVersion)` in `AddDefault` (`src/packers.cpp:200+`) |
| Note | Baseline must equal the first version that wrote UTF-8 config (feature 004), so healthy post-004 configs are never reset |

## 6. Audit inventory (deliverable)

Markdown table of Display Surface rows (§3), grouped by area, with a
summary header: totals per verdict, walkthrough date, sample-name set
used, build verified against. Completeness rule (SC-004): every area
of the core app plus each of the 18 enabled plugins appears — plugins
with no own name-rendering UI get a single `verified-correct (via
core panel)` row.
