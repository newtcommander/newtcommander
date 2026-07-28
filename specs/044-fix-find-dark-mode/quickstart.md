# Quickstart: Fix Find Window Dark-Mode Rendering

**Feature**: 044-fix-find-dark-mode · verification guide for the
implementation and the final walkthrough (spec SC-001..SC-004).

## Build & test gates

```batch
set OPENSAL_BUILD_DIR=D:\Build\OpenSal\
build.cmd                 :: Debug x64 incremental — must succeed
build.cmd full release    :: Release x64 — must succeed
```

- Run the `saltests` project — all tests green, including the new
  contrast assertions (dark `COLOR_BTNTEXT`/`COLOR_BTNFACE` ≥ 4.5:1,
  dark `COLOR_GRAYTEXT`/`COLOR_BTNFACE` ≥ 3:1).
- `clang-format` clean on every touched file.

## GUI walkthrough — Dark theme (SC-001, SC-002)

Launch `newtcommander.exe`, switch **Options → Theme → Dark**, press
**Ctrl+F**. Reference for the "before" state: `temp/dark_find_window.png`.

Check each state; at no point may any white/light line, frame, field,
or bar appear:

1. **Initial window**
   - [ ] Separator under the menu bar renders as a subtle dark bevel
     (was bright white)
   - [ ] Separator beside "Search file content" — same
   - [ ] Line above the results list (bottom edge of the toolbar strip)
     — dark, no white scanline
   - [ ] "No Advanced Options" box: dark frame (was white), text
     readable gray (was near-invisible)
   - [ ] "Found Items: (0)" — light text (was black-on-dark)
   - [ ] Disabled toolbar caption ("Focus") — flat readable gray, no
     emboss shadow
   - [ ] Results header "Name"/"Path" — light text (was black)
   - [ ] Status bar — dark background, light hint text, dark size grip
2. **Content search expanded** — check "Search file content":
   - [ ] Revealed fields, checkboxes, labels all dark and readable;
     no new light artifacts
3. **Advanced options set** — click Advanced…, define a condition, OK:
   - [ ] Summary box shows the text readable (enabled state), dark frame
   - [ ] The Advanced dialog itself: its separators are dark too
     (central fix side effect — same defect class)
4. **Running search** — search a large tree (e.g. `C:\Windows`):
   - [ ] Status bar shows the searched path in light text on dark
   - [ ] Progress bar (if shown) renders dark track / accent bar
   - [ ] After completion: results summary text light on dark
5. **Results present**
   - [ ] Results list, Path column, selection colors — unchanged
     (no regression)
   - [ ] Selection-count status text light on dark

Multi-instance (FR-008): open a second Find window (Ctrl+F from the main
window again) — both render identically.

## Live theme switch (SC-004, FR-006)

With a Find window open (one pass with a search running):

- Switch Dark → Default: entire Find window, including separators,
  status bar, header, advanced box, reverts to the classic light look.
- Switch Default → Dark: everything turns dark again, including the
  transient progress bar if present.
- Repeat 10×: no crash, no drawing corruption, no half-themed element.

## Light-mode regression pass (SC-003, FR-005)

In **Default** theme (and once with Windows High Contrast enabled,
FR-007):

- Open the Find window; compare side-by-side with a pre-feature build
  (or the `main` branch binary): separators, advanced box, status bar,
  toolbar disabled text, header labels, progress bar during a search —
  zero visual differences.
- High Contrast: Find window follows system colors exactly as before.

## Side-effect surfaces (verify, not goals)

The central fixes also touch these — confirm they look right in dark and
unchanged in light:

- Any dialog with etched separator lines (e.g. Find Settings, Find
  Advanced, configuration pages)
- Any dialog with bordered/read-only edits (border now `DarkMode_CFD`)
- The pack/archive dialog's status bar (`src/packac.cpp`)
- Disabled items on main-window toolbars (flat gray instead of emboss
  in dark)

## Scripted verification note

For automated screenshots, see the project memory note "Driving the
Newt Commander GUI for verification": the process's `MainWindowHandle`
is the splash screen, not the main window — enumerate top-level windows
by class/title instead. The Find window runs on its own thread with its
own message pump; give it a moment after `Ctrl+F` before capturing.
