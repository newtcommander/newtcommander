# Data Model: Markdown Viewer Dark-Mode Polish

**Feature**: 037-mdview-dark-polish | **Date**: 2026-07-25

No persisted data changes: registry values (`g_scheme`, `g_schemeLight`,
`g_schemeDark`, `g_followSys`, zoom, placement) are untouched. All new state
is in-memory, owned by the viewer window, and dies with it.

## Entities

### MdTheme (existing, read-only here)

Defined in `src/plugins/mdview/render.h`. Relevant field:

| Field | Type | Use in this feature |
|-------|------|---------------------|
| `docBg` | `COLORREF` | The document background; becomes the host-window erase color and the WebView2 `DefaultBackgroundColor` |

Resolution: `CViewerWindow::EffectiveTheme()` (existing) — honors
`g_followSys` (app Dark theme wins over the OS setting) and falls back to
`MdThemeDefault(false)`.

### CViewerWindow — new members (in-memory)

| Member | Type | Purpose | Lifecycle |
|--------|------|---------|-----------|
| `BgBrush` | `HBRUSH` | Fills the client area on `WM_ERASEBKGND` with `EffectiveTheme()->docBg` | Created in `WM_CREATE`; recreated whenever the effective scheme changes (`RebuildHtml` path); destroyed in `WM_DESTROY`/destructor |
| `DarkMenus` | `bool` | Snapshot of `IsDarkThemeActive()` taken once at window creation; selects the owner-drawn menu path | Set in `WM_CREATE` before `BuildMenu()`; immutable for the window's lifetime (036 reopen-adopts convention) |
| `MenuPaint` | dark-menu helper state (see below) | Owner-draw bookkeeping for the menu bar + popups | Built with the menu; freed when the menu is destroyed / window dies |

### Dark-menu helper state (`darkmenu.h`, plugin-local)

Owner-drawn items lose their system-drawn text, so each item carries paint
data referenced via `MENUITEMINFO.dwItemData`:

| Field | Type | Notes |
|-------|------|-------|
| `text` | string (owned) | Label including `&` accelerator marker; drawn with `DrawText` (+ `DT_HIDEPREFIX` when the UI state hides underlines) |
| `isSeparator` | flag | Separators are owner-drawn too under `MIM_APPLYTOSUBMENUS` |
| `isBarItem` | flag | Top-level bar items use `ODS_HOTLIGHT`-style highlight and bar background (`COLOR_MENUBAR`) instead of popup background (`COLOR_MENU`) |
| `radio` | flag | Derived from `MFT_RADIOCHECK` (scheme list uses `CheckMenuRadioItem`); selects dot vs check glyph when `ODS_CHECKED` |

Helper also owns: the menu font (`NONCLIENTMETRICS.lfMenuFont` `HFONT`) and
no brushes of its own — background brushes come from the engine cache
(`GetThemeSysColorBrush`), which the engine owns (do not delete).

Check-state source of truth stays in the HMENU itself
(`CheckMenuItem`/`CheckMenuRadioItem` as today); the painter only reads
`ODS_CHECKED`/item flags — `RefreshSchemeChecks()` keeps working unchanged.

### CMdWebHost — new member

| Member | Type | Purpose |
|--------|------|---------|
| `BgColor` | `COREWEBVIEW2_COLOR` | Desired surface color; applied via `ICoreWebView2Controller2::put_DefaultBackgroundColor` when the controller is ready, re-applied on every `SetBackgroundColor` call while ready |

`SetBackgroundColor(COLORREF)` is callable before controller creation
completes (color is stored and applied in `ApplyControllerReady`) — mirrors
the existing deferred-`RenderPending` pattern.

## State transitions

```text
Window creation:
  ctor: Theme = scheme from config
  WM_CREATE: DarkMenus = IsDarkThemeActive()
             BgBrush   = CreateSolidBrush(EffectiveTheme()->docBg)
             BuildMenu(DarkMenus)
             Web->SetBackgroundColor(docBg)   [stored; applied on ready]
  first WM_ERASEBKGND: fills with BgBrush  → no white frame ever shown

Scheme change (menu/F9/follow-sys/cfg broadcast) — live:
  RebuildHtml(): Theme = EffectiveTheme()
                 recreate BgBrush(docBg)
                 Web->SetBackgroundColor(docBg)
                 (existing) regenerate HTML + navigate

App theme change (Default ↔ Dark) — reopen adopts:
  open windows keep DarkMenus snapshot; windows created after the
  switch read the new IsDarkThemeActive()

Window destruction:
  WM_DESTROY: delete BgBrush; free menu paint data + font
```

## Validation rules

- `BgBrush` must exist before the window becomes visible (created in
  `WM_CREATE`, which `CreateEx` completes before `ShowWindow` runs).
- Engine-owned brushes (`GetThemeSysColorBrush`) must never be deleted by the
  plugin (contract from 036, `plugin-theme-api.md`).
- In Default (light) theme, `DarkMenus == false` ⇒ the menu code path is
  identical to the current release (native rendering, no owner-draw, no
  `MENUINFO` mutation).
- `put_DefaultBackgroundColor` failure (QI unsupported) is non-fatal — host
  erase (R2) remains as the outer defense.
