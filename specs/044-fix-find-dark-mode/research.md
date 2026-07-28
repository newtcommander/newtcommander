# Phase 0 Research: Fix Find Window Dark-Mode Rendering

**Feature**: 044-fix-find-dark-mode · **Date**: 2026-07-28
**Sources**: code survey of `src/themes.cpp`, `src/finddlg1.cpp`,
`src/finddlg2.cpp`, `src/lang/lang.rc`, `src/common/winlib.cpp`,
`src/toolbar2.cpp`; pixel measurement of `temp/dark_find_window.png`
(868×521); feature 028 plan + theme-engine contract
(`specs/028-visual-themes/`).

## Root-cause inventory (evidence)

The Find window (`CFindDialog : CCommonDialog`, `src/find.h:679`, template
`IDD_FIND` at `src/lang/lang.rc:1304-1331`) is themed by the central 028
layer: `ThemeHandleCtlColor` via `CCommonDialog::DialogProc`
(`src/dialogs2.cpp:258`) and `ThemeApplyToDialog` via
`NotifDlgJustCreated` (`src/dialogs2.cpp:251`). The residual defects map
to six distinct root causes:

| # | Visible defect | Control | Root cause |
|---|---|---|---|
| 1 | White separator lines (measured 160/160/160 + 255/255/255) | `IDC_FIND_LINE1/2` (`lang.rc:1308,1318`), `SS_ETCHEDHORZ` statics | Native static paints etched lines with real `GetSysColor(COLOR_3DSHADOW/3DHILIGHT)`; the 028 static subclass explicitly bails out for non-text statics (`themes.cpp:277-281`). **No dialog in the app has dark separators today.** |
| 2 | White line above the results list (measured 255/255/255 at y=229) | `CFindTBHeader` self-applied `WS_EX_STATICEDGE` (`finddlg2.cpp:339-341`) | Non-client 3D edge drawn by `DefWindowProc` with real system colors; no `WM_NCPAINT` override. |
| 3 | Bright white frame on advanced-options box | `IDC_FIND_ADVANCED_TEXT` (`lang.rc:1328`, read-only `EDITTEXT`) | `ThemeApplyChildEnumProc` gives Edits `DarkMode_Explorer` (`themes.cpp:352`), which has no dark Edit border part → light border. The repo's own precedent for edits is `DarkMode_CFD` (`editwnd.cpp:1709-1710`, combos at `themes.cpp:366`). Interior is already dark (45/45/45) — only the frame is wrong. |
| 4 | Dark-on-dark text | (a) results-list column headers "Name"/"Path": glyphs 0/0/0 on 25/25/25 — `DarkMode_ItemsView` darkens the header background but text color stays black because the app (deliberately) never calls undocumented `SetPreferredAppMode`; (b) "Found Items: (0)": `DrawText` with no `SetTextColor` (`finddlg2.cpp:568`) → default black on 45/45/45; (c) "No Advanced Options": edit is *disabled* when empty (`finddlg1.cpp:1739`) and a disabled themed edit paints text in the light theme's gray 109/109/109 on 45/45/45 ≈ 2.7:1 — fails the 3:1 disabled-text bar; (d) disabled toolbar caption "Focus": classic two-pass emboss `COLOR_BTNHILIGHT`/`COLOR_BTNSHADOW` (`toolbar2.cpp:648,659`) = 70/70/70 on 45/45/45 ≈ 1.9:1. |
| 5 | Fully light status bar (measured 240/240/240 strip) | `HStatusBar`, raw `msctls_statusbar32` created in `WM_INITDIALOG` (`finddlg1.cpp:2930-2938`) | Two independent causes: (a) `ThemeApplyToDialog` runs from `NotifDlgJustCreated` **before** `CFindDialog`'s `WM_INITDIALOG` body (`winlib.cpp:726` vs `:767`), so the status bar doesn't exist yet during the pass; (b) even when re-applied later (`OnColorsChange`, `finddlg2.cpp:310`), `ThemeApplyChildEnumProc` has **no status-bar branch** (`themes.cpp:390-398`) and comctl32 has no dark status-bar theme class at all. The existing owner-draw of part 1 (`finddlg1.cpp:3823-3843`) sets no colors either. |
| 6 | (Search-state) progress bar in status-bar part 2 | `PROGRESS_CLASS` child created in `SetTwoStatusParts` (`finddlg1.cpp:1471`) | Themed progress bar renders on a light track; never dark-adjusted. Visible only while a search runs (spec SC-001 walkthrough includes this state). |

Already correct (do not touch, no-regression): menu bar and popups
(owner-drawn, `menubar.cpp:156-206`), toolbar enabled glyphs, results
Path-column custom draw (`finddlg1.cpp:4042-4118`), combo boxes
(`DarkMode_CFD`), buttons, dialog background.

## Decisions

### R1 — Etched separator lines: fix centrally in the theme engine

