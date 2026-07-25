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

1. On `WM_INITDIALOG` (after the object is attached and
   `NotifDlgJustCreated()` ran, before the object's `DialogProc` sees the
   message): call `provider->ThemeApplyToDialog(hwndDlg)`.
2. On every dispatched message: invoke the object's `DialogProc` FIRST;
   only when it returns FALSE (left the message unhandled) forward to
   `provider->ThemeHandleCtlColor(...)`, which colors
   `WM_CTLCOLORDLG/STATIC/EDIT/LISTBOX/BTN` in the Dark theme and ignores
   everything else.

Guarantees:

- Default theme: `ThemeHandleCtlColor` returns FALSE and
  `ThemeApplyToDialog` no-ops → zero behavioral delta.
- A dialog's own `DialogProc` keeps full precedence: any WM_CTLCOLOR* it
  answers itself (custom warning colors etc.) wins over the theme — the
  same precedence the core established in 028 (`CCommonDialog` themes only
  what derived dialogs leave unhandled).
- No process-wide effects: no `InitCommonControlsEx`, no manifests, no
  class re-registration (constitution VI).

## Adoption checklist per plugin

1. Call `SetupWinLibTheme(SalamanderGeneral)` in `SalamanderPluginEntry`
   (after obtaining the interfaces, before any UI can open).
2. Rebuild the plugin — every winliblt dialog/propsheet page is themed.
3. Audit the plugin's non-winliblt surfaces separately (raw dialogs,
   custom top-level windows) per contracts/plugin-theme-api.md.
