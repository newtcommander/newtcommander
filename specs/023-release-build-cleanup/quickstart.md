# Quickstart / Verification: Clean Release Build Output

**Feature**: 023-release-build-cleanup

## Build

```bat
set OPENSAL_BUILD_DIR=E:\Projects\salamander\build\
build.cmd full release
```

(A plain `build.cmd release` or `build.cmd rebuild release` is also fine; the
post-build sweep runs for any Release build.)

## Acceptance checks

Let `OUT = %OPENSAL_BUILD_DIR%newtcommander\Release_x64`.

### SC-001 — no `Intermediate` directory anywhere in the Release tree

```powershell
Get-ChildItem -LiteralPath $OUT -Directory -Recurse -Filter Intermediate
# Expected: no output (zero matches)
```

### SC-002 — no `saltests` directory in the Release tree

```powershell
Test-Path (Join-Path $OUT 'saltests')
# Expected: False
```

### SC-003 — all runtime deliverables still present

```powershell
Test-Path (Join-Path $OUT 'salamand.exe')       # True
Test-Path (Join-Path $OUT 'salamand.pdb')        # True
Test-Path (Join-Path $OUT 'lang\english.slg')    # True
Test-Path (Join-Path $OUT 'plugins\plugins.ver') # True
Test-Path (Join-Path $OUT 'convert')             # True
Test-Path (Join-Path $OUT 'toolbars')            # True
Test-Path (Join-Path $OUT 'utils')               # True
(Get-ChildItem (Join-Path $OUT 'plugins') -Filter *.spl -Recurse).Count  # > 0
```

### SC-004 — incremental Release build does not full-recompile

```bat
build.cmd release
:: run it a second time immediately:
build.cmd release
```

The second run must be a fast incremental build (only up-to-date checks), not a
full recompilation — confirming the object cache under `obj\` was reused.

### SC-005 — Debug output unchanged (still has Intermediate + saltests)

```bat
build.cmd
```

```powershell
$DBG = "$env:OPENSAL_BUILD_DIR" + 'newtcommander\Debug_x64'
Test-Path (Join-Path $DBG 'Intermediate')  # True
Test-Path (Join-Path $DBG 'saltests')      # True
```

### SC-006 — unit tests still build & run in Debug

The Debug build produces `…\newtcommander\Debug_x64\saltests\saltests.exe`; run it
and confirm the tests pass.

## Where do the intermediates live now?

Under `%OPENSAL_BUILD_DIR%obj\Release_x64\…` — outside the shipped tree, still
persistent so incremental builds stay fast.
