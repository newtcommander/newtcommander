# Quickstart: Fix About Dialog Copyright Notice

**Feature**: 040-fix-about-copyright

## Build

A **full** build is required — language modules (`.slg`) are produced only on a
full build, and this feature edits translation archives.

```batch
set OPENSAL_BUILD_DIR=D:\Build\OpenSal\
build.cmd full
```

Without `OPENSAL_BUILD_DIR` the build defaults to `.\build\`, which is fine for
verification.

Output of interest:

```text
%OPENSAL_BUILD_DIR%\newtcommander\
├── newtcommander.exe
└── lang\
    ├── english.slg
    ├── czech.slg
    ├── german.slg  … (8 enabled languages)
```

## Verify — the fix itself

### 1. About dialog, English

Launch `newtcommander.exe`, then **Help → About Newt Commander**. The two
copyright lines must read, in this order:

```text
Copyright © 2026 Newt Commander Authors
Copyright © 1997-2026 Open Salamander Authors
```

### 2. About dialog, Czech (the reported case)

**Options → Configuration → Language** (or delete the language value under
`HKCU\Software\Newt Commander\0.1` to get the chooser at startup), pick Czech,
restart, reopen About. The two lines must be byte-identical to the English case
— *not* translated, *not* reordered, *not* dated 2023.

Repeat for the other enabled languages: German, French, Dutch, Hungarian,
Romanian, Slovak, Spanish.

### 3. Splash screen

Ensure the splash is enabled (**Options → Configuration → Main Window → Show
splash screen at startup**) and restart. The two lines at the bottom must be in
the same order as in the About dialog — Newt Commander first.

### 4. Both themes

Toggle the application theme and repeat step 1. Both lines must stay legible and
unclipped in light and dark.

## Verify — the invariants

### `LegalCopyright` unchanged (FR-012)

```powershell
(Get-Item "$env:OPENSAL_BUILD_DIR\newtcommander\newtcommander.exe").VersionInfo.LegalCopyright
```

Must print exactly:

```text
Copyright © 1997-2026 Open Salamander Authors, © 2026 Newt Commander Authors
```

### No copyright text left in any translation archive (FR-009a, SC-004a)

```bash
grep -nE '^(1150,10,97,196,8|1151,10,108,196,8),1,"..*"' translations/*/salamand.slt
```

Must print nothing. (Before the fix it prints 22 rows.)

### Archives structurally intact (INV-5)

```bash
python - <<'PY'
import glob
for p in sorted(glob.glob('translations/*/salamand.slt')):
    d = open(p, 'rb').read()
    assert d.startswith(b'\xef\xbb\xbf'), p
    assert d.count(b'\r\n') == d.count(b'\n'), p
    print(f"{p}: BOM ok, CRLF ok, {d.count(b'\n')} rows")
PY
```

Row counts must match the pre-change values (recorded in tasks.md T009).

### The translation source stays clean across a round-trip

Optional, but this is what proves the notice cannot come back:

```bash
python -m tools.translate.merge --language czech --module salamand --dry-run
```

The two copyright rows must remain empty — there is no English text for the
merge to translate.

## Rollback

Every change is source-only:

```bash
git checkout -- src/versinfo.rh2 src/logo.cpp src/lang/lang.rc translations/
```

then rebuild.
