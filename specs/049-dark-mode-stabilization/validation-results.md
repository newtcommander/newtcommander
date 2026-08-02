# Validation Results — Dark Mode Stabilization (049)

**Date**: 2026-08-02 · **Build**: Debug x64, `build.cmd full`, BUILD SUCCEEDED (0 errors; only
pre-existing warnings) · **Branch**: `049-dark-mode-stabilization`

## Automated results

| Check | Result |
|---|---|
| Full solution build (`build.cmd full`, core + 18 plugins + language modules) | ✅ SUCCEEDED, 0 errors |
| saltests | ✅ **1135 checks, 0 failed** (1133 pre-existing + 2 new 049 assertions) |
| New assertion: `Lum(COLOR_WINDOW) > Lum(COLOR_BTNFACE)` (field lighter than face) | ✅ |
| New assertion: `Contrast(COLOR_HOTLIGHT, About navy #0A1424) ≥ 4.5` | ✅ (≈ 8:1) |
| All pre-existing contrast floors with `COLOR_WINDOW` = 56,56,56 | ✅ (WINDOWTEXT 10.2:1, HOTLIGHT 5.2:1, GRAYTEXT 3.9:1) |
| clang-format on all 30 touched files (VS2022-bundled LLVM) | ✅ conformant after formatting pass; UTF-8-BOM + CRLF preserved |

## Scripted GUI verification (Dark theme, Debug x64, screenshots in session scratchpad)

| Scenario | Result |
|---|---|
| **SC-001 / US1 — panel view switches**: automated Alt+3 → Alt+4 → Alt+5 → Alt+2 → Alt+3 sequence in the active panel | ✅ scroll bars, bottom bar and panel chrome stay dark after every switch (`dark-01…05-*.png`); the reported re-lightening no longer reproduces |
| Quick-rename box visible in panel | ✅ renders as a dark field (no white box) |
| **SC-002 / US2 — About dialog link**: Help → About opened in Dark | ✅ `tandemcommander.org` renders light blue (102,178,255) on the navy background, clearly readable (`dark-12-about-dialog.png`); previously near-invisible dark blue |
| Owner-drawn menu bar + Help menu popup in Dark | ✅ dark (`dark-06-help-menu.png`) |
| App startup/shutdown stability with all new subclasses active | ✅ no crash, no assert across 3 scripted runs |
| Registry hygiene | ✅ `Theme Mode` restored to the pre-test value (0) after each run |

## Verified during implementation (code-level)

- **A-cluster**: every view-switch entry point funnels through `CFilesBox::SetMode` →
  single `ThemeApplyToWindowTree` call covers Alt+digit, menu/toolbar commands, Alt+wheel,
  smart-column toggle, config apply, plugin-forced view modes, header toggle.
- **B4 (rebar background)**: audit claim disproven — `RB_SETBKCOLOR` already present
  (`salamdr1.cpp` ColorsChanged path). No change made.
- **D5 (`filesmap.cpp` rubber band)**: white/black XOR pair verified visible in both themes;
  changing it would alter light mode. No change made.
- **F4 (diskmap About)**: audit claim disproven — the raw `DialogBox` is compiled only in the
  standalone (non-SALAMANDER) build; the shipped plugin routes About through the themed
  `SalamanderGeneral->SalMessageBox` (`DiskMapPlugin.cpp:325`). No change made.
- **ABI 106**: pure vtable append after `ThemeHandleCtlColor`; structural backward compatibility
  follows the 036/104 precedent (older plugins never call the new slot). No pre-106 binary was
  available to load-test; the version-gate mechanics are unchanged from 104→105.
- **BS_PUSHLIKE radios**: excluded from the glyph overlay (render as buttons); none exist in
  `lang.rc` today (defensive guard).
- **WM_THEMECHANGED recursion**: `SetWindowTheme`-on-self loops guarded (viewer uses a
  thread-local re-entry flag; dialog handlers only re-theme children).

## Manual checks remaining for a human GUI session (quickstart steps 3–7)

The exhaustive 110-template dialog walkthrough and environment toggles need an interactive
desktop and were NOT fully exercised by the scripted pass:

1. Dialog-by-dialog Dark walkthrough (group boxes, radios, DTP/hotkey grayscale remap in Change
   Attributes + Find Advanced, multiline disabled edits in Drive Info / Language Selection,
   Change Icon list, Colors-page swatches, inline list editors, Archivers auto-config status
   bar + checkbox list). Mechanism-level verified; visual confirmation pending.
2. Plugin surfaces: peviewer config, FTP/PictView/File Comparator sheet frames, mdview Ctrl+F,
   plugin numeric-validation box.
3. Windows High Contrast toggle (FR-007/G1 — also the check 044 left open) and a system
   visual-style/color change with windows open (G2/G5).
4. Find duplicates over a large folder (progress bar — the second 044 open check).
5. Full light-theme regression sweep (SC-006). Scripted light run was aborted when a user
   window took the foreground mid-test; the partial capture showed the classic light UI intact.

## Accepted residuals (by design, do not file as defects)

- DTP drop-down month calendar popup stays light (OS-drawn themed popup; 028 boundary class).
- The date picker's active-segment text renders dark-on-lightblue during editing (the grayscale
  remap preserves the native selection pixels; transient, readable).
- Native Win32 menus everywhere — deferred to the follow-up menu feature per clarification.
- OS common dialogs, shell context menus, splash, installer/salmon/SFX stubs (028 boundaries).
- Startup failure-path error boxes (missing .slg, registry, allocation) deliberately keep the
  plain system message box (clarified 2026-08-02).

## Build/tooling notes

- Language modules (`.slg`) are produced only by `build.cmd full`; an incremental build after
  a clean output tree leaves the app unable to load its configured non-English language (this
  surfaced during testing as a startup error box — not a 049 defect).
- Builds invoked through PowerShell per the 044 gotcha.
