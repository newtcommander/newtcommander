# Implementation Plan: Markdown Viewer Dark-Mode Polish

**Branch**: `037-mdview-dark-polish` | **Date**: 2026-07-25 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/037-mdview-dark-polish/spec.md`

## Summary

Two polish defects in the Markdown View plugin (`mdview.spl`):

1. **White flash on open** — the viewer window is created with winliblt's
   `CWINDOW_CLASSNAME2` class whose background brush is `COLOR_WINDOW + 1`
   (white), and the WebView2 controller's default surface color is also white
   until the generated HTML (with the scheme background) finishes navigating.
   Fix in two layers: the host window erases its client area with the active
   Markdown scheme's `docBg` color (`WM_ERASEBKGND` with a window-owned brush),
   and the WebView2 controller gets `put_DefaultBackgroundColor(docBg)` via
   `ICoreWebView2Controller2` so the embedded surface never paints white
   either. Both are refreshed on scheme change.

2. **Light menus in dark mode** — the viewer uses a native Win32 menu bar
   (`CreateMenu`/`SetMenu`), which Windows always renders light. The main
   application's menus are dark because they are owner-drawn (core
   `CMenuPopup`/`CMenuBar`), and feature 028 explicitly rejected the
   undocumented uxtheme dark-menu ordinals. mdview therefore follows the same
   precedent locally: when the application Dark theme is active at window
   creation, the viewer's menu bar and all its popups switch to owner-drawn
   rendering (`MF_OWNERDRAW` items + `MENUINFO.hbrBack` with
   `MIM_BACKGROUND | MIM_APPLYTOSUBMENUS`), painted with the engine palette
   exported by feature 036 (`GetThemeSysColor`/`GetThemeSysColorBrush`,
   ABI 105). In Default (light) theme the menu code path is untouched native.

No plugin ABI change, no engine change, no process-wide behavior change —
everything is contained in `src/plugins/mdview/`.

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI; WebView2 SDK (embedded,
`src/common/dep/webview2/`); winliblt (`src/plugins/shared/winliblt.*`);
theme services on `CSalamanderGeneralAbstract` (feature 036, interface
version 105): `IsDarkThemeActive`, `GetThemeSysColor`,
`GetThemeSysColorBrush`, `ThemeApplyToTopLevel`
**Storage**: existing registry config (`g_scheme`, `g_schemeLight`,
`g_schemeDark`, `g_followSys`) — unchanged, no new values
**Testing**: build via `build.cmd`; manual run-verification with screen
capture / frame-by-frame recording review (validation pattern established in
`specs/036-plugin-dark-theme/validation-results.md`)
**Target Platform**: Windows 11+
**Project Type**: desktop-app plugin (`mdview.spl` inside `newtcommander.exe`
process, viewer runs on its own thread)
**Performance Goals**: correct background from the first visible frame; no
added open latency (fix is paint-path only, no extra waits)
**Constraints**: documented WinAPI only (no undocumented uxtheme ordinals /
UAH messages — rejected in feature 028); no process-wide visual changes
(Constitution VI); Default (light) theme rendering must stay pixel-identical
**Scale/Scope**: one plugin, ~4–6 files in `src/plugins/mdview/` (viewer.cpp,
viewer.h, webview.cpp, webview.h, new darkmenu.cpp/darkmenu.h)

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Gate | Verdict |
|-----------|------|---------|
| I. Build Reproducibility | No build-system changes; new files added to the existing `mdview.vcxproj` only | PASS |
| II. Backward Compatibility | No plugin ABI change (consumes existing version-105 theme services); light-mode behavior unchanged; registry layout unchanged | PASS |
| III. Incremental Modernization | Changes confined to mdview; no refactor of adjacent code; winliblt class brush left as-is (per-window fix instead) | PASS |
| IV. Windows Platform Commitment | Documented WinAPI + embedded WebView2 SDK; no new dependencies | PASS |
| V. Plugin Architecture Preservation | Feature implemented inside the plugin using engine-exported theme services; no plugin interface modification | PASS |
| VI. UI Consistency | Dark menu colors come from the engine palette (`GetThemeSysColor`), matching the main window's owner-drawn menus; no process-wide changes (no `SetPreferredAppMode`, no `ICC_STANDARD_CLASSES`, no manifest); light mode untouched | PASS |

**Post-Phase-1 re-check**: design introduces only window-local state (one
background brush, owner-draw menu data owned by `CViewerWindow`) and QI for
`ICoreWebView2Controller2` guarded by failure fallback — all gates still PASS.

## Project Structure

### Documentation (this feature)

```text
specs/037-mdview-dark-polish/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

`contracts/` is intentionally omitted: this feature changes no external
interface — no plugin ABI virtuals, no registry schema, no file formats. It
only consumes contracts frozen by feature 036
(`specs/036-plugin-dark-theme/contracts/plugin-theme-api.md`).

### Source Code (repository root)

```text
src/plugins/mdview/
├── viewer.cpp       # WM_ERASEBKGND + scheme-brush lifecycle; dark-menu hookup
│                    # (WM_MEASUREITEM/WM_DRAWITEM routing, BuildMenu changes)
├── viewer.h         # CViewerWindow: BgBrush, dark-menu state members
├── webview.cpp      # ICoreWebView2Controller2::put_DefaultBackgroundColor
│                    # on controller ready + SetBackgroundColor() entry point
├── webview.h        # CMdWebHost::SetBackgroundColor(COLORREF)
├── darkmenu.cpp     # NEW: owner-drawn dark menu painting (bar + popups):
│                    # measure/draw items, check/radio glyphs, separators,
│                    # MENUINFO background, engine-palette colors
├── darkmenu.h       # NEW: small interface used by viewer.cpp
└── vcxproj/mdview.vcxproj  # add darkmenu.cpp/h to the project
```

**Structure Decision**: single-plugin change inside `src/plugins/mdview/`.
The dark-menu helper stays plugin-local (not in winliblt): only mdview uses a
native menu bar among themed plugins today, and Constitution III forbids
speculative shared-infrastructure refactors; promotion to winliblt can happen
later if a second plugin needs it.

## Complexity Tracking

No Constitution Check violations — table not needed.
