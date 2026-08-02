# Contract: Plugin Theme API v106 (049)

Delta on `specs/036-plugin-dark-theme/contracts/plugin-theme-api.md`. All 036 rules stay:
engine-owned brushes, fetch-at-draw-time, reopen-adopts, no forced dark for non-adopters.

## 1. ABI append

`src/plugins/shared/spl_gen.h` — appended at the **end** of `CSalamanderGeneralAbstract`,
immediately after `ThemeHandleCtlColor` (the last 036 method):

```c
// dark-themes a property-sheet FRAME window (the comctl-owned dialog that
// hosts the pages): subclasses it so WM_CTLCOLOR*/WM_ERASEBKGND use the dark
// palette (tab strip area, buttons row, background). Call once, from the
// first page's WM_INITDIALOG, with GetParent(page hwnd). Idempotent; no-op
// in the Default theme. Pages themselves still use ThemeApplyToDialog.
// omezeni: thread vlastnici okno 'hwnd'
virtual void WINAPI ThemeSubclassPropSheetFrame(HWND hwnd) = 0;
```

Implementation: one-line delegation to `::ThemeSubclassPropSheetFrame` in `CSalamanderGeneral`
(`src/zip.cpp`, beside the six 036 delegations).

## 2. Version bump

`src/plugins/shared/spl_vers.h`:
- history row: `106 - 0.1.0 build 184: dark mode stabilization (feature 049):
  ThemeSubclassPropSheetFrame appended at the end of CSalamanderGeneralAbstract;
  pure vtable append — plugins built for 104/105 keep loading and running unchanged.`
- `#define LAST_VERSION_OF_SALAMANDER 106`

## 3. winliblt central adoption (no per-plugin edits)

`src/plugins/shared/winliblt.cpp` `CPropSheetPage::CPropSheetPageProc`, in the existing
`WM_INITDIALOG` theme block (after `ThemeApplyToDialog`):

```c
WinLibThemeServices->ThemeSubclassPropSheetFrame(::GetParent(hwndDlg));
```

Guarded by the same `WinLibThemeServices != NULL` check as the 036 calls. This closes the
light frames of ftp, pictview, and filecomp configuration sheets automatically; any plugin
using winliblt property sheets inherits the fix.

## 4. Compatibility guarantees

- Pure vtable append → plugins built against SDK 104/105 keep loading and running (constitution
  II; 036/104 precedent).
- Plugins rebuilt against SDK 106 refuse to load on cores < 106 (standard version gate).
- Third-party plugins that never call the theme API stay light by design (036 FR-006 upheld).
- No other interface, struct, or message contract changes.
