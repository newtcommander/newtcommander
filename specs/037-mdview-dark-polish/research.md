# Research: Markdown Viewer Dark-Mode Polish

**Feature**: 037-mdview-dark-polish | **Date**: 2026-07-25

All Technical Context unknowns were resolved by direct codebase inspection;
no external research tasks remained open.

## R1 — Root cause of the white flash

**Finding**: Two independent white surfaces are visible before the rendered
document appears:

1. The viewer top-level window uses winliblt's shared class
   `CWINDOW_CLASSNAME2`, registered with
   `hbrBackground = (HBRUSH)(COLOR_WINDOW + 1)`
   (`src/plugins/shared/winliblt.cpp:364`). The first
   `WM_ERASEBKGND` after `ShowWindow`/`UpdateWindow`
   (`src/plugins/mdview/viewer.cpp:206-208`) therefore paints the client area
   white while the WebView2 child does not exist yet (controller creation is
   asynchronous).
2. The WebView2 controller's own composition surface defaults to opaque
   white until the first navigation completes; the generated HTML carries the
   scheme background (`MdTheme::docBg`), so the surface flips from white to
   the scheme color when `Navigate` finishes.

**Decision**: fix both layers (see R2/R3). Fixing only one still leaves a
visible flash from the other.

## R2 — Host-window background fix

**Decision**: `CViewerWindow` owns a solid brush created from
`EffectiveTheme()->docBg` and answers `WM_ERASEBKGND` by filling the client
area with it (returning non-zero). The brush is created in `WM_CREATE`
(before the window is shown — `CreateEx` runs `WM_CREATE` synchronously ahead
of `ShowWindow`) and recreated whenever the effective scheme changes
(`SelectScheme`, `CM_VIEW_FOLLOWSYS` toggle, `WM_USER_VIEWERCFGCHNG`,
`RebuildHtml`). Destroyed in `WM_DESTROY`/destructor.

**Rationale**: per-window state; the first erase already uses the right
color; `WS_CLIPCHILDREN` (already set) prevents fighting with the WebView2
child once it covers the client area.

**Alternatives considered**:
- `SetClassLongPtr(GCLP_HBRBACKGROUND)` — rejected: `CWINDOW_CLASSNAME2` is
  shared by every winliblt window of the plugin and the brush must vary per
  window/scheme; mutating shared class state violates the 036 pattern of
  window-local theming.
- Changing the class brush in winliblt itself — rejected: affects all 14+
  winliblt plugins (Constitution III/VI: no cross-cutting side effects from a
  single feature).
- Keeping the window hidden until WebView2 is ready — rejected: delays
  perceived open, breaks `ShowWindow(showCmd)` semantics (maximized restore),
  and does not cover scheme-change reloads.

## R3 — WebView2 surface background

**Decision**: after controller creation (`ApplyControllerReady`,
`src/plugins/mdview/webview.cpp`), QI the controller for
`ICoreWebView2Controller2` and call
`put_DefaultBackgroundColor({0xFF, GetRValue(docBg), GetGValue(docBg),
GetBValue(docBg)})`. Expose `CMdWebHost::SetBackgroundColor(COLORREF)` so the
viewer sets it before the first `Navigate` and updates it on scheme change.
QI failure is tolerated (fall back to current behavior — R2 still removes the
host-window flash).

**Rationale**: `DefaultBackgroundColor` is the documented WebView2 mechanism
for exactly this defect (surface color shown before/between navigations). The
embedded SDK (`src/common/dep/webview2/include/WebView2.h`) declares
`ICoreWebView2Controller2`; the existing code already uses the progressive-QI
pattern for `ICoreWebView2Settings3..8`, so the runtime-version guard style is
established.

**Alternatives considered**:
- Only setting the HTML/body background earlier — rejected: the flash happens
  before the document exists.
- `ICoreWebView2Controller2` hard requirement — rejected: keep the QI
  optional to tolerate old WebView2 runtimes (engine-unavailable fallback
  already exists).

## R4 — Dark menu mechanism

