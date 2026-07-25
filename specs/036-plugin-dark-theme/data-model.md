# Data Model: Dark Theme for Plugin Windows and Dialogs (036)

No persistent data changes. The "model" is the theme-state flow and the
per-plugin surface inventory the tasks execute against.

## Theme-state flow

```text
Configuration.ThemeMode ("Theme Mode" DWORD, HKCU…\Configuration, 028)
        │
        ▼
src/themes.cpp  (single owner: predicate + dark palette + apply helpers)
        │
        ├── core UI (028): winlib/sheets central procs, chrome sweep
        │
        └── NEW (036): CSalamanderGeneral (6 delegating methods)
                 │  spl_gen.h vtable slots appended; version 104 → 105
                 ▼
        plugin side
            ├── winliblt central procs  ← SetupWinLibTheme(sal) per plugin
            │       └── every CDialog / CPropSheetPage in that plugin
            ├── raw dialog procs        ← 2 touchpoints each
            └── top-level windows       ← ThemeApplyToTopLevel + chrome colors
```

Invariants:
- Default theme ⇒ every new call is a passthrough/no-op (zero visual delta).
- Theme read at window creation only; no plugin-side caching of palette
  entries across theme switches (brushes owned by the engine).
- Provider unset in winliblt ⇒ behavior identical to pre-036.

## Per-plugin surface inventory (enabled set, plugins.cfg)

| Plugin | Dialog templates | winliblt | Extra top-level surfaces | 036 work |
|--------|-----------------:|----------|--------------------------|----------|
| ftp | 55 | yes | welcome-msg/log windows | Setup call + audit |
| zip | 26 | no (raw procs) | — | touchpoints ×~17 procs |
| pictview | 17 | yes | viewer frame (image canvas untouched) | Setup call + frame chrome |
| regedt | 9 | yes | edit/view windows | Setup call + views dark |
| renamer | 9 | yes | preview list | Setup call + audit |
| sftp | 9 (raw) | linked, dialogs raw | modeless log window | touchpoints ×9 + log chrome |
| dbviewer | 6 | yes | table window (content dark) | Setup call + table colors |
| undelete | 6 | yes (library/) | — | Setup call + audit |
| filecomp | 5 | yes | compare panes (content dark) | Setup call + pane colors |
| 7zip | 4 | yes | — | Setup call + audit |
| uncab | 4 | no (raw procs) | — | touchpoints ×4 |
| checksum | 3 | yes | — | Setup call + audit |
| peviewer | 1 | yes | report window (text dark) | Setup call + window |
| uniso | 1 | yes | — | Setup call + audit |
| mdview | 0 | yes | viewer window (document dark) | Setup + title bar + content colors |
| diskmap | 0 | no | map window (custom-drawn) | title bar + chrome colors |
| folders | 0 | yes | menu-driven only | Setup call + audit |
| portables | 0 | yes | FS UI via core | Setup call + audit |
| tar | 0 | no | none (uses core UI) | audit only |

(Counts from the rc-template census 2026-07-25; the audit task re-verifies
each plugin's real surface list — templates in `.rc2`, message boxes via
`SalMessageBox` already themed by core, etc.)

## Interface delta (contract detail in contracts/plugin-theme-api.md)

| New vtable slot (appended, in order) | Delegates to |
|--------------------------------------|--------------|
| `IsDarkThemeActive()` | `::IsDarkThemeActive` (themes.cpp) |
| `GetThemeSysColor(int)` | `::ThemeSysColor` |
| `GetThemeSysColorBrush(int)` | `::ThemeSysColorBrush` |
| `ThemeApplyToDialog(HWND)` | `::ThemeApplyToDialog` |
| `ThemeApplyToTopLevel(HWND)` | `::ThemeApplyToTopLevel` |
| `ThemeHandleCtlColor(UINT,WPARAM,LPARAM,INT_PTR*)` | `::ThemeHandleCtlColor` |

Version: `LAST_VERSION_OF_SALAMANDER` 104 → 105 (`spl_vers.h` + history).
