# Contract: `translator.exe` Quiet-Mode CLI

**Status**: **Consumed, frozen.** Defined by
`CFrameWindow::ProcessCmdLineParams` (`src/translator/wndframe.cpp:562`) and the
dispatch block at `wndframe.cpp:344-535`. The build scripts must conform; they
do not get to change the tool.

**Binary**: `translator.exe`, built from `src/vcxproj/translator/translator.vcxproj`
(already in `salamand.sln`, GUID `{C5833A09-5056-4C59-9E53-721A4D011936}`).

---

## Invocations used by this feature

| Command | Arity | Effect |
|---|---|---|
| `translator.exe -quiet-export-slt <dir> <project.atp>` | 3 | Writes `<dir>\<project-basename>.slt` from the project's **translated** module |
| `translator.exe -quiet-import-slt <dir> <project.atp>` | 3 | Reads `<dir>\<project-basename>.slt`, applies it, saves the translated `.slg` and the project |
| `translator.exe -quiet-validate-layout <project.atp>` | 2 | Layout-only validation (`QuietValidate = 2`) |
| `translator.exe -quiet-validate-all <project.atp>` | 2 | Full validation (`QuietValidate = 1`) |
| `translator.exe -quiet-export-sizes <dir> <project.atp>` | 3 | Writes `<dir>\<basename>.sdc` — dialog/control geometry |

Argument count is checked strictly (`wndframe.cpp:572` `if (p > 3) return;`) —
an unexpected extra argument makes the tool silently fall through to interactive
mode and **open a window**, which in a build means a hang until the timeout.

---

## Exit codes — inverted from the usual convention

```cpp
DestroyWindow(HWindow);
ExitProcess(1);   // "exit code 1 means validation succeeded"
```
— `wndframe.cpp:365`, `:461`, `:532`

| Exit code | Meaning |
|---|---|
| **1** | **Success** |
| 0 | Operation completed but reported "nothing to do" (`-quiet-translate` with a completely untranslated module; `-quiet-mark-changed-as-translated` with changes saved) |
| anything else / no exit | Failure — the tool fell through to interactive mode or hit an error path |

**A build script that treats non-zero as failure will reject every successful
run.** Test for `ERRORLEVEL == 1`.

---

## The hang risk, and how to contain it

`translator.exe` is a GUI application (`WinMain`, `translator.cpp:558`). Its
error paths call `MessageBox` — `LoadProject` failures (`dataprj.cpp:604`),
read failures, and the import syntax error (`trldata.cpp:2594`) all block on a
modal dialog with no console output.

**Required by every invocation:**

1. Run under a **timeout** (30 s is generous for a single module). A timeout is a
   build failure with a clear message naming the (language, module) pair — never
   an unbounded wait.
2. Kill the process on timeout so the build can continue to report.
3. Never run interactively from a build agent.

There is one prompt on the *success* path — the "you have made changes since the
last import" confirmation (`trldata.cpp:2360`) — but it is guarded by
`IsSLGSignature.IsSLTDataChanged()`, which returns `FALSE` when
`SLGCRCofImpSLT == "none"`. A fresh copy of `english.slg` carries exactly that
value (`src/plugins/shared/versinfo.rc2:63`), so **seeding the target from
`english.slg` suppresses it**. This is a precondition, not an optimization.

---

## Preconditions for `-quiet-import-slt`

All must hold or the run fails (usually with a modal dialog):

| # | Precondition | Why |
|---|---|---|
| 1 | `<project.atp>` exists and parses | `Data.LoadProject` (`dataprj.cpp:599`) |
| 2 | `Original=` points at an existing, readable `english.slg` | loaded as the source module |
| 3 | `Translated=` points at an **already existing** `.slg` | `Save()` starts with `CopyFile(FullTargetFile, …)` (`trldata.cpp:328`) — it patches a copy, it cannot create one |
| 4 | `Include=` points at an existing `lang.rh` | `OpenProject` gates on `DataRH.Load` (`wndframe.cpp:344`) |
| 5 | The target `.slg` has `SLGCRCofImpSLT == "none"` | suppresses the modal prompt (above) |
| 6 | `<dir>\<basename>.slt` exists and matches the module **positionally** | see [slt-format.md](slt-format.md) |
| 7 | The target `.slg` is not locked | a running Newt Commander holds its `.slg` open — the tool works around this for `.bak` but not for the target |

Precondition 3 is the one that surprises: the pipeline must **copy
`english.slg` → `<language>.slg` first**, then import.

---

## Path resolution

- The `<dir>` argument may be absolute (detected by `\\` prefix or `X:`) or
  relative to the `.atp`'s directory (`wndframe.cpp:481-489`).
- The output/input filename is derived from the **`.atp` basename** with the
  extension swapped (`wndframe.cpp:490-493`) — so the project **must** be named
  `<module>.atp` to read/write `<module>.slt`.
- Paths inside the `.atp` (`Original`, `Translated`, `Include`) are resolved
  relative to the `.atp` file (`dataprj.cpp:419-432` — `PathRemoveFileSpec` +
  `PathMerge`). Absolute paths are simplest and are what the generator emits.

---

## Side effects

`-quiet-import-slt` writes more than the target:

| Path | Contents |
|---|---|
| `<target>.slg` | the patched language module (via `.tmp` → rename) |
| `<target>.bak` | previous version, or `<target> (2).bak`, `(3).bak`… if locked |
| `<project>.atp` | rewritten by `Data.SaveProject()` — translation states are persisted back |

Because `.atp` files are build intermediates under `OPENSAL_BUILD_DIR`, the
rewrite is harmless. `.bak` accumulation in the output tree should be cleaned by
the build script.

---

## Reference invocation

```bat
:: 1. seed the target (precondition 3 + 5)
copy /Y "%LANGDIR%\english.slg" "%LANGDIR%\czech.slg" >nul

:: 2. generate the project (Original / Translated / Include, absolute paths)
powershell -File gen_atp.ps1 -Module salamand -Language czech -Out "%ATPDIR%"

:: 3. import — success is ERRORLEVEL 1, guarded by a timeout
"%TRANSLATOR%" -quiet-import-slt "%SLTDIR%" "%ATPDIR%\salamand.atp"
if not "%ERRORLEVEL%"=="1" goto :import_failed

:: 4. gate on layout (SC-005)
"%TRANSLATOR%" -quiet-validate-layout "%ATPDIR%\salamand.atp"
if not "%ERRORLEVEL%"=="1" goto :layout_failed
```

---

## Contract checklist for the build script

- [ ] Success is `ERRORLEVEL == 1`, not `0`
- [ ] Every invocation is timeout-guarded and kills on timeout
- [ ] Target `.slg` is copied from `english.slg` before import
- [ ] `.atp` is named `<module>.atp` and carries an absolute `Include=`
- [ ] Argument count is exactly 2 or 3 as per the table — no stray arguments
- [ ] `.bak` files are cleaned from the output tree
- [ ] Failure messages name the (language, module) pair
