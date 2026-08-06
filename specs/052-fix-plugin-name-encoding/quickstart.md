# Quickstart: Validating the Plugin Name Encoding Fix

**Feature**: 052-fix-plugin-name-encoding · Phase 1 artifact

> **Validation results (2026-08-06, this machine, Czech UI, ACP=1250)**
>
> - **Scenario 1 PASS** — Debug build launched twice; second start showed
>   Správce pluginů with „celkem: 19, načtených: 0" and every name correct
>   („Hromadné přejmenování", „Kontrolní součet", „Obnovení souborů a
>   adresářů", …) — all rows fed from the registry cache, zero plugins
>   loaded (screenshot captured via PrintWindow during automated run).
> - **Scenario 2 PASS** — registry export before/after a clean run+exit-save:
>   byte-identical. The first run after the full build re-registered plugins
>   (plugins.ver bump — pre-existing behavior) and the fixed save path wrote
>   correct UTF-16, including „ZIP" replacing „PSČ".
> - **Scenario 3 PASS** — seeded `ListView_SetItemText(… plugin->Name)` →
>   checker flagged `utf8-to-legacy-sink` at the exact line; seeded
>   `LoadStr` template → `mixed-composition` flagged; python removed from
>   PATH → build fails with the prerequisite message (exit 1).
> - **Scenario 4 PASS** — saltests: 1145 checks, 0 failed (includes
>   TestPluginMetadataEncoding; CP1250 exact-bytes branch executed).
> - **Scenario 5 PASS** — all built `zip` language modules report string
>   1007 = "ZIP" (czech/slovak/french/spanish/german/english dumped via
>   LoadStringW); `ui-overrides.json` pins IDS_PLUGINNAME="ZIP" for all 11
>   non-English folders and parses cleanly.
> - **Scenario 6** — `build.cmd` and `build.cmd full` green (0 errors);
>   clean rebuild + `full release` run recorded in the implementation notes.

## Prerequisites

- Windows 11, VS2022 C++ workload, Windows SDK (repo standard)
- **Python 3.x on PATH** — from this feature on, `build.cmd` fails without it
  (the encoding guard must be able to run)
- Czech UI language available (built by default; `translations/languages.cfg`)

## Build

```batch
build.cmd full            :: Debug x64 + runtime data + plugins.ver
```

Expected: build succeeds; `Encoding guard` line reports it ran (not SKIPPED —
that state no longer exists; missing python fails the build).

## Scenario 1 — Mojibake gone for not-loaded plugins (SC-001, US1)

1. Start `tandemcommander.exe`, switch UI language to Czech (Nastavení →
   jazyk), let the app restart/reload language.
2. Exit the application completely (configuration save writes the cache).
3. Start it again — do **not** open any archive or plugin function (so most
   plugins stay not-loaded).
4. Open Správce pluginů (Plugins Manager).

**Expected**: every row shows correct Czech diacritics — e.g.
„Hromadné přejmenování", „Kontrolní součet", „Obnovení souborů a adresářů",
„Porovnání souborů", „Prohlížeč databází" — for rows with „Načteno = Ne" as
well as „Ano". Compare against the bug screenshot `temp/plugins_strings.png`.

5. Load one plugin (e.g. F3 on a file handled by a viewer plugin), reopen the
   dialog. **Expected**: its row text is unchanged (identical before/after
   load — edge case in spec).

## Scenario 2 — Existing installation, no data rewrite (SC-002, US2)

1. On a machine/registry state that reproduced the bug (or after running an
   affected build once): export `HKCU\Software\Tandem Commander\0.1\Plugins`
   to a `.reg` file.
2. Install/run the fixed build; open Plugins Manager. **Expected**: names
   correct immediately.
3. Export the key again before any plugin loads and diff against step 1.
   **Expected**: byte-identical `Name` values (the fix reads existing data
   correctly instead of rewriting it; values change only through normal
   re-registration when `plugins.ver` bumps — that is pre-existing behavior).

## Scenario 3 — Guard catches the defect class (SC-003, US3)

1. Healthy tree: `python tools/check_encoding.py --strict` → exit 0.
2. Seed the defect: temporarily revert the name-column call in
   `src/plugins2.cpp` (`SalListViewSetItemTextU8` → `ListView_SetItemText`).
   Run the checker. **Expected**: non-zero exit, finding names the file/line
   (`utf8-to-legacy-sink`).
3. Seed a mixed composition: restore one removed
   `sprintf(buf, LoadStr(...), plugin->Name)` form. **Expected**:
   `mixed-composition` finding.
4. Remove python from PATH (e.g. `set PATH=` in a scratch shell) and run
   `build.cmd`. **Expected**: build **fails** with a message that the
   encoding guard requires python (no silent skip).
5. Revert all seeds.

## Scenario 4 — Runtime tests (D4.2)

Run the saltests suite (Debug build, per `src/saltests/` convention — same as
features 042/043). **Expected**: new plugin-metadata encoding tests pass:
registry facade round-trip (ANSI-in/UTF-8-in), normalization helper
properties, tolerant listview helper.

## Scenario 5 — ZIP named "ZIP" everywhere (SC-005, US4/FR-007)

1. After `build.cmd full`, check built modules: for each enabled language,
   the zip module's `IDS_PLUGINNAME` (id 1007) is `"ZIP"`:
   `grep -H "^1007,1," translations/*/zip.slt` → every line reads `"ZIP"`.
2. In the app: switch to Czech → Plugins Manager shows „ZIP" (not „PSČ");
   spot-check German and French the same way.
3. Re-translation protection: `translations/ui-overrides.json` contains
   `zip → <language> → IDS_PLUGINNAME: "ZIP"` for all 10 non-English folders;
   `tools/translate/merge.py` coverage report lists them as `human`.

## Scenario 6 — Full build regression

`build.cmd rebuild` (clean Debug) and `build.cmd full release` complete;
`CHANGELOG.md` contains the Fixed/Changed entries for this feature.
