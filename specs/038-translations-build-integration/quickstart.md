# Quickstart: Translations Build Integration

**Feature**: 038-translations-build-integration

Three stages with different cadences. Only stage 3 runs on every build.

| Stage | When it runs | Who runs it | Network |
|---|---|---|---|
| 1. Refresh templates | English resources changed | maintainer | no |
| 2. Merge + machine-translate | new/changed English text, or a new language | maintainer | **yes** (Anthropic API) |
| 3. Build language modules | every `build.cmd full` | the build | no |

Stages 1–2 produce **committed** `.slt` files. Stage 3 consumes them. This split
is what keeps the build offline, reproducible, and single-command (FR-023,
FR-024).

---

## Prerequisites

```bat
set OPENSAL_BUILD_DIR=D:\Build\OpenSal\
```

Plus, for stages 1–2 only:

```bat
:: Python 3.13+ and the tools package
pip install -e tools

:: Anthropic credentials (stage 2 only)
set ANTHROPIC_API_KEY=sk-ant-...
:: or: ant auth login   (the SDK picks up the profile automatically)
```

`translator.exe` is built from the solution — no separate download.

---

## Stage 1 — refresh the English templates

Run after any change to `src/lang/*.rc*` or `src/plugins/*/lang/*.rc*`.

```bat
build.cmd full
src\vcxproj\build_langs.cmd --export-templates
```

For each enabled module this seeds a scratch `.slg` from `english.slg`, emits an
`.atp`, and runs `-quiet-export-slt`. The result is a current-structure,
English-text `.slt` per module under
`%OPENSAL_BUILD_DIR%salamander\translator\templates\`.

These are **not** committed — they are the skeleton stage 2 fills.

> Why this exists: `.slt` import is positional (see
> [contracts/slt-format.md](contracts/slt-format.md)), so translation source
> must always match the *current* resource layout. Templates are how that
> correspondence is established, rather than assumed.

---

## Stage 2 — merge and machine-translate

```bat
:: Preview: what would change, and what it would cost
python -m translate.merge --dry-run

:: Fill the gaps for every language (writes translations/<lang>/<module>.slt)
python -m translate.merge --all

:: One language, e.g. after adding Ukrainian
python -m translate.merge --language ukrainian

:: One module, e.g. after adding SFTP dialogs
python -m translate.merge --module sftp
```

What `merge` does per (language, module):

1. loads the stage-1 template (the authoritative structure);
2. fills each entry from the legacy `.slt` where the ID matches → marked `human`;
3. batches everything still empty to `claude-opus-5` via the Message Batches API
   → marked `machine`;
4. validates placeholders / accelerators / `\t`-shortcuts against the English
   source; failures retry once, then fall back to English and are reported;
5. applies the rebrand pass (product/vendor names, legacy URLs);
6. writes the merged `.slt` plus its `.origin` sidecar.

Batch jobs take up to an hour. Results are keyed by `custom_id`, never by
position. Re-running is safe and incremental — already-filled entries are not
re-translated.

Then review and commit:

```bat
git diff --stat translations/
git add translations/ && git commit
```

---

## Stage 3 — build the language modules

Nothing extra to do; it is part of the normal build:

```bat
build.cmd full
```

Per (module × language) the build seeds `<language>.slg` from `english.slg`,
generates the `.atp`, runs `-quiet-import-slt`, verifies `FILEVERSION`, and runs
`-quiet-validate-layout`. Output:

```
  language modules built: 240 (12 languages x 20 modules)
  coverage: cs 94% human / 6% machine   de 91% / 9%   uk 0% / 100%   ...
  layout warnings: 0
```

To rebuild one pair while iterating:

```bat
src\vcxproj\build_langs.cmd --language czech --module sftp
```

---

## Verifying the result

```bat
%OPENSAL_BUILD_DIR%salamander\Debug_x64\newtcommander.exe
```

1. **First run** — the language chooser lists **12** entries with readable names
   and author credits; the language matching your Windows display language is
   preselected.
2. Pick a non-English language, restart when asked.
3. **No per-plugin prompt** should ever appear. Open an archive (7zip/zip),
   compare files (filecomp), connect over SFTP, view a Markdown file (mdview),
   open a picture (pictview) — each plugin comes up in the chosen language on
   first load.
4. Open the configuration dialog and confirm the active language is shown and
   changeable.
5. Spot-check dialogs for clipped or overlapping controls — `-quiet-validate-layout`
   catches most, but confirm the frequently used ones visually.
6. Check Russian, Ukrainian, and Simplified Chinese render correctly in menus,
   list columns, and message boxes.

---

## Adding a new language

> **Updated by feature 039**: `languages.cfg` gained a required
> `enabled = on|off` field, so the record format described here is no longer
> complete on its own. See
> `specs/039-language-build-policy/contracts/languages-cfg.md`.

1. Add a record to `translations/languages.cfg` (folder, LANGID, display name,
   author, web, comment, `origin = machine`, `enabled = on|off`).
2. `mkdir translations/<folder>`
3. `python -m translate.merge --language <folder>` — every entry is a gap, so
   the whole language is machine-produced. Naming the language explicitly also
   works while it is still `enabled = off`, which is the usual way to prepare
   one before shipping it.
4. Review, commit, `build.cmd full`.

---

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| Build reports import failure for one pair | `.slt` no longer matches the module's resources | Re-run stage 1, then `merge --module <name>` |
| Build hangs on a module | `translator.exe` hit an error path and opened a message box | The timeout guard should kill it; check the named pair. Do not run the build interactively |
| `translator.exe` "succeeded" but the build says it failed | Success is **exit code 1**, not 0 | Script bug — see [contracts/translator-cli.md](contracts/translator-cli.md) |
| Language missing from the chooser at runtime | `.slg` version mismatch, so the engine rejected it | The post-build `FILEVERSION` check should have caught it; confirm the target was seeded from *this build's* `english.slg` |
| Per-plugin language prompt appears | That plugin has no `.slg` for the active language | Check the plugin is `on` in `plugins.cfg` and has a `.slt` for that language |
| Clipped controls in one dialog | Translation longer than the English original | Widen the control in the `.slt` row (`cx` field), or shorten the text; re-run stage 3 for that pair |
| `merge` reports many `english_fallback` entries | Machine translations are dropping placeholders | Inspect the validation report — usually a prompt issue with a specific format string |