**Decision**: owner-drawn native menu, plugin-local, dark theme only. When
`SalamanderGeneral->IsDarkThemeActive()` is TRUE at window creation,
`BuildMenu()` marks every item `MF_OWNERDRAW` and sets
`MENUINFO.hbrBack` (dark menu brush from `GetThemeSysColorBrush(COLOR_MENU)`)
with `MIM_BACKGROUND | MIM_APPLYTOSUBMENUS` on the menu bar. A new
`darkmenu.cpp/h` implements `WM_MEASUREITEM`/`WM_DRAWITEM`: text via
`DrawText` (respecting `DT_HIDEPREFIX` per `WM_QUERYUISTATE`), highlight for
`ODS_SELECTED`/`ODS_HOTLIGHT`, disabled text, separators, and check/radio
glyphs (the scheme list uses `CheckMenuRadioItem`; glyph choice keyed on
`MFT_RADIOCHECK` + `ODS_CHECKED` via `GetMenuItemInfo`). Colors exclusively
from the engine palette (`GetThemeSysColor`: `COLOR_MENU`, `COLOR_MENUTEXT`,
`COLOR_HIGHLIGHT`, `COLOR_HIGHLIGHTTEXT`, `COLOR_GRAYTEXT`,
`COLOR_MENUHILIGHT`, `COLOR_MENUBAR`); menu font from
`SystemParametersInfo(SPI_GETNONCLIENTMETRICS).lfMenuFont`. In Default theme
`BuildMenu()` is byte-for-byte today's native path.

**Rationale**: matches how the main application achieves dark menus
(owner-drawn `CMenuBar`/`CMenuPopup` painting with `ThemeSysColor`), uses
only documented WinAPI, and keeps the change inside the plugin window
(Constitution VI).

**Alternatives considered**:
- Undocumented uxtheme ordinals (`SetPreferredAppMode`,
  `AllowDarkModeForWindow`, `FlushMenuThemes`) — rejected: explicitly ruled
  out in feature 028 research (`specs/028-visual-themes/research.md:114-116`);
  also process-wide (`SetPreferredAppMode`), which Constitution VI forbids as
  a plugin side effect.
- Undocumented UAH menu-bar messages (`WM_UAHDRAWMENU` et al.) — rejected:
  same undocumented-API objection.
- Porting the core `CMenuBar`/`CMenuPopup` into winliblt — rejected: large
  shared-infrastructure refactor for one consumer (Constitution III);
  revisit only if a second plugin needs a themed menu bar.
- Enabling dark native menus engine-wide — rejected: changes every native
  menu in the process (shell context menus included); far beyond this
  feature's scope and 028's recorded boundary.

**Known cosmetic limits** (accepted): the popup frame/border and the menu-bar
nonclient edge are drawn by the system and may keep a thin light line; the
main application's owner-drawn menus have the same class of edge artifacts.
These do not constitute the "light menu" defect being fixed.

## R5 — Scope of "system menu" in the user's request

**Finding**: the main application's menu bar is dark because it is a custom
control; its **caption system menu** (Alt+Space / title-bar icon) is the
native system popup, which is light even in Dark theme — 028 deliberately did
not touch native system menus. mdview already gets the dark title bar via
`ThemeApplyToTopLevel` (DWM immersive dark).

**Decision**: the deliverable is the viewer's **menu bar and its drop-down
popups** rendered dark. The caption system menu must match the main
application window's caption menu (parity) — it is not independently forced
dark, because no documented mechanism exists and the main window exhibits the
same native behavior. Spec FR-004/SC-002 and US2 were refined accordingly.

**Rationale**: the spec's own consistency anchor is "consistent with the
menus of the main application window"; parity is achievable and honest,
force-darkening the native caption popup is not (documented APIs only).

## R6 — Theme/scheme change semantics

**Decision**:
- **Markdown scheme change** (F9/menu, follow-system toggle, config change
  broadcast `WM_USER_VIEWERCFGCHNG`): live — background brush and WebView2
  default background update together with the regenerated document (this is
  existing behavior for content; the new surfaces join the same code path in
  `RebuildHtml`/`Regenerate`).
- **Application theme change** (Default ↔ Dark): read at window creation,
  adopted on reopen — the convention established by feature 036 for plugin
  windows (`spl_gen.h:3461` "Theme state should be read at window creation
  time"). No live re-theming of an open viewer's menus.

**Rationale**: consistent with every other themed plugin window; avoids
rebuilding an owner-drawn menu under an open tracking loop.
