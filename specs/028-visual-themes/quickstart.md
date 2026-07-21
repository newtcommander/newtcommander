# Quickstart: Visual Themes (Default + Dark)

**Feature**: 028-visual-themes

## Build & automated verification

```batch
build.cmd                 :: Debug x64 incremental — must compile clean
build.cmd full release    :: Release x64 — must compile clean
```

Unit tests (saltests — all pre-existing 427 must stay green, plus new
theme tests):

- dark chrome palette covers all drawn COLOR_* indices
- WCAG contrast ≥ 4.5:1 (text), ≥ 3:1 (disabled) for dark pairs
- `DarkColors`/`DarkViewerColors` contain no `SCF_DEFAULT` flags
- Default theme ⇒ `ThemeSysColor(i) == GetSysColor(i)` for all i
- `ThemeMode` defaults to 0; invalid registry values coerce to 0

Smoke: launch the built exe, verify clean start and exit.

## Visual walkthrough (user, GUI) — acceptance checklist

**A. Switch (US1, SC-001)** — Options → Theme → Dark:
main window fully dark in < 2 s without restart — panels (normal /
selected / focused / hidden items), both panel captions + directory
lines, info line, top toolbars + rebar, menu bar + every drop-down
menu (hover/pressed states), command line, F1–F12 bottom bar, window
title bar. Options → Theme shows Dark checked.

**B. Back to Default (SC-003)** — Options → Theme → Default: appearance
identical to pre-feature build (side-by-side), except the new Theme
submenu itself.

**C. Persistence (US2, SC-002)** — switch to Dark, exit, relaunch:
starts dark (no light flash of the main window). Delete
`HKCU\Software\Open Salamander\5.0\Configuration\Theme Mode`, relaunch:
starts in Default.

**D. Dialogs (US3, SC-004)** — with Dark active open: Configuration
(several pages incl. Colors), Find (+ its results list), F5 copy
progress dialog, an error message box, Alt+F7, password prompt, Alt+F12
drive info. All dark with readable controls. Internal viewer (F3): dark
text scheme.

**E. Imagery (US4)** — toolbar icons, menu glyphs, bottom-bar symbols
legible, no light halos; file icons legible on dark panel background.

**F. Coexistence** — while Dark: change color scheme in Configuration →
Colors → affects nothing visibly; switch to Default → chosen scheme
active, custom colors intact. High-contrast Windows mode: system colors
win regardless of stored theme.

**G. Stability (SC-006)** — 20 consecutive switches with viewer + Find
open in between: no crash, corruption, or panel-state loss; running
file operation continues across a switch (FR-014).
