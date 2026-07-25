# Contract: Plugin Theme API (036)

Six methods appended to `CSalamanderGeneralAbstract`
(`src/plugins/shared/spl_gen.h`), available from plugin interface
version **105**. All are safe to call from the plugin's UI thread(s); all
are strict no-ops/passthroughs while the Default theme (or Windows High
Contrast) is active.

## Methods

### `BOOL IsDarkThemeActive()`
TRUE iff the user selected the Dark theme AND High Contrast is off.
Plugins branch custom drawing on this predicate (same contract as the
core-internal function of the same name).

### `COLORREF GetThemeSysColor(int index)`
`GetSysColor` replacement for DRAWING code: Default → exact
`GetSysColor(index)`; Dark → the 028 dark chrome palette (unmapped
indices fall back to `GetSysColor`). Never store across a theme switch.

### `HBRUSH GetThemeSysColorBrush(int index)`
`GetSysColorBrush` replacement for fills. Engine-owned brush — the plugin
MUST NOT delete it; valid until process exit (rebuilt internally on
switch, so fetch at draw time, do not cache).

### `void ThemeApplyToDialog(HWND hDialog)`
Call once from `WM_INITDIALOG`. Applies the dark DWM title bar, per-child
`SetWindowTheme` dark variants, and listview/treeview colors. Safe to call
in the Default theme (restores/no-ops).

### `void ThemeApplyToTopLevel(HWND hWindow)`
Call after creating any plugin top-level window (viewer frames, modeless
tool windows). Applies/removes the dark title bar per the active theme.

### `BOOL ThemeHandleCtlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, INT_PTR* result)`
Forward `WM_CTLCOLORDLG/STATIC/EDIT/LISTBOX/BTN` here first. Returns TRUE
with `*result` = brush handle when the Dark theme handled it (dialog proc
returns `*result`); FALSE in the Default theme (fall through to existing
handling).

## Usage patterns (normative)

Raw dialog proc:
```cpp
case WM_INITDIALOG:
    SalamanderGeneral->ThemeApplyToDialog(hwnd);
    … // existing init
    break;
case WM_CTLCOLORDLG: case WM_CTLCOLORSTATIC: case WM_CTLCOLOREDIT:
case WM_CTLCOLORLISTBOX: case WM_CTLCOLORBTN:
{
    INT_PTR r;
    if (SalamanderGeneral->ThemeHandleCtlColor(uMsg, wParam, lParam, &r))
        return r;
    break;
}
```

winliblt-based plugin — one line in `SalamanderPluginEntry` (after
`SalamanderGeneral` is obtained):
```cpp
SetupWinLibTheme(SalamanderGeneral); // themes every winliblt CDialog/CPropSheetPage
```

## Versioning & compatibility rules

- Slots are appended at the vtable end, in the exact order above; existing
  slots are untouched → plugins built against ≤104 keep loading and
  running (they stay light in Dark mode — permitted by spec FR-006).
- `LAST_VERSION_OF_SALAMANDER` = 105; a plugin calling these methods MUST
  require ≥105 via the standard version handshake.
- Theme adoption is at window creation; the core does NOT push live
  repaints into plugin windows on switch (clarified 2026-07-25).
  `PLUGINEVENT_COLORSCHANGED` continues to fire as before for plugins that
  want to refresh caches sooner.