**Decision**: Extend the `Static` branch of `ThemeApplyChildEnumProc`
(`themes.cpp:354-362`): for `SS_ETCHEDHORZ`/`SS_ETCHEDVERT`/
`SS_ETCHEDFRAME` statics install a dedicated etched-line subclass that,
when `IsDarkThemeActive()`, paints the line via the existing
`ThemeDrawEdge` idiom (dark bevel pair `COLOR_3DDKSHADOW`/`COLOR_3DLIGHT`,
`themes.cpp:138-139`); otherwise `DefSubclassProc` (native light paint).

**Rationale**: One code path fixes `IDC_FIND_LINE1/2` plus every other
etched separator (~200 in `lang.rc`, incl. the Find Settings and Advanced
dialogs opened from the Find window) with zero per-dialog edits — the
exact philosophy of 028's central dialog layer, and the sanctioned
"application-wide deliberate decision" shape required by constitution
principle VI. Painting keyed on `IsDarkThemeActive()` per message makes
live theme switches and High Contrast automatic.

**Alternatives considered**: (a) owner-draw the two lines inside
`CFindDialog` only — rejected: repeats per dialog, leaves the identical
defect visible in the Find Settings/Advanced dialogs named in the spec
walkthrough; (b) recolor via `WM_CTLCOLORSTATIC` — impossible, the etched
edge ignores the CTLCOLOR brush (it only fills surroundings, confirmed by
measurement: surroundings are already dark).

### R2 — Header strip white edge: `WM_NCPAINT` override in `CFindTBHeader`

**Decision**: Handle `WM_NCPAINT` in `CFindTBHeader`
(`src/finddlg2.cpp`): when dark, draw the `WS_EX_STATICEDGE` frame with
`ThemeDrawEdge`; otherwise default. Precedent: the panel non-client frame
does exactly this (`src/filesbx1.cpp:1342-1370`).

**Rationale**: Keeps the control's metrics identical (spec: no layout
changes); reuses the one existing NC-edge precedent.

**Alternatives**: removing `WS_EX_STATICEDGE` in dark — rejected, changes
client metrics and layout math between themes.

### R3 — Edit border: switch central Edit theming to `DarkMode_CFD`

**Decision**: In `ThemeApplyChildEnumProc`, theme `Edit` controls with
`DarkMode_CFD` instead of `DarkMode_Explorer` (`themes.cpp:352`).

**Rationale**: `DarkMode_CFD` is the repo's proven choice for edit-like
controls — combo boxes already use it (`themes.cpp:363-367`) and the
command line was explicitly switched to it because "its classic light
border would otherwise shine through" (`editwnd.cpp:1709-1710`). This
fixes the white frame of `IDC_FIND_ADVANCED_TEXT` and the same latent
defect on every bordered edit in every dialog.

**Alternatives**: per-control NC paint of the border — rejected, more
code for a worse result; keeping Explorer class and stripping the theme
(`L""`) — rejected, classic border uses real syscolors (still light).

### R4 — Disabled-edit text: central flat-repaint subclass (mirror of the 028 static fix)

**Decision**: Install a paint-override subclass on `Edit` children (same
installation point as R3) that acts only when `IsDarkThemeActive()` AND
the edit is disabled: fill `COLOR_BTNFACE`, draw the text with dark
`COLOR_GRAYTEXT` (150/150/150 → 4.6:1 on 45/45/45, passes the 3:1
disabled bar). Enabled edits and light mode fall through to default
painting. Modeled directly on `ThemeFlatDisabledTextSubclassProc`
(`themes.cpp:266-326`), which solved the identical problem for disabled
statics in 028.

**Rationale**: A disabled themed edit ignores `WM_CTLCOLOR*` text color
entirely, so no message-level fix exists; the repaint subclass is the
established in-repo pattern for exactly this rendering class.

**Alternatives**: keep `IDC_FIND_ADVANCED_TEXT` always enabled
(read-only) so CTLCOLOR applies — rejected: changes focus/tab behavior
(`EnableWindow(..., dirty)` at `finddlg1.cpp:1739` is intentional) and
fixes only one control instead of the control class.

### R5 — Status bar: central dark subclass for `msctls_statusbar32`

**Decision**: Add a `STATUSCLASSNAME` branch to
`ThemeApplyChildEnumProc` installing a status-bar subclass. When
`IsDarkThemeActive()`: `WM_ERASEBKGND` fills `COLOR_BTNFACE`; `WM_PAINT`
custom-paints all parts — geometry via `SB_GETPARTS`/`SB_GETRECT`, text
via `SB_GETTEXT` drawn with `COLOR_BTNTEXT`, owner-draw parts forwarded
to the parent as `WM_DRAWITEM` (native behavior preserved), size grip
drawn with the dark bevel colors. Light mode: pure `DefSubclassProc`
(pixel-identical passthrough). Complementary Find-side fixes:
`CFindDialog`'s existing `WM_DRAWITEM` status handler
(`finddlg1.cpp:3823-3843`) gains
`SetTextColor(ThemeSysColor(COLOR_BTNTEXT))` (passthrough-safe in light,
where `ThemeSysColor ≡ GetSysColor`).

