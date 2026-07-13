# Quickstart: Long Path and Unicode File Name Support

**Feature**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md)

How to build, create test fixtures, and verify the feature's success
criteria (SC-001…SC-009) on a development machine.

## Prerequisites

- Windows 11, Visual Studio 2022 with C++ Desktop workload
- Repo cloned, `OPENSAL_BUILD_DIR` set (optional; defaults to `.\build\`)
- No registry changes required — long-path support must work
  regardless of `HKLM\...\FileSystem\LongPathsEnabled` (the
  implementation uses extended-length paths internally)

## Build

```batch
build.cmd rebuild        :: full Debug x64 rebuild from repo root
```

## Create test fixtures

Run in PowerShell (uses `\\?\` so the fixtures can be created even
before/without the fix):

```powershell
# --- 1. Deep path fixture (total path > 300 chars) ---
$base = "$env:TEMP\salamander-test"
$seg  = "a" * 60
$deep = "\\?\$base\$seg\$seg\$seg\$seg\$seg"
New-Item -ItemType Directory -Force $deep | Out-Null
Set-Content -Path "$deep\deep-file.txt" -Value "content at long path" -Encoding utf8

# --- 2. Unicode name fixtures (NFC vs NFD, non-ACP, non-BMP) ---
$u = "\\?\$base\unicode"
New-Item -ItemType Directory -Force $u | Out-Null
# NFC: precomposed č (U+010D)
Set-Content -LiteralPath "$u\$([char]0x010D).txt" -Value "NFC" -Encoding utf8
# NFD: c + combining caron (U+030C) — visually identical to the NFC file
Set-Content -LiteralPath "$u\c$([char]0x030C).txt" -Value "NFD" -Encoding utf8
# Outside a Czech/Western ACP: Greek, Japanese
Set-Content -LiteralPath "$u\δοκιμή.txt" -Value "greek" -Encoding utf8
Set-Content -LiteralPath "$u\テスト.txt" -Value "japanese" -Encoding utf8
# Non-BMP (surrogate pair): emoji
Set-Content -LiteralPath "$u\📁report.txt" -Value "emoji" -Encoding utf8

# --- 3. Combined stress: NFD name at a deep path ---
Set-Content -LiteralPath "$deep\c$([char]0x030C)-deep.txt" -Value "NFD deep" -Encoding utf8
```

## Verification walkthrough

Map each check to a success criterion; compare behavior with Windows
Explorer side by side.

| # | Action in Salamander | Expected | SC |
|---|----------------------|----------|----|
| 1 | Navigate panel into the `$deep` folder | Full listing shown, path in title/panel correct | SC-001 |
| 2 | Copy/move/rename/delete `deep-file.txt`; create a subfolder there | All succeed; F3 viewer opens the file | SC-001 |
| 3 | Open the `unicode` folder | All six names render exactly as in Explorer (both "č.txt" entries visible, no `?`) | SC-002, SC-003 |
| 4 | Copy the whole `unicode` folder elsewhere and back | Names preserved bit-exactly (verify with `Get-ChildItem -LiteralPath | % {[int[]][char[]]$_.Name}`) | SC-004 |
| 5 | In the `unicode` folder type `č` (quick search) | Selection cycles through BOTH the NFC and NFD file | SC-005 |
| 6 | Find (Alt+F7) for `č*.txt` over `$base` | Both composition forms found, incl. the deep NFD file | SC-005 |
| 7 | Copy the NFD `č.txt` into a folder already containing the NFC one | Both coexist (no overwrite prompt) + one-time notice about the indistinguishable pair | FR-007 |
| 8 | Clipboard: copy files from Salamander, paste in Explorer (and back) | Transfer completes, names preserved | SC-001/002 |
| 9 | Pack the `unicode` folder to ZIP and extract into `$deep` | All entries preserved (bundled plugin parity) | SC-008 |
| 10 | Regression pass on ordinary ASCII paths (browse, copy, sort, config save/restart) | Identical behavior to previous release | SC-006 |

## Performance check (SC-009)

```powershell
# Fixture: 100k files in one directory
$perf = "$base\perf"; New-Item -ItemType Directory -Force $perf | Out-Null
1..100000 | ForEach-Object { New-Item -ItemType File "$perf\f$_.txt" } | Out-Null
```

Measure panel listing time (status-bar ready) and full re-sort on the
`perf` directory in the previous release vs. this build — elapsed time
must be within ±10%.

## Legacy plugin degradation (FR-014, SC-007)

With a plugin built against the legacy interface (keep one bundled
plugin un-migrated in a dev build, or use an old third-party `.spl`):

1. Direct an NFD-named file to it (e.g. pack into its archive format).
2. Expected: the item is refused with a per-item message naming the
   file; remaining items complete; nothing is silently renamed.
