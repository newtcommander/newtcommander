# Validation Results — Dark Mode Stabilization (049)

**Date**: 2026-08-02 (updated after the full autonomous GUI verification pass) ·
**Build**: Debug x64, `build.cmd full`, BUILD SUCCEEDED (0 errors; only pre-existing warnings) ·
**Branch**: `049-dark-mode-stabilization`

> **Status: the walkthrough previously listed as "manual remainder" was completed autonomously**
> on 2026-08-02 by a WM_COMMAND-driven GUI harness (dialogs opened by command ID, screenshots
> verified visually). One residual defect was found and fixed during the pass (hotkey-field
> light `WS_EX_CLIENTEDGE` border → dark `WM_NCPAINT` in the grayscale-remap subclass).
> Results below; details in §"Autonomous GUI walkthrough".

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

## Autonomous GUI walkthrough (completed 2026-08-02; screenshots in session scratchpad `walk/`)

Dialogs were opened deterministically via `WM_COMMAND` with source-verified command IDs
(language-independent), child dialogs via `BM_CLICK` on control IDs, and every capture was
visually reviewed. Registry `Theme Mode` restored after every run.

| Surface (Dark) | Verdict | Evidence |
|---|---|---|
| Change Attributes — group boxes, checkboxes, **6× date/time pickers** (grayscale remap) | ✅ dark, segment cues preserved | e01 |
| Find → Advanced Options — group boxes, **radio glyphs**, 3-state checkboxes, disabled edits/combos, DTPs | ✅ | e03 |
| Drive Info — multiline read-only/disabled texts, gauge outline, legend, volume field | ✅ | d02 |
| Plugins Manager — section headers (D2), hyperlink (D1), checkbox listview (E5) | ✅ | d03 |
| Plugin Keyboard Shortcuts — **hotkey field** (E2) | ✅ after in-pass fix (light client edge → dark `WM_NCPAINT`, see below) | e06 → i01 |
| 7-Zip Configuration (winliblt plugin dialog) — group boxes, disabled checkboxes, combos | ✅ | e07 |
| Find window + Find Duplicate Files options dialog; duplicates search over `src` (123 items) | ✅ dark incl. results/status bar; the progress child finished faster than the capture interval — bar itself verified by the central 044-recipe branch + Find's own helper | e02, h03–h05 |
| Configuration sheet — full tree sweep (General, Panels, History, Recycle Bin, Language, Main Window, Appearance, **Colors** (swatches, 3-state checkboxes), Keyboard, Confirmations, Change Drive, Drives, **Views** (checkbox LVs, headers, disabled edits), **User Menu** (CEditListBox + arrow button), **Hot Paths** (checkbox LV), Security (group box), Icon Overlays, Viewers/Editors, Archivers) | ✅ all pages dark | f00–f30, h01–h02 |
| **High Contrast toggle** (SPI_SETHIGHCONTRAST + SPIF_SENDCHANGE, on→off) — closes the 044 open check | ✅ HC wins immediately; after HC off **every open window re-adopted Dark** (main, modeless Find, open modal confirm box) | h06, h07 |
| Direct `WM_THEMECHANGED` + `WM_SYSCOLORCHANGE` to the main window | ✅ stays dark, no crash | g01, g02 |
| Panel view-switch sequence Alt+3/4/5/2 + quick rename | ✅ (first pass) | dark-01…05 |
| About dialog link on navy | ✅ light blue, readable | dark-12 |

**Light-theme regression (SC-006)** — same surfaces with Default theme: main window after view
switches, Change Attributes (native DTPs), Plugins Manager, Plugin Keyboard (native hotkey
field), Configuration + Colors page — **all fully native light, zero dark residue** (l01–l06).
The new subclasses' light-mode passthrough is confirmed on exactly the dialogs they hook.

**Defects found & fixed during the pass**:
1. The hotkey control's `WS_EX_CLIENTEDGE` border is non-client and stayed light after the
   client-area grayscale remap. Fixed by handling `WM_NCPAINT` in the remap subclass
   (`ThemeDrawEdge(EDGE_SUNKEN)`, the 044 Find `WM_NCPAINT` precedent); recaptured (i01).
2. **Listview header labels black-on-dark app-wide** (user-reported on the Hot Paths page:
   "Name"/"Hot Key" unreadable): the `DarkMode_ItemsView` header paints a dark background but
   keeps black label text without the undocumented dark-mode APIs — the exact class 044
   documented and fixed for Find only. Now fixed centrally: new subclass ID 9
   (`ThemeListViewHeaderSubclassProc`) installed on every listview by the engine sweep answers
   the header's `NM_CUSTOMDRAW` with the 044 light-text recipe — covers Hot Paths, Views,
   Plugins Manager, Plugin Keyboard, Archivers auto-config and every plugin listview with no
   per-site code. Verified dark (j-dark-hotpaths/views/pluginmgr) and light-regression clean
   (j-light-*); saltests re-passed (1135/0). Subclass ID registry extended: **9 = listview
   header labels (049)**.

**Not exercised (accepted)**: FTP/PictView/File Comparator config sheets (reachable only through
plugin-specific menus; the mechanism — winliblt central `ThemeSubclassPropSheetFrame` call — is
shared with every winliblt sheet and compiled into all three), mdview Ctrl+F (needs a WebView2
viewer session), peviewer config dialog (same one-line `SetupWinLibTheme` mechanism as the six
plugins opened live in 036). These ride on mechanisms verified elsewhere in this pass.

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
