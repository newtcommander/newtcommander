# Data Model: Plugin Metadata Encoding

**Feature**: 052-fix-plugin-name-encoding · Phase 1 artifact

## Entity: CPluginData translated metadata (`src/plugins.h:2415`)

The per-plugin record shown in Plugins Manager and persisted per user under
`HKCU\Software\Tandem Commander\0.1\Plugins\<n>`.

| Field | Registry value | Translated? | Encoding today (defect) | Encoding after fix |
|---|---|---|---|---|
| `Name` | `Name` | yes (plugin `.slg` `IDS_PLUGINNAME`) | CP1250 when loaded / UTF-8 from registry | **UTF-8 always** |
| `Version` | `Version` | no (ASCII in practice) | mixed as above, invisible | UTF-8 always (normalized for uniformity) |
| `Copyright` | `Copyright` | yes | mixed as above | UTF-8 always |
| `Description` | `Description` | yes | mixed as above | UTF-8 always |
| `Extensions` | `Extensions` | no (ASCII masks) | mixed, invisible | UTF-8 always |
| `RegKeyName` | `RegKeyName` | no (ASCII identifier) | mixed, invisible | UTF-8 always |
| `ChDrvMenuFSItemName` | `FSCmdName` | yes | mixed as above | UTF-8 always |
| `CPluginMenuItem::Name` | `Menu\<n>\Name` (`plugins2.cpp:1696`) | yes | mixed (drawn via tolerant menu path) | UTF-8 always |

**Audit result (T001)**: full REG_SZ list in `CPlugins::Save`/`Load` =
Name, DLLName, Version, Copyright, Extensions, Description, RegKeyName,
FSNames, FSCmdName, LastSLGName, HomePage, ThumbMasks, Menu\<n>\Name.
Out of contract scope (ASCII by nature — paths, masks, identifiers, URLs):
`DLLName`, `RegKeyName`, `FSNames`, `LastSLGName`, `PluginHomePageURL`,
`ThumbnailMasks`. `BugReportMessage`/`BugReportEMail` are NOT persisted
(set only by a loaded plugin) — out of scope.
Producers to normalize: `SetBasicPluginData` (name/version/copyright/
description/extensions), `SetChangeDriveMenuItem` intake
(`plugins1.cpp:1243`), `CPluginMenuItem` constructor (`plugins1.cpp:1907`,
covers every menu-item intake). Registry-load producers (`plugins2.cpp:1387`,
`:1432`, menu load) already receive UTF-8 from the facade.

**Invariant (the contract)**: every `char*` field above holds **valid UTF-8**
from the moment it is stored into `CPluginData`, regardless of producer.
Consumers may rely on it; tolerant (`Sal*U8`) sinks remain as defense in depth.

## Producers and their normalization

| Producer | Input encoding | Action |
|---|---|---|
| `CSalamanderPluginEntry::SetBasicPluginData` (`src/plugins1.cpp:1537`) | plugin-supplied, CP_ACP in practice (`LoadStringA`) | normalize: keep if valid UTF-8, else CP_ACP → UTF-8 (new salunicode helper) |
| `CPlugins::Load` ← registry facade (`src/plugins2.cpp:1310/1331`) | UTF-8 (facade contract) | none needed |
| ChDrv/menu-item setters (if strings persist — audit) | CP_ACP in practice | same normalization |

## State transitions

```
plugin registration/load:
  .slg UTF-16 → LoadStringA → CP1250 → SetBasicPluginData ⟦normalize⟧ → UTF-8 in CPluginData

configuration save (exit / explicit):
  UTF-8 in CPluginData → SalRegSetValueExW8 (UTF-8 probe succeeds) → UTF-16 REG_SZ at rest

next start (plugin not loaded):
  UTF-16 REG_SZ → SalRegQueryValueExW8 → UTF-8 in CPluginData   (already the case today)

display:
  UTF-8 → SalListViewSetItemTextU8 / Sal*U8 sinks / LoadStrU8 compositions → correct glyphs
```

Legacy data at rest (written by any prior build) is already correct UTF-16
(verified byte-level, investigation.md §4) — it enters the new model with no
migration.

## Validation rules

- Normalization helper: ASCII in → byte-identical out; valid UTF-8 in →
  byte-identical out; invalid UTF-8 in → CP_ACP→UTF-8 conversion, never
  failure (falls back to lossy `?` substitution only if the ACP conversion
  itself fails, which cannot happen for CP_ACP bytes).
- Field length caps unchanged (`MAX_PATH - 1` per field, `plugins.h`
  comments); UTF-8 expansion of a CP1250 string can grow byte length — the
  helper must clamp safely to the documented cap (truncation at a UTF-8
  sequence boundary).
- Registry round-trip property: for any UTF-8 string ≤ cap,
  `read(write(s)) == s` byte-for-byte (saltests D4.2a).

## Entity: ZIP plugin display name (translation data)

| Language folder | `zip.slt:311` today | after |
|---|---|---|
| czech, slovak | `"PSČ"` | `"ZIP"` |
| french | `"Code postal"` | `"ZIP"` |
| spanish | `"Código postal"` | `"ZIP"` |
| german | `"ZIP-Archiv"` | `"ZIP"` |
| chinesesimplified | `"邮编"` | `"ZIP"` |
| dutch, hungarian, romanian, russian, ukrainian | `"ZIP"` | unchanged |

Pin: `translations/ui-overrides.json` → `"zip": { "<lang>": { "IDS_PLUGINNAME": "ZIP" } }`
for all 10 non-English folders (overrides win over machine translation).
