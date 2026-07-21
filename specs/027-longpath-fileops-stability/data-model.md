# Data Model: Long-Path & Unicode File-Operation Stability Revision (027)

No persistent data is introduced. The entities are verification/audit
artifacts and in-memory structures.

## 1. Buffer/route audit inventory (`audit/INVENTORY.md`)

One row per audited site (continues 014 `_BRIEF.md` methodology):

| Field | Meaning |
|---|---|
| `id` | `<pass-letter><seq>` (e.g. `G07`) |
| `file:line` | site location on branch 027 |
| `symbol` | buffer/gate/function |
| `size` | declared size (MAX_PATH, 2*MAX_PATH, PATH_MAX_PATH, other) |
| `verdict` | `CRASH` / `BOUNDED` / `COMPONENT` / `EXTERNAL` / `FIXED` |
| `resolution` | for CRASH: `widen` / `heap` / `eliminate` + commit; for others: rationale |

Invariants:
- Every CRASH row MUST have a resolution commit before the feature closes.
- EXTERNAL rows MUST appear in the external-limit list (§3).
- The static check script re-derives the row set and fails on any
  unclassified or unresolved CRASH site.

## 2. Transfer & operation matrices (verification data)

**Clipboard/drag transfer matrix** (US1): {copy, cut, drag} × {source:
ordinary, long-ASCII, long-Unicode} × {target: ordinary, long-ASCII,
long-Unicode, external program} — expected outcome per cell: byte-identical
files, exact names, process alive; external-program cells: valid data handed
over, consumer's limits documented.

**Operation matrix** (US2): {view F3, edit F4, open Enter, copy F5, move F6,
rename F2, delete Del, create F7, attributes, properties, clipboard C/X/V,
drag, calc-size, change-drive Alt+F1} × {ordinary, long-ASCII ~291,
long-Unicode ~570 B} — expected: success or clear bounded message, never
process death.

**F5/F6 matrix** (US3): {file, recursive folder} × {long→normal,
normal→long, long→long} × {no collision, overwrite, skip, cancel midway} —
expected: exact result, intact source on skip/cancel.

**Timing matrix** (US4): fixed file set (500×4 KB + 5×50 MB), ordinary→
ordinary vs long→long, same volume; long ≤ 1.10 × ordinary.

Test data root: `%LOCALAPPDATA%\Temp\salamander-test\010\long-paths\`
(existing: `L1-…ěščřžýáíé…` Unicode tree ≥427 chars deep, `ascii-level-…`,
`edge-260`, `copy-target-dir`; extended with boundary cases as needed).

## 3. External-limit list (single source of truth: `audit/INVENTORY.md` §External)

Carried forward: ShellExecute(Ex) command line; IShellLink/.lnk + Paste
Shortcut; SHFileOperation Recycle Bin; shell "New" menu; launched-process
cwd inheritance; salextx64 IPC `TargetPath` (ABI-frozen, explicit refusal).
Added by 027: Explorer-side paste consumption of long-path clipboard data;
EncryptFile/DecryptFile if W-API rejects `\\?\` (verified during
implementation).

## 4. Key in-memory structures touched (existing, no layout changes except widening)

| Structure | Change |
|---|---|
| `CDrivesList` (`drivelst.h:110`) | `CurrentPath[MAX_PATH]` → `[SAL_MAX_PATH_UTF8]` (member order preserved) |
| `CExecuteExpData` (`execute.cpp:580`) | `Buffer[MAX_PATH]` → `[SAL_MAX_PATH_UTF8]` |
| own IDataObject (new, `shellib.cpp`) | wide DROPFILES CF_HDROP + CFSTR_PREFERREDDROPEFFECT (see contracts/clipboard-dataobject.md) |
| `SalShExtSharedMem` (`shexreg.h:218`) | **frozen** — refusal gates added at write sites only |
