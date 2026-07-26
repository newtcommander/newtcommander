# Phase 1 Data Model: Translations Build Integration

**Feature**: 038-translations-build-integration
**Date**: 2026-07-26

Entities are grouped by lifetime: **committed** (source of truth, in git),
**generated** (build intermediates under `OPENSAL_BUILD_DIR`, gitignored), and
**shipped** (build output the user receives).

---

## Committed entities

### Language

The registry of what the product ships. New file `translations/languages.cfg`,
same `key=value` / `#`-comment style as the existing `plugins.cfg`.

| Field | Type | Rules |
|---|---|---|
| `folder` | string | Directory under `translations/`; lowercase ASCII; the record key |
| `langid` | integer | Windows LANGID; **unique**; written to `VarFileInfo\Translation` |
| `display_name` | string | Shown in the language chooser |
| `author` | string | Original translator credits — **preserved verbatim** (FR-020) |
| `web` | string | Newt Commander project address (legacy forum URLs removed, FR-019) |
| `comment` | string | Native-language description, e.g. `Česká verze` |
| `helpdir` | string | `ENGLISH` for every language (no localized help, out of scope) |
| `origin` | enum | `human` \| `machine` \| `mixed` — drives `SLGIncomplete` (FR-011, FR-016) |

Twelve records. English is implicit (built from `.rc` sources, never from a
`.slt`) and is not listed.

Known LANGIDs, read from the existing `.slt` headers: Czech 1029, German 1031,
Russian 1049, **Ukrainian 1058** (new).

**Validation**: every `folder` must exist under `translations/`; every `langid`
unique; the set of folders must equal the set of `translations/` subdirectories
(mirroring how `build.cmd` validates `plugins.cfg` against `src/plugins/`).

---

### Module

A separately versioned unit owning its own UI text. Not a new file — derived
from existing state:

| Source | Meaning |
|---|---|
| `plugins.cfg` (`name=on`) | which plugins are in scope (19 enabled) |
| `src/lang/` | the main application module (`salamand`) |
| `src/plugins/<name>/lang/lang.rh` | the `Include=` file every `.atp` needs |

