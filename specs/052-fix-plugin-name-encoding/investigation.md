# Root-Cause Investigation: Plugin Name Mojibake in Plugins Manager

**Feature**: 052-fix-plugin-name-encoding
**Date**: 2026-08-06
**Method**: Three independent investigation agents (runtime trace, build/translation
pipeline audit, regression history), findings cross-verified. All three converged
on the same root cause with byte-level evidence.

## Verdict

`CPluginData::Name` (`src/plugins.h:2419`) has **no defined encoding** and in
practice carries **two different encodings depending on plugin load state**.
The Plugins Manager name column renders it through the one remaining ANSI-only
listview write. Everything upstream (translations, built `.slg` modules,
registry contents) is **intact** — the defect is display-only.

| Plugin state | `Name` producer | Encoding | Rendered by `LVM_SETITEMTEXTA` |
|---|---|---|---|
| Loaded | `SetBasicPluginData` ← plugin `LoadStr` → `LoadStringA` (`src/salamdr2.cpp:53` → `src/plugins1.cpp:1602`) | CP_ACP (CP1250) | correct |
| Not loaded | `CPlugins::Load` → `GetValue` → `SalRegQueryValueExW8` (`src/plugins2.cpp:1331` → `src/salamdr6.cpp:2343,2363`) | UTF-8 | **mojibake** |

The failing sink: `src/plugins2.cpp:1048` in `CPlugins::AddNamesToListView`
(`ListView_SetItemText(hListView, i, 0, plugin->Name)`) — the project builds
without `UNICODE`, so this is `LVM_SETITEMTEXTA` interpreting UTF-8 bytes as
CP1250. Byte-level reproduction is exact:
`"Hromadné přejmenování"` → UTF-8 → read as CP1250 → `"HromadnĂ© pĹ™ejmenovĂˇnĂ­"`,
character-for-character the screenshot text.

## The asymmetry that creates the two encodings

Feature 004 (commit `39dabac`) introduced a UTF-8 registry facade:

- **Write** `SalRegSetValueExW8` (`src/salamdr6.cpp:2379-2409`): UTF-8-first with a
  *transitional CP_ACP fallback* (`MB_ERR_INVALID_CHARS` probe). A CP1250 name
  fails the UTF-8 probe, takes the fallback, and lands in the registry as
  **correct UTF-16**.
- **Read** `SalRegQueryValueExW8` (`src/salamdr6.cpp:2306-2377`): **always** emits
  UTF-8 (`WideCharToMultiByte(CP_UTF8, ...)`), by documented contract
  (`salamdr6.cpp:2298-2303`). There is no ANSI counterpart to the write-side
  tolerance.

So the ANSI name goes in cleanly and comes back out as UTF-8, into a field
that other code paths still treat as ANSI.

## Verified-clean stages (evidence)

1. **Translation sources** — all 27 `translations/czech/*.slt` are UTF-8-BOM,
   strict-decode, no mojibake markers. E.g. `czech/renamer.slt:151`
   `1000,1,"Hromadné přejmenování"` (bytes `C3 A9`=é, `C5 99`=ř correct).
2. **Built language modules** — `build\tandemcommander\Release_x64\plugins\*\lang\czech.slg`
   dumped via `LoadStringW`: correct UTF-16 `RT_STRING` for renamer/checksum/
   undelete/filecomp/mdview/dbviewer; main `lang\czech.slg` headers correct.
3. **Registry** — `HKCU\Software\Tandem Commander\0.1\Plugins\1..19`, raw
   code-unit dump: all 19 names are clean UTF-16 (e.g. key 15 renamer:
   `0048 0072 006F 006D 0061 0064 006E 00E9 …` = "Hromadné přejmenování").
   Regex scan for mojibake markers: zero hits. **No data repair is needed.**
4. **plugins.ver auto-registration** — `plugins.ver` carries only versions and
   paths (no names); registration loads each plugin with the user's configured
   language (`Configuration.LoadedSLGName` tried first, `src/plugins1.cpp:1738`),
   so registered names are correct CP1250 Czech.

## Why the loaded/not-loaded difference and why the rest of the dialog is fine

- `SetBasicPluginData` runs on every `InitDLL`, overwriting `Name` with the
  CP1250 string → loading a plugin "repairs" its row for the session.
- Description/copyright/www/extensions in the same dialog were fixed by
  feature 010 with `Sal*U8` helpers; those helpers use `SalU8ToW` with
  `MB_ERR_INVALID_CHARS` and **fall back to the legacy ANSI call** on invalid
  UTF-8 (`src/common/winlib.cpp:1102-1114`, `src/common/salunicode.cpp:23`) —
  so they render both encodings correctly *by accident of tolerance*.
