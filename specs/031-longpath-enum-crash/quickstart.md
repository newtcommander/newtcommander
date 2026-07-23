# Quickstart: Build, Test & Verify (031)

## Build

```batch
:: from repo root; OPENSAL_BUILD_DIR is unset on this machine -> defaults to .\build\
build.cmd                 :: Debug x64 incremental (also builds saltests)
build.cmd full release    :: Release x64 (the configuration the user runs)
```

Outputs: `build\salamander\Debug_x64\` and `build\salamander\Release_x64\`.

## Automated tests

```powershell
& '.\build\salamander\Debug_x64\saltests\saltests.exe'
# expected: "saltests: <N> checks, 0 failed", exit code 0
```

New in 031: `TestLongComponentNames` (UTF-8 byte-length invariants for the
215-char repro name and 255-char worst cases, `SalConvertFindDataW`
round-trip and fail-safe) and a `TestFileIO` extension that creates the
user's exact 215-character diacritics directory name on disk under
`%TEMP%\saltests-deep\`, enumerates it, and asserts byte-exact retrieval.

Compile-time fence: `static_assert(sizeof(<buf>) >= SAL_FIND_NAME_U8 + 4)`
at every fixed site — shrinking a buffer back to `MAX_PATH + 4` fails the
build.

## Live repro verification

The original reproduction directory (verified to exist):
`D:\Temp\ýášřtščýáíf buaweýáh …` (215 chars, 330 UTF-8 bytes).

1. Start `build\salamander\Release_x64\salamand.exe`.
2. Navigate a panel to `D:\Temp` (the crash used to fire on first paint).
3. Switch view modes: Brief, Detailed, Icons, Thumbnails, Tiles (sites 1–3,
   7–8) — panel must stay responsive, name rendered in full.
4. Focus the long-named directory, check the Type column renders (site 6),
   enter it, go back up.
5. Confirm no new dump appears in `%LOCALAPPDATA%\CrashDumps`.

To re-create the repro directory if it is ever deleted (PowerShell):

```powershell
$u = 'ýášřtščýáíf buaweýáh čáíhšáífšfhčíáéfšh dnf'
New-Item -ItemType Directory -LiteralPath ("D:\Temp\" + ($u * 5))
```

## Crash-dump analysis (if ever needed again)

WER dumps land in `%LOCALAPPDATA%\CrashDumps`. This feature's analysis used
a minimal DbgEng-based dumper (session scratchpad `dumpstack.cpp`):
`dumpstack.exe <dump> <dir-with-matching-pdb>` prints the exception and the
symbolized crash stack. The 031 dumps showed
`FAST_FAIL_STACK_COOKIE_CHECK_FAILURE` in `CFilesWindow::DrawIcon`
(`fileswn4.cpp`).

## Formatting

```powershell
# clang-format check of touched files (repo .clang-format applies)
clang-format --dry-run --Werror src\fileswn4.cpp src\fileswnb.cpp src\salamdr4.cpp src\filesbx1.cpp src\fileswn0.cpp src\saltests\saltests.cpp
```
