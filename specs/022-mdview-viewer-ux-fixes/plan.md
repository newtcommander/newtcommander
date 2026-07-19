# Implementation Plan: mdview Viewer UX Fixes

**Branch**: `022-mdview-viewer-ux-fixes` | **Date**: 2026-07-19 | **Spec**: [spec.md](./spec.md)

## Summary

Six contained fixes to the feature-021 WebView2 viewer, all in
`src/plugins/mdview/webview.{h,cpp}` and `viewer.{h,cpp}` (+ menu/label strings).
No new dependencies; all WebView2 APIs used are already in the vendored SDK.

## Technical Context

**Language/Version**: C++20, MSVC v143. **Dependencies**: existing (WebView2 SDK
1.0.4078.44, md4c). **Target**: Windows 11+. **Testing**: htmlgen unit harness
(extended for source view) + Debug x64 build + GUI smoke test.

## Constitution Check

PASS — bug-fix increment, contained in mdview; no build/shared changes
(NFR-002); security invariants preserved (FR-014); incremental & revertible.

## Root causes & fixes

### FR-001/002 — Focus after F3 (bug #1)
Root cause: the WebView2 content never receives focus after creation, so
keyboard events go nowhere until the user clicks in (which focuses the content).
Fix: add `CMdWebHost::Focus()` → `controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC)`.
Call it (a) from `OnReady`, (b) on `add_NavigationCompleted` (content is loaded),
and (c) from a new `WM_SETFOCUS` handler in `viewer.cpp` (forward frame focus to
the content). The existing `SetForegroundWindow` in the thread body then results
in content focus once ready.

### FR-003/004/005/006/007 — Zoom (bugs #2, #3, #4)
Root causes: `IsZoomControlEnabled(FALSE)` disables Ctrl+wheel and Ctrl+±; the
accelerator handler checks only VK `'0'` not `VK_NUMPAD0`; the title never shows
zoom. Fixes:
- Set `put_IsZoomControlEnabled(TRUE)` so the engine handles Ctrl+wheel and
  Ctrl+± natively.
- Register `add_ZoomFactorChanged` → read `get_ZoomFactor`, update the persisted
  `g_zoom` (rounded percent) and the title. This keeps app state in sync with
  engine-driven zoom and is the single source of truth.
- In `AcceleratorKeyPressed`, stop handling Ctrl+Plus/Minus (let the engine
  zoom), and handle Ctrl+0 **and** Ctrl+`VK_NUMPAD0` → reset to 100%
  (`SetZoom(100)`), because Ctrl+0 reset is a browser-accelerator key that is
  disabled by the lockdown. Add `VK_NUMPAD0` to the frame accelerator table too.
- `SetZoom()` / menu items call `put_ZoomFactor`, which triggers
  `ZoomFactorChanged`, syncing `g_zoom` + title. `UpdateTitle()` appends
  ` (NNN%)`.

### FR-008/009/010/011 — Search (bug #5)
Root cause: `DoFind` called `Regenerate()` (which navigates a full reload) and
then immediately issued a second fragment navigation; the two race, so the
`<mark>` elements are not in the DOM when the fragment scroll happens → nothing
visible. Fix — single navigation with cache-busting:
- `CMdWebHost` tracks a `docVersion`, incremented in `SetDocument`. `Navigate(frag)`
  builds `https://mdview.invalid/doc.html?v=<docVersion>[#frag]`. The interceptor
  matches the document by path (ignoring the `?query`/`#fragment`).
- Changing the term → regenerate marks + `SetDocument` (bumps version) → one
  `Navigate("mdfind-0")` forces a full reload with the new marks and scrolls.
- Find Next/Prev (same term) → no `SetDocument`; `Navigate("mdfind-N")` keeps the
  same version → same-document fragment scroll (no reload).
- `DoFind` rewritten: prompt branch regenerates + SetDocument, then computes the
  wrapped index and navigates once. `matchCount` comes from generation (already
  unit-tested). `mark:target` CSS highlights the active match.

### FR-012/013 — Open as Text (bug #6)
Root cause: `ViewFileInPluginViewer` is main-thread-only; the viewer runs in a
`CViewerThread`, so the call is a silent no-op. Fix:
- **User action** (Ctrl+U / menu): a robust **in-window source toggle**. A
  `SourceMode` flag; when on, `RebuildHtml` wraps the raw decoded text in a
  themed escaped `<pre>` document instead of parsing Markdown. Menu label
  becomes a checkable "View &Source\tCtrl+U". Zoom/find work on the source.
- **Engine-unavailable fallback**: handle it in `CPluginInterfaceForViewer::ViewFile`
  (which runs on the main thread) — if `!RuntimeAvailable()`, open the internal
  text viewer via `ViewFileInPluginViewer(NULL,…)` directly and return, without
  spawning the WebView2 thread. This removes the thread-boundary problem for the
  fallback path.

## Files touched

- `src/plugins/mdview/webview.h/.cpp` — `Focus()`, zoom-control + `ZoomFactorChanged`,
  `docVersion` + query-based `Navigate`, interceptor path match, accel changes.
- `src/plugins/mdview/viewer.h/.cpp` — `WM_SETFOCUS`, `UpdateTitle` percent,
  `SourceMode` toggle + `RebuildHtml` source path, `DoFind` rewrite, `ViewFile`
  main-thread fallback, zoom title sync callback.
- `src/plugins/mdview/lang/lang.rc2`, `mdview.rh2` — menu label / string tweaks.
- `tests/mdview_htmlgen_test/` — add a source-view generation case.

## Verification

Debug x64 build clean; unit tests pass (incl. source-view); GUI smoke test of
all six behaviors (manual, by the user).
