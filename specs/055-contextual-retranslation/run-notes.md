# Run Notes: Contextual Re-translation (feature 055)

**Date**: 2026-08-07 · **Executed by**: maintainer session on branch
`055-contextual-retranslation` · Raw console log: `run-output.log`

## What ran

1. Templates exported from the current Release_x64 tree (20/20 modules).
2. Baseline (`--dry-run`): **0 gaps** in all 8 enabled languages after the
   `match.py` same-text fix (before it, ~150 translations-identical-to-English
   per language were mis-classified as gaps and re-sent on every run).
3. Contract preview (`--dry-run --redo-machine --exclude-module sftp`):
   gap sets equal the `.origin` machine sets **exactly** at entry level
   (czech 292, german 366, french 403, dutch 559, hungarian 324,
   romanian 650, slovak 300, spanish 363 = 3,257 entries; the coverage
   table's per-row counts differ by +2 fr / +16 nl / +16 ro because a few
   dialog rows share a control ID and are counted per row, planned per key).
4. The live run: `merge --all --redo-machine --exclude-module sftp` —
   3,064 unique (context, text) pairs sent, 100% translated,
   **52,728 characters** spent (quota remaining after: 49,465).
5. Offline replay (no quota) after two tooling corrections found by
   verification — see "Defects found and fixed" below.
6. Override pass: 20 pins added to `ui-overrides.json` for strings the
   context could not fix (see below), applied by a second offline replay.
7. `build.cmd full`: BUILD SUCCEEDED, 0 errors, 180 language modules,
   zero positional-import failures; `build_langs.cmd` re-run after the
   override pass (152 rebuilt + 8 up to date, version check 180 OK).

## Success-criteria results

| Criterion | Result |
|---|---|
| SC-001 scope completeness | PASS — all 3,257 machine-provenance entries re-translated (3,064 unique pairs after dedup); provenance sidecars agree |
| SC-002 human/skip untouched | PASS — 59,360 entries compared against pre-run state: 0 violations; every changed entry had `machine` provenance; `.slt` round-trip byte-exact (290 files) |
| SC-003 spot-check cs+sk | PASS — all 174 changed Czech+Slovak entries reviewed (not a 20-sample); 9 defective results found (≈5%), all corrected via pinned overrides, see below |
| SC-004 build gate | PASS — full build 0 errors, 180 `.slg` modules, all imports positional-clean |
| SC-005 new fallbacks < 2% | PASS — exactly 1 product-wide: spanish `DPI%` (DeepL inserted a `% d`; validation kept English and reported it) |
| SC-006 quota | PASS — 52,728 chars in one run, single free-tier month |

## Quality sample (Czech/Slovak highlights)

Wrong-sense errors fixed by context: FTP «&Přístav:» (harbor!) → «&Port:»,
«Ponožky 4A» (socks!) → «SOCKS 4A», «Shift» as «Změna/Zmena» → «Shift» (key),
dialog captions «Velitel tandemu / Veliteľ tandemu» → «Tandem Commander»,
«Dark» as «Tma» (noun darkness) → «Tmavý», regedt «Údaje» → «Data»,
dbviewer «&Obsah...» (table of contents) → «&Rejstřík...» (index),
mdview «Solární svítidlo» (solar lamp) → «Sluneční světlo», salamand
«&Jméno:» (person's first name) → «&Název:», resource placeholder «dummy»
no longer translated as «atrapa» (physical decoy).

## Defects found and fixed during verification

1. **`dedupe_accelerators` rewrote human rows** — the section-wide
   accelerator matching treated every row as movable, so re-running it
   relocated `&` markers inside 1,148 human-translated menu items (wording
   intact, marker moved). The committed pipeline had always done this; the
   first SC-002 run exposed it. Fixed: human/skip rows are now *frozen*
   obstacles the machine rows dedupe around (`layout.py` + `build_slt`), and
   the state was rebuilt by an offline replay of the obtained translations
   over the pre-run state. Measured effect vs pre-run HEAD: duplicate
   accelerators MENU 222→211, STRINGTABLE 435→379, DIALOG 9→**0** — strictly
   better, with zero human rows touched.
2. **Kuhn matching was exponential** — per-branch copies of the visited set
   made one language's merge take >4 minutes (sections with more accelerated
   rows than free letters); a shared visited set makes it 0.3 s.
3. **Pinned overrides lost their provenance when the engine agreed** — an
   override equal to the engine's output stayed labelled `machine`
   (observed: the ZIP plugin name in 7 of 8 languages). Overrides now always
   record as `human`, which also freezes them for the accelerator dedup and
   keeps them out of future `--redo-machine` populations.

## Pinned overrides added (the context's limits)

- `mdview IDS_MENU_VIEW` — "&View" survived untranslated in cs/fr/nl/ro/es,
  lower-cased genitive in sk, "V&iew" in hu → pinned to each language's
  standard menu term (Zobrazit/Zobraziť/Affichage/Beeld/Nézet/Vizualizare/Ver).
- `mdview IDS_MENU_VIEW_FOLLOWSYS` — "Follow S&ystem Theme" garbled in 7
  languages (the verb "Follow" kept as a name) → pinned everywhere but
  english.
- `mdview IDS_PLUGINNAME` (czech) — DeepL appended a stray „ quote.
- `mdview #Graphite (dark)` / `#Nordic Dark` (czech) — theme list consistency.
- `salamand IDS_FINDLOG_TEXT` (czech, slovak) — "Text" came back lower-cased.

## Residual notes

- The one spanish `DPI%` fallback is re-attempted (4 chars) by any future
  run — inherent fallback-retry behaviour, harmless.
- `.origin` provenance is per entry key; the few dialog rows that share a
  control ID receive identical treatment and text (pipeline-wide invariant,
  predates this feature).
- Final idempotence: plain `--dry-run` reports 0 gaps in all languages
  (spanish 1 = the fallback retry above).
