# Contract: Theme Engine Additions (delta on feature 028)

**Feature**: 044-fix-find-dark-mode. Delta contract on
`specs/028-visual-themes/contracts/theme-engine.md` — the 028 API,
invariants, and hook obligations remain in force unchanged. This feature
adds central behaviors inside `src/themes.cpp` and narrow obligations on
the Find window. No public API signature changes; new subclass procs are
`static` internals of `themes.cpp` unless a declaration must be shared
(then `themes.h` only).

## Inherited invariants (re-affirmed, not restated)

1. **Default = passthrough** — every new subclass MUST route to
   `DefSubclassProc` (and every new branch to the legacy code path) when
   `IsDarkThemeActive()` is FALSE. No light-mode pixel may change.
2. **High Contrast wins** — enforced automatically by invariant 1, since
   `IsDarkThemeActive()` already folds the high-contrast state in.
3. **Draw sites use accessors only** — `ThemeSysColor` /
   `ThemeSysColorBrush` / `ThemeDrawEdge`; never raw `GetSysColor` in a
   new paint path.
4. **No functional side effects** — subclasses handle paint-class
   messages only (`WM_PAINT`, `WM_ERASEBKGND`, `WM_NCPAINT`,
   `WM_DRAWITEM` forwarding); all input, focus, and layout behavior is
   `DefSubclassProc`.
5. **Idempotence** — `ThemeApplyToDialog` remains safe to call multiple
   times on the same window (`SetWindowSubclass` with a fixed id
   re-registers, never stacks). This feature *relies* on that: the Find
   window calls it a second time after `WM_INITDIALOG`.

## New central behaviors (`ThemeApplyChildEnumProc` dispatch table delta)

| Child class | 028 behavior | 044 behavior |
|---|---|---|
| `Static` with `SS_ETCHEDHORZ/VERT/FRAME` | none (excluded by text-static subclass) | install etched-line subclass: dark → paint etched edge via `ThemeDrawEdge` bevel pair; light → default |
| `Edit` | `SetWindowTheme(L"DarkMode_Explorer")` | `SetWindowTheme(L"DarkMode_CFD")` (dark border, matches combos & command line) + install disabled-edit subclass: dark AND disabled → fill `COLOR_BTNFACE`, text `COLOR_GRAYTEXT`; otherwise default |
| `msctls_statusbar32` | none (fell into scrollable-else) | install status-bar subclass (below) |
| all others | unchanged | unchanged |

Un-darkening (`ThemeApplyToDialog` on a previously darkened window with
dark now off) MUST reset the Edit theme class the same way other classes
are reset today; installed subclasses stay registered but become pure
passthroughs (invariant 1).

## Status-bar subclass (normative behavior)

When dark is active:

- `WM_ERASEBKGND`: fill client with `ThemeSysColorBrush(COLOR_BTNFACE)`,
  return non-zero.
- `WM_PAINT`: for each part (`SB_GETPARTS`, `SB_GETRECT`):
  - part text (`SB_GETTEXT`/`SB_GETTEXTLENGTH`): if the part carries
    `SBT_OWNERDRAW`, build a `DRAWITEMSTRUCT` and send `WM_DRAWITEM` to
    the parent (preserving the native owner-draw contract); else draw
    the text with `ThemeSysColor(COLOR_BTNTEXT)`, transparent
    background, native alignment/ellipsis semantics.
  - part separators (only where the native control draws them): dark
    bevel pair; `SBT_NOBORDERS` parts draw none.
  - size grip (`SBARS_SIZEGRIP`): draw the glyph using the dark bevel
    colors within the native grip rectangle.
- All other messages: `DefSubclassProc`.

When dark is inactive: every message goes to `DefSubclassProc`
(native light rendering, including the grip).

## Obligations on the Find window (hook table delta)

| Site | Obligation |
|---|---|
| `CFindDialog` `WM_INITDIALOG` (end, after status bar/menu/toolbars exist) | call `ThemeApplyToDialog(HWindow)` once more |
| `CFindDialog` `WM_DRAWITEM` for `IDC_FIND_STATUS` (`finddlg1.cpp:3823`) | `SetTextColor(ThemeSysColor(COLOR_BTNTEXT))` before `DrawTextW` (passthrough-safe in light) |
| `CFindTBHeader` | handle `WM_NCPAINT`: dark → `ThemeDrawEdge` static-edge frame; light → default. `WM_ERASEBKGND` text draw gains `SetTextColor(ThemeSysColor(COLOR_BTNTEXT))` |
| `CFoundFilesListView::WindowProc` | handle header `NM_CUSTOMDRAW`: dark → header label text in `ThemeSysColor(COLOR_BTNTEXT)` (self-draw with `CDRF_SKIPDEFAULT` if DC color is ignored); light → `CDRF_DODEFAULT` |
| `SetTwoStatusParts` progress-bar creation (`finddlg1.cpp:1471`) + `OnColorsChange` | dark → `SetWindowTheme(L"", L"")` + `PBM_SETBKCOLOR(COLOR_BTNSHADOW)` + `PBM_SETBARCOLOR(COLOR_HIGHLIGHT)`; light → leave native |
| `CToolBar` disabled-text draw (`toolbar2.cpp:648,659`) | dark → single-pass `ThemeSysColor(COLOR_GRAYTEXT)`; light → existing emboss untouched |

## Consumers unchanged

- `CFindDialog::OnColorsChange()` (`finddlg2.cpp:295-318`) — already
  re-applies `ThemeApplyToDialog` + invalidates; gains only the
  progress-bar re-color if one is alive.
- Plugin-facing exports (`spl_gen.h:3469-3503`) — signatures and
  semantics unchanged; plugin dialogs inherit the central fixes
  automatically.
- `src/packac.cpp` status bar — inherits the central status-bar subclass
  with zero code changes (verified as a side-effect surface in
  quickstart, not a goal).
