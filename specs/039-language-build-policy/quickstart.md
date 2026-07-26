# Quickstart: Language Build Policy

**Feature**: 039-language-build-policy

How to change which languages ship, and how to verify the change did what it
says.

---

## Change what ships

Edit `translations/languages.cfg` and flip one line:

```ini
[russian]
langid       = 1049
display_name = Russian
...
origin       = mixed
enabled      = off        # <-- this
```

Then build:

```batch
set OPENSAL_BUILD_DIR=D:\Build\OpenSal\
build.cmd
```

That is the whole procedure. A plain `build.cmd` is enough to **remove** a
language — reconciliation runs before MSBuild on every build. Producing a
newly-enabled language needs a full build, because that is where language
modules are made at all:

```batch
build.cmd full
```

The banner tells you what the policy resolved to:

```text
 Plugin policy : 19 plugins enabled (plugins.cfg)
 Lang policy   : 8 of 11 languages enabled (languages.cfg)
```

---

## Verify

### It is gone from the output

```powershell
# OPENSAL_BUILD_DIR defaults to the repository's build\ directory when unset
$root = if ($env:OPENSAL_BUILD_DIR) { $env:OPENSAL_BUILD_DIR } else { '.\build\' }
$out = Join-Path $root 'newtcommander\Release_x64'
Get-ChildItem -Path $out -Recurse -Filter 'russian.slg'      # expect: nothing
(Get-ChildItem -Path "$out\lang" -Filter '*.slg').Count      # expect: 9 (8 + english)
(Get-ChildItem -Path $out -Recurse -Filter '*.slg').Count    # expect: 180 (20 modules x 9)
```

### It is gone from the product

Launch `newtcommander.exe` → **Options ▸ Configuration ▸ Language ▸ the
`Language...` button**. The list has 9 entries: the 8 enabled languages plus
English.

### A user who was running it is not stranded

With the product configured for Russian, disable Russian, rebuild, and start it.
Expected: a message that the file was not found and the product will look for
another language. What follows depends on your Windows display language: if a
shipped language matches it, the product switches to that one automatically; if
none matches, the chooser opens. Either way it is a guided recovery — not a
crash, and not a silent English fallback.

### Re-enabling costs nothing

```batch
:: flip enabled = on, then
build.cmd full
```

The restored `.slg` files are byte-identical to the ones from before disabling,
**given an unchanged `english.slg`** — that file and the committed `.slt` are the
only inputs. If MSBuild relinks a module's `lang_<name>` project between the two
builds, its `english.slg` gets a new PE timestamp and every language module
derived from it changes with it; that affects enabled and disabled languages
alike and is unrelated to the policy. To prove the round trip, hash before and
after **without** an intervening compile:

```powershell
Get-ChildItem $out -Recurse -Filter 'russian.slg' |
  Get-FileHash | Select-Object Hash, Path
```

### The policy file catches mistakes

Each of these must fail the build **before MSBuild runs**, naming the entry:

| Break it like this | Expect |
|---|---|
| `enabled = maybe` | `language [russian] has enabled 'maybe' (expected on or off)` |
| delete the `enabled` line | `language [russian] is missing: enabled` |
| rename a section to `[russain]` | `no translations\russain\ directory` **and** `translations\russian\ exists but is not registered` |
| set every language `off` | build succeeds; product is English-only |

---

## Work on a disabled translation

Disabled languages keep their translation source, and the authoring tools skip
them by default so no budget is spent on something that will not ship:

```batch
cd tools
python -m translate.merge --all              :: enabled languages only
```

To deliberately prepare one before enabling it, name it:

```batch
cd tools
python -m translate.merge --language ukrainian
note: 'ukrainian' is disabled in languages.cfg -- processing it because you named it explicitly
```

The build tool does **not** work this way — it refuses:

```batch
src\vcxproj\build_langs.cmd --language ukrainian
ERROR: language 'ukrainian' is disabled in languages.cfg -- enable it there to build it
```

See the whole registry, shipped or not:

```batch
cd tools
python -m translate.config
```

`translate.*` is a package under `tools/`, so these run from `tools\`, not the
repository root.

---

## Why three languages are off right now

Simplified Chinese, Russian and Ukrainian render incorrectly in menus. They are
disabled so users stop seeing them broken; the defect itself is recorded in
`spec.md` and is **not** fixed by this feature. All three keep their full
translation source. Re-enabling each is one line once the rendering is fixed.
