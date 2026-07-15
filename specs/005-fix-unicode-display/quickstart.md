# Quickstart: Correct Display of Unicode File Names in Dialogs and Text Fields

**Feature**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md)

How to build and verify the fix for the mojibake defect in the Rename
dialog and the other audited name-bearing text surfaces
(SC-001…SC-004).

## Prerequisites

- Windows 11, Visual Studio 2022 with C++ Desktop workload
- Repo cloned, `OPENSAL_BUILD_DIR` set (optional; defaults to `.\build\`)
- Test fixtures from feature 004 (recreate below if missing)

## Build

```batch
build.cmd full           :: complete Debug x64 build incl. runtime data
```

## Test fixtures

The feature-004 fixture folder is reused. If
`%TEMP%\salamander-test` is missing, recreate the parts needed here
(PowerShell):

```powershell
$base = "$env:TEMP\salamander-test"
New-Item -ItemType Directory -Force $base | Out-Null
# The two visually identical directories from the bug report:
New-Item -ItemType Directory -Force "$base\$([char]0x010D)-dir" | Out-Null        # NFC:  č (U+010D)
New-Item -ItemType Directory -Force "$base\c$([char]0x030C)-dir" | Out-Null       # NFD:  c + U+030C
# Unicode sample set for the audit sweep:
$u = "$base\unicode"; New-Item -ItemType Directory -Force $u | Out-Null
Set-Content -LiteralPath "$u\$([char]0x010D).txt" -Value "NFC" -Encoding utf8
Set-Content -LiteralPath "$u\c$([char]0x030C).txt" -Value "NFD" -Encoding utf8
Set-Content -LiteralPath "$u\δοκιμή.txt" -Value "greek" -Encoding utf8
Set-Content -LiteralPath "$u\テスト.txt" -Value "japanese" -Encoding utf8
Set-Content -LiteralPath "$u\📁report.txt" -Value "emoji" -Encoding utf8
```

Verify stored code points at any time:

```powershell
Get-ChildItem -LiteralPath $base | ForEach-Object {
  "$($_.Name) => " + (($_.Name.ToCharArray() | ForEach-Object { 'U+{0:X4}' -f [int]$_ }) -join ' ') }
```

## Verification walkthrough

Run Salamander, navigate the active panel to
`%TEMP%\salamander-test`. Every step must also be repeated once with a
plain-ASCII name (regression guard, SC-004).

| # | Action | Expected | SC |
|---|--------|----------|----|
| 1 | F2 on the NFC `č-dir` | Edit field shows exactly `č-dir` (not `ÄŤ-dir`) | SC-001 |
| 2 | F2 on the NFD `č-dir` | Edit field shows text visually identical to `č-dir` (not `cĚŚ-dir`) | SC-001 |
| 3 | Confirm step 1/2 dialog **unchanged** | No error; directory name on disk byte-identical (check code points; NFD stays NFD) | SC-001 |
| 4 | F2 on `č-dir`, edit to `řž-dir`, OK | Directory renamed to exactly `řž-dir` | SC-003 |
| 5 | F2, rename to `Тест-测试` (outside ACP) | Renamed exactly; no `?` substitution, no "invalid name" error | SC-003 |
| 6 | Reopen F2 → drop down the history list | Previously typed Unicode names listed unmangled | SC-002 |
| 7 | F5 (Copy) on a Unicode-named item | "Copy … to:" subject and pre-filled target path render correctly (both plain and Options/F10 variant) | SC-002 |
| 8 | F7 (new directory) named `テスト-dir` | Created exactly; dialog text correct | SC-003 |
| 9 | Shift+F7 (Change Directory), paste `%TEMP%\salamander-test\č-dir` | Combo shows the path correctly; navigation works; history entry correct | SC-002 |
| 10 | Alt+F7 Find: "Look in" shows a Unicode path; search `č*`; open result | Path combo + results list + opened item all correct | SC-002 |
| 11 | Delete (F8) prompt on `č-dir`; provoke an error prompt (e.g. rename to an existing name) | File name rendered correctly in every prompt | SC-002 |
| 12 | Pack `unicode` folder to ZIP (F5 → plugin dialog), then unpack with a Unicode target path | Plugin dialog paths render and apply correctly | SC-002/003 |
| 13 | F3/F4 on `テスト.txt` from inside the ZIP archive | Viewer opens it (file-cache name validation accepts Unicode) | SC-002 |
| 14 | Enter `č-dir`: check the panel **directory line** and bottom info line | Path renders correctly, hot-track segments still clickable | SC-002 |
| 15 | Copy a large Unicode-named file (progress dialog visible) | Source/Target paths in progress dialog render correctly | SC-002 |
| 16 | Provoke a message box naming `č-dir` (e.g. F8 delete prompt) | Name correct in message box text | SC-002 |
| 17 | Directory-history menu (Alt+F12), hot-paths menu, drive menu (Alt+F1) | Unicode paths/labels render correctly in menus | SC-002 |
| 18 | Type a path containing `č-dir` in the bottom **command line**, Enter | Command executes against the right directory; history entry correct | SC-002/003 |
| 19 | FTP/renamer/filecomp plugin dialog with a Unicode path (spot-check) | Fields render and round-trip correctly (winliblt fix) | SC-002/003 |
| 20 | All steps repeated with `plain-ascii.txt` | Behavior identical to previous build | SC-004 |

## Audit closure check (FR-005 / SC-002)

The audit inventory lives in the tasks/implementation notes of this
feature. Before sign-off, walk the inventory and confirm every surface
marked DEFECTIVE has a fix commit and a ✓ verification entry with the
sample-name set (NFC, NFD, Greek, Japanese, emoji).

Note (from 004): emoji may render as a placeholder box in the *panel*
font — that is the documented font-fallback cosmetic limitation, not
an encoding defect; in dialogs the name must still round-trip
byte-exactly.