| Field | Value |
|---|---|
| `name` | `salamand`, or the plugin directory name |
| `rh_path` | `src/lang/lang.rh` \| `src/plugins/<name>/lang/lang.rh` |
| `slg_dir` | `<OutDir>\lang\` \| `<OutDir>\plugins\<name>\lang\` |
| `owner_binary` | `newtcommander.exe` \| `<name>.spl` — the FILEVERSION to match |

20 modules in a default build. Verified: all 19 enabled plugins have both a
`lang/` directory and a `lang_<name>.vcxproj`.

---

### Translation source (`.slt`)

`translations/<language>/<module>.slt` — the committed, human-editable
translation for one (Language, Module) pair. Grammar in
[contracts/slt-format.md](contracts/slt-format.md).

Structural rule that dominates the design: the file must correspond **positionally**
to the module's current resources, because `CData::ImportTextArchive`
(`trldata.cpp:2301`) walks `DlgData`/`MenuData`/`StrData` in order and aborts the
whole import on the first mismatch. A `.slt` is therefore always
*template-shaped* — it is regenerated whenever English resources change, never
hand-maintained against a stale layout.

| Section | Row shape | Notes |
|---|---|---|
| `[EXPORTINFO]` | `PROJECTNAME` / `TEXTVERSION` / `VERSION` | **Ignored on import** (`trldata.cpp:2404-2412`) |
| `[TRANSLATION]` | `LANGID`, `AUTHOR`, `WEB`, `COMMENT`, `HELPDIR`, `SLGINCOMPLETE` | From the Language record |
| `[DIALOG <id>]` | first row `cx,cy,state,"caption"`, then `ctrlID,x,y,cx,cy,state,"text"` | Geometry is per-control → the merge tool can width-fit |
| `[MENU <id>]` | `itemID,state,"text"` | Only items with non-empty original text appear |
| `[STRINGTABLE <n>]` | `strID,state,"text"` | 16 slots per block; only populated slots appear |
| `[RELAYOUT]` | bare dialog IDs | Optional trailing section; dialogs needing manual re-layout |

`state` is the translation state: `0` = untranslated, `1` = translated
(`PROGRESS_STATE_*`, `trldata.h:6`).

**Per-entry provenance** (FR-011): tracked in a **sidecar** file
`translations/<language>/<module>.origin` rather than inside the `.slt`. The
`.slt` grammar is fixed by the parser and has no comment syntax — adding a
column would break `ImportTextArchive`. The sidecar maps entry key → `human` |
`machine`, and is what lets a translator find exactly the machine-produced text.

---

## Generated entities (build intermediates, gitignored)

### Translator project (`.atp`)

One per (Language, Module), written into the build tree. Format per
`CData::SaveProject` (`dataprj.cpp:820`); contract in
[contracts/translator-cli.md](contracts/translator-cli.md).

```ini
[Files]
Original=<abs path>\english.slg
Translated=<abs path>\<language>.slg
Include=<abs path>\lang.rh
```

`Include=` is **mandatory** — `OpenProject` gates on
`DataRH.Load(FullIncludeFile)` (`wndframe.cpp:344`). `SalMenu`, `IgnoreList`,
`CheckList`, `SalamanderExe`, `Export` are optional and omitted.

Must be named `<module>.atp`: the export filename is derived from the project
basename (`wndframe.cpp:489-493`).

### English template (`.slt`)

Produced by `-quiet-export-slt` against a project whose `Translated=` is a fresh
copy of `english.slg` — so every "translation" equals its English original. This
is the canonical current-structure skeleton the merge step fills. Regenerated
every time English resources change; never committed.

---

## Shipped entities

### Language module (`.slg`)

`<slg_dir>\<language>.slg` — a resource-only DLL. Produced by copying
`english.slg` and patching it in place:

| Resource | Patched by |
|---|---|
| `RT_STRING` / `RT_MENU` / `RT_DIALOG` | `SaveStrings` / `SaveMenus` / `SaveDialogs` |
| `VS_VERSION_INFO` → `VarFileInfo\Translation` | the Language's `langid` |
| `StringFileInfo\040904b0\SLGAuthor` / `SLGWeb` / `SLGComment` | Language record |
| `…\SLGHelpDir` / `SLGIncomplete` | Language record (`SLGIncomplete` set for non-`human` origin) |
| `…\SLGCRCofImpSLT` | CRC32 of the imported `.slt` |

`FILEVERSION` is **never** written — it survives the copy from `english.slg`,
which is what makes `IsSLGFileValid` (`salamdr2.cpp:3005`) accept it by
construction.

**Invariant to verify post-build** (FR-026): each shipped `.slg`'s
`VS_FIXEDFILEINFO.dwFileVersionMS/LS` equals its `owner_binary`'s.

### Coverage report

Emitted per build (FR-015), consumed by a human.

| Field | Meaning |
|---|---|
| `language`, `module` | the pair |
| `total` | translation units in the current English template |
| `human` | units carrying a human-authored translation |
| `machine` | units filled by machine translation |
| `english_fallback` | units left as English (validation failed, FR-012) |
| `discarded` | legacy entries with no counterpart in the current product |
| `layout_errors` | count from `-quiet-validate-layout` (SC-005 gate) |

---

## Relationships

```
languages.cfg ──< Language >──┐
                              ├──< Translation source (.slt) >── Origin sidecar
plugins.cfg   ──< Module   >──┤
src/lang, src/plugins/*/lang  │
                              ├──< Translator project (.atp) >──┐
english.slg (per Module) ─────┤                                 │
                              └──< English template (.slt) >────┤
                                                                ▼
                                                    Language module (.slg)
                                                                │
                                                                ▼
                                                        Coverage report
```

Cardinality: 12 Languages × 20 Modules = 240 language modules; 11 non-English ×
20 = **220 `.slt` files, 220 `.atp` projects, 220 imports** per full build.

---

## State transitions

A translation unit's lifecycle within one (Language, Module):

```
                    ┌─ present in legacy .slt, ID matches template ─→ human
template entry ─────┼─ no legacy match, machine translation valid ──→ machine
                    ├─ machine translation fails validation (R11) ──→ english_fallback
                    └─ legacy entry with no template counterpart ───→ discarded (dropped, counted)
```

`human` always wins over `machine` for the same entry (FR-017). The build never
promotes or demotes a state — it consumes what the merge step committed.
