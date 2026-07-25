# Contract: winliblt Central Theming (036)

`src/plugins/shared/winliblt.{h,cpp}` — the shared plugin WinLib compiled
into each plugin DLL that uses it.

## New API

```cpp
// winliblt.h
void SetupWinLibTheme(CSalamanderGeneralAbstract* salamander);
```

Stores the provider in a module-global (one per plugin DLL, same lifetime
model as `SetupWinLibHelp`). Passing NULL (or never calling) disables all
theming — winliblt behaves byte-for-byte as before feature 036.

## Central-proc behavior (provider set)

`CDialog::CDialogProc` and `CPropSheetPage::CPropSheetPageProc`:

1. On `WM_INITDIALOG` (after the object is attached, before the object's
   `DialogProc` runs): call `provider->ThemeApplyToDialog(hwndDlg)`.
2. On `WM_CTLCOLORDLG/STATIC/EDIT/LISTBOX/BTN`: call
   `provider->ThemeHandleCtlColor(...)` first; if TRUE, return its result
   without invoking the object's `DialogProc` for that message; if FALSE,
   proceed exactly as today.

Guarantees:

- Default theme: `ThemeHandleCtlColor` returns FALSE and
  `ThemeApplyToDialog` no-ops → zero behavioral delta.
- A dialog's own `DialogProc` can still fully customize drawing: object
  handlers run for every message except a Dark-handled WM_CTLCOLOR*, which
  is the same precedence 028 uses in the core winlib.
- No process-wide effects: no `InitCommonControlsEx`, no manifests, no
  class re-registration (constitution VI).

## Adoption checklist per plugin

1. Call `SetupWinLibTheme(SalamanderGeneral)` in `SalamanderPluginEntry`
   (after obtaining the interfaces, before any UI can open).
2. Rebuild the plugin — every winliblt dialog/propsheet page is themed.
3. Audit the plugin's non-winliblt surfaces separately (raw dialogs,
   custom top-level windows) per contracts/plugin-theme-api.md.
