# `tools/translate` — offline translation tooling

Produces the translation source that `src\vcxproj\build_langs.cmd` turns into
shipped `.slg` language modules.

**This package is never invoked by the build.** It is hand-run by a maintainer,
like `tools/brand/gen_icons.py`. Its output is committed to `translations/` and
the build consumes it as ordinary source. That is what keeps `build.cmd` offline
and reproducible (spec FR-023, FR-024).

Full workflow: `specs/038-translations-build-integration/quickstart.md`.

---

## Install

```bat
pip install -e tools
set ANTHROPIC_API_KEY=sk-ant-...     :: stage 2 only
```

`ant auth login` works too — the SDK picks up the profile automatically.

---

## The three stages

```
                 build.cmd full
                       │
                       ▼
  ┌──────────────────────────────────────────┐
  │ 1. english.slg  ──►  template .slt       │   build_langs.cmd --export-templates
  └──────────────────────────────────────────┘   (not committed)
                       │
                       ▼
  ┌──────────────────────────────────────────┐
  │ 2. template + legacy + machine           │   python -m translate.merge
  │      ──►  translations/<lang>/<mod>.slt  │   (COMMITTED)
  └──────────────────────────────────────────┘
                       │
                       ▼
  ┌──────────────────────────────────────────┐
  │ 3. english.slg + .slt  ──►  <lang>.slg   │   build_langs.cmd
  └──────────────────────────────────────────┘   (shipped)
```

Only stage 3 runs on every build.

---

## Modules

| Module | Purpose |
|---|---|
| `config.py` | Reads `translations/languages.cfg` and `plugins.cfg`; enumerates the (module × language) matrix |
| `slt.py` | `.slt` reader/writer with byte-exact round-trip; `--verify` mode |
| `rebrand.py` | Predecessor product/vendor names and legacy URLs → Newt Commander identity |
| `validate.py` | Placeholder / accelerator / shortcut-label preservation checks |
| `translate.py` | Anthropic Batches API driver (`claude-opus-5`) |
| `merge.py` | Orchestrates: template + legacy + machine → committed `.slt` |

---

## Commands

```bat
:: Inspect the configured matrix
python -m translate.config

:: Verify the .slt reader/writer round-trips every committed file byte-for-byte
python -m translate.slt --verify

:: Preview what merging would change (no API calls, no writes)
python -m translate.merge --dry-run

:: Fill gaps for everything
python -m translate.merge --all

:: Scope to one language or one module
python -m translate.merge --language ukrainian
python -m translate.merge --module sftp
```

---

## Why `.slt` files are regenerated, never hand-edited

`CData::ImportTextArchive` (`src/translator/trldata.cpp:2301`) walks the
module's parsed resources in array order and requires the file to line up
**positionally**:

```cpp
for (i = 0; i < DlgData.Count; i++) {
    ret &= ITACheckSection(buff, lineNumber, L"DIALOG", dialog->ID);
    for (int j = 0; j < dialog->Controls.Count; j++) { ... }
}
```

One extra, missing, or reordered row and the **entire import is rejected**. This
is why the repository's Salamander-4.0-era `.slt` files cannot be used directly,
and why stage 1 exists: the template establishes the correct structure, and the
merge only ever replaces quoted text (and, where a translation overflows, control
geometry).

Editing a committed `.slt` by hand is fine for a text or width fix. Adding or
removing rows is not.

---

## Provenance

Each `.slt` has a sibling `<module>.origin` recording, per entry, whether the
text is `human`, `machine`, or `english_fallback`. The `.slt` grammar is fixed by
the C++ parser and has no comment syntax, so provenance cannot live inside the
file itself. The sidecar is what lets a translator find exactly the
machine-produced text to review.