- The Plugins **menu** draws names via `SalU8ToWAlloc`+`DrawTextW`
  (`src/plugins2.cpp:1083-1108`, `src/menu3.cpp:927-1051`) — also tolerant.
- Only `plugins2.cpp:1048` (and siblings `:1050,:1053,:1055`, ASCII in
  practice) has no wide/tolerant path. A ready-made tolerant helper exists and
  is used at 20+ other sites: `SalListViewSetItemTextU8`
  (`src/common/winlib.cpp:1202-1220`, `src/common/winlib.h:346`).

## Regression history — why "we already fixed this" and it came back

- **Feature 010** (`f950681`, 2026-07-17) fixed 12 mojibake sites in the Plugins
  Manager **detail pane** (`src/dialogs5.cpp`) and marked the page done in
  `specs/010-fix-filename-encoding/surface-inventory.md:79`. The **name column
  writer lives in `src/plugins2.cpp`**, was never inventoried, and no encoding
  feature ever opened that file (`git log` on `src/plugins2.cpp`: only 007/016/
  032/045/046/comment-translation commits).
- **Features 042/043** explicitly **deferred** plugin metadata:
  `specs/042-fix-find-results-encoding/inventory.md:90` and
  `specs/043-fix-ui-text-encoding/inventory.md:62` — "plugin-supplied metadata;
  revisit when the plugin metadata encoding is defined." 043 even **reverted**
  a 042 conversion in `dialogs5.cpp` on the (half-true) reasoning that
  `p->Name` is ANSI (`src/dialogs5.cpp:832-838`). The assumption holds only for
  loaded plugins; nothing anywhere records that the encoding is
  **load-state-dependent**.
- **What made a dormant line visible**: feature 038 first shipped Czech *plugin*
  language modules (before that, plugin names were English/ASCII), so
  non-ASCII names started flowing through the cache → the decade-old ANSI
  listview line began corrupting. Feature 051's translation changes did **not**
  re-break anything (`b617735` touched only sftp.slt + tooling).

## Existing guards — all blind to this defect

| Guard | Why it misses |
|---|---|
| `tools/check_encoding.py` (wired at `build.cmd:194-221`) | `SINK_LISTVIEW` matches the call, but `UTF8_IDENT` (lines 112-117) enumerates only `f->Name`, `item->Name`, `oneFile->Name` — `plugin->Name`/`p->Name` absent. Reports 0 findings with the defect present. Also silently skipped if python is not on PATH (`build.cmd:214`). |
| `src/saltests/saltests.cpp:802-927` (042/043 tests) | Helper-level property tests only; nothing exercises registry → `AddNamesToListView`. |
| `lang_policy.ps1`, `verify_slg.ps1`, `tools/translate/validate.py` | Policy/FILEVERSION/accelerator checks; no encoding rules. |

043's own diagnosis of 042's guard applies verbatim to the current one:
"it described two EXAMPLES, not the defect."

## Second, independent defect found: mistranslated plugin identifier

The ZIP plugin's display name (`IDS_PLUGINNAME` = English `"ZIP"`,
`src/plugins/zip/lang/lang.rc2:27`) was machine-translated as the postal-code
term by DeepL in feature 038 (commit `220cb72`):

- czech `zip.slt:311` → `"PSČ"`; slovak → `"PSČ"`; french → `"Code postal"`;
  spanish → `"Código postal"` (german `"ZIP-Archiv"`, others `"ZIP"` are fine).
- Stored and displayed "correctly" — it is a translation-content bug, not
  encoding. `translations/ui-overrides.json` has no protection for
  `IDS_PLUGINNAME` entries.

## Fix directions for the planning phase (not decided here)

1. **Define the encoding of `CPluginData::Name`** (and sibling translated
   metadata fields: Description, Copyright, …) — one contract, both producers
   normalized to it; this is the root-cause fix 042/043 deferred.
2. Render the name column through a tolerant/wide path
   (`SalListViewSetItemTextU8` exists) — necessary but not sufficient alone;
   without (1) the field remains a mixed-encoding trap for every future consumer.
3. Extend `tools/check_encoding.py` so the rule describes the defect class
   (registry-read values / plugin metadata into ANSI sinks), not just known
   identifiers; add a test that exercises registry → listview with non-ASCII
   names; make the build fail loudly when the checker cannot run.
4. Correct the four mistranslated ZIP plugin names and protect
   plugin-identifier strings from machine translation (do-not-translate list
   or `ui-overrides.json` entries).
5. Forward-looking: the CP1250/ANSI pathway cannot represent non-Latin
   scripts — relevant to re-enabling Russian/Ukrainian/Chinese (currently off
   per `languages.cfg`); the chosen contract should not bake in the system
   code page.
