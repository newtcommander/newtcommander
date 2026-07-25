# Quickstart: Markdown Viewer Dark-Mode Polish

**Feature**: 037-mdview-dark-polish

## Build

```batch
set OPENSAL_BUILD_DIR=D:\Build\OpenSal\
build.cmd full
```

`mdview` must be enabled in `plugins.cfg` (it is by default). Run
`newtcommander.exe` from the build output.

## Verify US1 — no white flash (SC-001, SC-003)

1. In the Markdown viewer, pick a **dark** scheme: open any `.md` (F3) →
   View > Color Scheme > e.g. *Dark* (and turn **off** "Follow system" for a
   deterministic test). Close the viewer.
2. Start a screen recording (or use a slow-motion phone capture of the
   monitor).
3. Press F3 on a `.md` file. Repeat 10×.
4. Review frame by frame: **zero** frames may show a white/mismatched client
   area — the window interior must be the scheme background from its first
   visible frame until content appears.
5. Large-file case: open a multi-MB `.md` — the scheme background must hold
   for the whole (longer) load.
6. Scheme change: switch to a different scheme (F9), close, reopen — first
   frame uses the **new** scheme's background.
7. Follow-system mode: enable View > Color Scheme > Follow system, set app
   theme Dark (Options > Theme) — reopened viewer must first-paint with the
   dark scheme's background.

## Verify US2 — dark menus (SC-002)

1. Options > Theme > **Dark** (restart/reopen per app convention).
2. Open a `.md` in the viewer:
   - menu **bar** (File, Edit, View, Help) renders dark;
   - every **drop-down** (including View > Color Scheme submenu) renders
     dark: dark background, light text, engine highlight color on hover,
     readable disabled items, visible separators;
   - radio dot on the active scheme and checkmarks (Follow system, Allow
     remote images, View Source) are visible;
   - keyboard navigation (F10/Alt, arrows, accelerator underlines after Alt)
     behaves as before.
3. Caption system menu (Alt+Space): must look the **same as the main
   window's** Alt+Space menu (parity — see research R5).
4. Compare side by side with the main window's menus — colors must match the
   application dark palette.

## Verify regression — light mode (SC-004)

1. Options > Theme > **Default**.
2. Reopen the viewer: menu bar, drop-downs, and window background must be
   pixel-identical to the previous release (native light menus; no owner-draw
   artifacts).
3. Light scheme + Default theme: no flash of a *dark* background either —
   first paint uses the light scheme's `docBg`.

## Acceptance evidence

Follow the 036 pattern: capture screenshots (dark menus open, first-frame
captures) and record results in `validation-results.md` in this spec
directory, keyed SC-001..SC-004.
