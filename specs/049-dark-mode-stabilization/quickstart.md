# Quickstart — Dark Mode Stabilization (049)

## Build & automated tests

```powershell
# From repo root, ALWAYS through PowerShell (cmd /c from Git Bash mangles /c — 044 gotcha)
.\build.cmd full            # Debug x64 + runtime data + plugins.ver
# saltests (unit suite incl. palette/contrast assertions):
& "$env:OPENSAL_BUILD_DIR\salamander\Debug_x64\saltests.exe"   # or .\build\... when OPENSAL_BUILD_DIR unset
```

Expected: build clean; saltests all-pass including the two new 049 assertions
(field-lighter-than-face, HOTLIGHT-vs-About-navy).

## Manual Dark-theme walkthrough (validation for SC-001…SC-006)

Enable: Options → Theme → Dark. Then:

1. **Panels (US1 / SC-001)** — in both panels cycle Alt+1…Alt+0, use menu View modes, toolbar
   drop-down, Alt+wheel; toggle header line (menu View); enter a ZIP and (if configured) an
   FTP path; F2-rename a file. Scroll bars, bottom bar, header, rename box stay dark
   throughout.
2. **Links (US2 / SC-002)** — Help → About (link readable on navy); Plugins Manager; Config →
   Keyboard, → Regional; Language Selection. All links light blue (102,178,255), not navy blue.
3. **Dialogs (US3 / SC-003)** — walk: Change Attributes (date/time fields dark), Find →
   Advanced (date/time), Drive Info (fields, gauge outline, multiline drive-type text),
   Alt+F10 size results, Language Selection (group boxes, radios, multiline author), Load/Save
   Selection, Master Password, Change Icon (dark list), Config → every page (group frames,
   radios, headers above lists, Colors-page swatches with nothing selected, User Menu/Hot
   Paths inline edit via F2-in-list), Archivers auto-config (status bar + checkbox list),
   Plugin Keyboard Shortcuts (hotkey field). No bright frames/glyphs, no black-hole fields —
   input fields now slightly LIGHTER than the dialog face.
4. **Indicators (US4)** — checksum verify + pictview batch (progress bars dark); Config →
   Confirmations & View-modes & Hot Paths & Icon Overlays (dark checkbox glyphs in lists);
   hover panel splitter + viewer for tooltips; Customize Toolbar drag (visible insert mark);
   launch a user-menu command (executing overlay readable). Find duplicates over a large
   folder — progress bar dark (closes the 044 open check).
5. **Plugins (US5 / SC-004)** — PE Viewer config; FTP + PictView + File Comparator config
   sheets (frame dark around pages); mdview Ctrl+F; Disk Map About; enter a bad number in a
   plugin dialog numeric field (themed validation box).
6. **Robustness (US6 / SC-005)** — toggle Windows High Contrast on/off (HC wins immediately —
   closes the 044 open check); change a Windows accent/color while app runs; verify Config →
   Views lists and all open windows stay dark afterward.
7. **Light regression (SC-006)** — switch back to Default theme, re-walk items 1–5 quickly:
   everything renders exactly as before the feature (no dark residue: light scrollbars native,
   links pure blue, native checkbox glyphs, native progress bars, light propsheet frames).

## Known accepted residuals (do not file as defects)

- DTP drop-down month calendar popup stays light (OS-drawn, 028 boundary class).
- Native Win32 menus everywhere (deferred to the follow-up menu feature).
- OS common dialogs, shell menus, splash, installer/salmon/SFX (028 boundaries).
- winliblt validation message box falls back to the system box when a plugin predates the
  hook (F5 residual, if recorded during implementation).
