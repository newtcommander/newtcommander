# Contract: Plugin Metadata Encoding

**Feature**: 052-fix-plugin-name-encoding · Phase 1 artifact
**Supersedes**: the "plugin metadata encoding is undefined — revisit later"
deferral recorded in `specs/042-fix-find-results-encoding/inventory.md:90` and
`specs/043-fix-ui-text-encoding/inventory.md:62,55-56`.

## 1. The contract

Every translated/free-text `char*` metadata field stored in `CPluginData`
(see data-model.md for the field list) holds **valid UTF-8** at all times.

- **Producers** are responsible for the invariant:
  - `CSalamanderPluginEntry::SetBasicPluginData` normalizes plugin-supplied
    strings (valid-UTF-8 kept; otherwise CP_ACP → UTF-8). Plugins themselves
    are unchanged and keep passing what `LoadStr` (ANSI) gives them — the
    plugin ABI (interface version 105) and the SDK header semantics are NOT
    modified; normalization is a host-side guarantee.
  - The registry facade (`SalRegQueryValueExW8`) already returns UTF-8 by the
    feature-004 contract (`src/salamdr6.cpp:2298-2303`).
- **Consumers** may assume UTF-8 and must use UTF-8-capable sinks
  (`Sal*U8` helpers, `LoadStrU8` templates for composition). Tolerant
  fallbacks inside `Sal*U8` helpers stay as defense in depth, not as an
  excuse for mixed input.
- **Persistence**: values cross the registry facade as UTF-8 and rest as
  UTF-16 `REG_SZ`. The at-rest format is unchanged by this feature; existing
  stored values are already correct and are not migrated or rewritten.

## 2. Guard contract

1. `tools/check_encoding.py` `UTF8_IDENT` includes the plugin metadata
   identifiers; the 15 `allow mixed-composition - plugin-supplied metadata`
   annotations are removed together with the code conversion — an annotation
   citing an undefined metadata encoding is invalid from this feature on.
2. `saltests` pins: registry facade round-trip (ANSI-in and UTF-8-in),
   normalization helper properties, tolerant listview helper behavior.
3. `build.cmd` fails when the checker cannot run (python missing). No silent
   skip, no bypass switch.

## 3. Identifier-name translation contract

Plugin display names that are product identifiers are pinned in
`translations/ui-overrides.json` (`{module: {language: {key: text}}}`), which
wins over machine translation. First pinned entry: `zip` module,
`IDS_PLUGINNAME` = `"ZIP"` for all non-English languages (user decision
2026-08-06). Any future identifier-type plugin name gets the same pin when
introduced.

## 4. Out of scope (recorded, not addressed here)

- Non-plugin `allow mixed-composition` sites (`dialogs6.cpp:645` network
  share name, `mainwnd3.cpp:2827` configuration name) keep their annotations.
- Other registry-cached translated strings outside `CPluginData` (e.g.
  user-editable custom packer titles) follow the same facade and may exhibit
  the same class of defect at ANSI sinks; if the extended checker surfaces
  such sites, they are follow-up work, not silently included here.