**Rationale**: comctl32 offers no dark status bar (`SB_SETBKCOLOR` is
ignored under visual styles; no `DarkMode_*` class exists), so painting
is the only route. Doing it centrally follows 028's layer design and
also silently heals the app's only other raw status bar
(`src/packac.cpp:75`). The dark-only paint keyed per-message keeps the
Default theme bit-for-bit native and makes theme switches/High Contrast
work with no extra plumbing.

**Alternatives**: (a) replace the control with the self-drawn
`CStatusWindow` used by the main window (`src/stswnd.cpp`) — rejected:
functional surgery on working code (3 parts, embedded progress child,
size grip) for a purely visual defect, contrary to principle III;
(b) `SetWindowTheme(L"", L"")` + `SB_SETBKCOLOR` — rejected: classic
paint honors the background but still draws text/grip/borders with real
system colors (text stays black, bevels stay light).

### R6 — Status bar creation ordering: re-apply after `WM_INITDIALOG`

**Decision**: At the end of `CFindDialog`'s `WM_INITDIALOG` handling,
call `ThemeApplyToDialog(HWindow)` once more.

**Rationale**: `NotifDlgJustCreated` (where the first pass runs,
`winlib.cpp:726`) fires **before** the dialog's own `WM_INITDIALOG` body
creates the status bar, menu bar, and header toolbars — so the initial
pass can never see them. `ThemeApplyToDialog` is idempotent by design
(`THEME_DARKENED_PROP` sentinel, `themes.cpp:403-418`), making a second
pass safe and cheap; it also future-proofs any later-created children.
`OnColorsChange` (`finddlg2.cpp:310`) already re-applies on live theme
switches, so no change is needed there.

**Alternatives**: theming each child at its creation site — rejected:
four call sites instead of one, easy to miss the next one.

### R7 — Results-list header text: custom-draw override in `CFoundFilesListView`

**Decision**: Handle the header's `NM_CUSTOMDRAW` in
`CFoundFilesListView::WindowProc` (`finddlg1.cpp:860`; the header is the
list view's child, so its notifications arrive there): on item pre-paint
under dark, draw the label with `ThemeSysColor(COLOR_BTNTEXT)` (taking
`CDRF_SKIPDEFAULT` and drawing the text if the DC-color route is ignored
by the themed header).

**Rationale**: `DarkMode_ItemsView` (applied in 028, `themes.cpp:378`)
darkens the header background but its text stays black unless the
undocumented `SetPreferredAppMode` is used — which 028 deliberately
avoided (documented APIs only). Custom draw is the documented escape
hatch.

**Alternatives**: calling undocumented uxtheme ordinals 133/135 —
rejected: 028 explicitly excluded undocumented APIs, and they affect the
whole process; theming the header `DarkMode_Explorer` — no dark header
text part either.

### R8 — Disabled toolbar text: dark single-pass branch in `CToolBar`

**Decision**: In the disabled-text path of `src/toolbar2.cpp`
(`:648,:659`), when `IsDarkThemeActive()`, replace the classic two-pass
emboss with a single `COLOR_GRAYTEXT` draw (dark value 150/150/150).
Light mode keeps the emboss untouched.

**Rationale**: The emboss idiom (`BTNHIGHLIGHT` over `BTNSHADOW`) is
unreadable on a dark face (~1.9:1). The same reasoning produced 028's
flat-disabled-text subclass for statics; this extends it to the one
remaining emboss site the Find window exposes ("Focus" caption). Being in
shared `CToolBar` code it also corrects disabled items on all toolbars —
an intended app-wide consistency gain under principle VI.

**Alternatives**: Find-local override — not possible, drawing lives in
shared `CToolBar`; leaving it — fails spec FR-002/SC-002 contrast.

### R9 — Progress bar on the status bar: dark colors at creation

**Decision**: When `SetTwoStatusParts` creates the `PROGRESS_CLASS`
child (`finddlg1.cpp:1471`) and dark is active, strip its visual-style
theme (`SetWindowTheme(L"", L"")`) and set
`PBM_SETBKCOLOR`/`PBM_SETBARCOLOR` from the dark palette
(`COLOR_BTNSHADOW` track, `COLOR_HIGHLIGHT` bar). The control is
created/destroyed per search, so theme switches are naturally covered;
`OnColorsChange` re-colors it if one is alive during a switch.

**Rationale**: Themed progress bars ignore both PBM color messages; the
classic renderer honors them — the standard documented technique.

**Alternatives**: owner-drawing a progress strip — overkill for a
transient control.

### R10 — Out of scope (recorded deliberately)

- `CToolbarHeader::OnPaint` (`src/gui.cpp:2860-2877`) has the same
  missing-`SetTextColor` defect class but is not used by the Find window
  — left untouched (spec scope; candidate for a future sweep).
- The high-contrast guard, live-switch broadcast
  (`WM_USER_COLORCHANGEFIND`), and Default-theme passthrough invariant
  all come from the 028 engine and are reused, not modified.

## Resolved unknowns

No `NEEDS CLARIFICATION` items remain: every defect has a measured root
cause and a decided, precedented fix; no new configuration, no new
dependencies (dwmapi/uxtheme already linked), no dialog-template or
layout changes required.
