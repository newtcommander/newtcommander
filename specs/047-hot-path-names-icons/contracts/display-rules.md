# Contract: Hot Path Label & Icon Display Rules

**Feature**: 047-hot-path-names-icons

## Resolution rule (single source of truth)

For an assigned slot (non-empty path):

- **Label** = custom name, if non-empty; otherwise the user-visible path text
  (stored path with `$$` unescaped to `$`). Implemented once in
  `CHotPathItems::GetDisplayName()`; every surface below MUST use it (SC-003).
- **Icon** = gallery icon `HHotPathIcons[IconIndex]`; index 0 is the current
  default bookmark icon (`HFavoritIcon`).

## Per-surface contract

| Surface | Membership | Label | Icon | Extras |
|---------|-----------|-------|------|--------|
| Hot Path Bar (`CHotPathsBar::CreateButtons`) | Path non-empty (was: Name non-empty) | `GetDisplayName`, truncated to 200 chars, ampersands doubled | per-item gallery icon | Tooltip = full path (unchanged, FR-003) |
| Change Drive menu, Alt+F1/F2 (`CDrivesList::BuildData`) | `Visible` && Path non-empty (Name requirement dropped) | `"<digit>\t" + GetDisplayName` for slots 1–10 (accelerator digits 1..9,0 preserved), `"\t" + GetDisplayName` for 11–30 | per-item `drv.HIcon` from gallery (was: shared `HFavoritIcon`) | `drvtHotPath` execution unchanged |
| Hot paths menu (`FillHotPathsMenu`: Go menu, top-toolbar drop, Left/Right panel menus) | Path non-empty | `GetDisplayName` + `Ctrl+<n>` / `Ctrl+Shift+<n>` accelerator column for slots 1–10 | per-item gallery icon for assigned slots; NULL for the assign-mode "empty" pseudo-items | "Customize…" trailing item unchanged |
| Directory-line "Assign Hot Path" submenu | as FillHotPathsMenu (assign mode) | same | same | |
| Taskbar jump list (`jumplist.cpp`) | `Visible` && Path non-empty | `GetDisplayName` as title; target = expanded path | (system-rendered; no gallery icon — out of icon scope) | |
| Settings list (`CCfgPageHotPath`) | all 30 rows always shown | `GetDisplayName` for assigned rows; empty label for empty rows | per-row gallery icon via `LVSIL_SMALL` imagelist | Hotkey column display-only, unchanged; checkbox = `Visible` |

## Behavioral rules

1. Changes confirmed in settings propagate immediately (bar rebuild + menus are
   built on open) — no restart (SC-001/SC-002).
2. Label text is display-only: `&` renders literally (existing
   `DuplicateAmpersands` on the bar; menu code already renders raw text).
3. Duplicate labels across slots are allowed; activation is by slot index
   (`Param`/command id), never by label.
4. Empty name after trim ⇒ fallback to path (FR-005).
5. Name with empty path is rejected at page validation (FR-004); no surface
   ever needs to render that state.
6. Quick-assign produces label = path (no name) + default icon (FR-011).
7. On DPI or system color change, gallery `HICON`s are reloaded and the Hot
   Path Bar rebuilt (existing `HFavoritIcon` lifecycle, FR-014); the Change
   Drive menu and popup menus are rebuilt on every open by construction.
