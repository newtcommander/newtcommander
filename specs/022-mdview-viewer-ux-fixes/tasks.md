# Tasks: mdview Viewer UX Fixes

**Input**: spec.md, plan.md. Tests included (source-view generation case).
All changes contained in `src/plugins/mdview/`.

## Phase 1: Focus (US1)

- [ ] T001 [US1] `webview.{h,cpp}`: add `CMdWebHost::Focus()` →
  `controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC)`; register
  `add_NavigationCompleted` → `Focus()`; call `Focus()` at end of controller-ready.
- [ ] T002 [US1] `viewer.cpp`: add `WM_SETFOCUS` handler → `if (Web && Web->IsReady()) Web->Focus();`.

## Phase 2: Zoom (US2)

- [ ] T003 [US2] `webview.cpp`: set `put_IsZoomControlEnabled(TRUE)`; register
  `add_ZoomFactorChanged` → `get_ZoomFactor` → owner callback `OnZoomChanged(pct)`.
- [ ] T004 [US2] `webview.{h,cpp}`: add `OnZoomChanged` callback to `Callbacks`.
- [ ] T005 [US2] `webview.cpp` AcceleratorKeyPressed: remove Ctrl+Plus/Minus/Add/
  Subtract handling (engine does it); handle Ctrl+`'0'` and Ctrl+`VK_NUMPAD0` → reset.
- [ ] T006 [US2] `viewer.cpp`: `OnZoomChanged` → `g_zoom = pct; UpdateTitle();`;
  `UpdateTitle()` appends ` (NNN%)`; add `VK_NUMPAD0` to the frame accelerator table.

## Phase 3: Search (US3)

- [ ] T007 [US3] `webview.{h,cpp}`: add `docVersion` (bump in `SetDocument`);
  `Navigate(frag)` → `doc.html?v=<docVersion>[#frag]`; interceptor matches the
  document/img by path (strip `?`/`#`).
- [ ] T008 [US3] `viewer.{h,cpp}`: split `RebuildHtml()` (generate only) from
  `ShowDocument(frag)` (SetDocument+Navigate); rewrite `DoFind` to single
  navigation (prompt → RebuildHtml + SetDocument; then wrapped index →
  `Navigate("mdfind-N")`); ensure `Regenerate` uses the split helpers.

## Phase 4: Open as Text (US4)

- [ ] T009 [US4] `viewer.{h,cpp}`: add `SourceMode` flag; `RebuildHtml` emits a
  themed escaped `<pre>` of the raw source when `SourceMode`; Ctrl+U / menu
  toggles it (checkable "View &Source"); update `mdview.rh2`/`lang.rc2`.
- [ ] T010 [US4] `viewer.cpp` `ViewFile`: if `!CMdWebHost::RuntimeAvailable()`,
  open the internal text viewer via `ViewFileInPluginViewer(NULL,…)` on the main
  thread and return (no WebView2 thread). Keep in-thread `EngineFailed` as the
  belt-and-suspenders path (posts to text viewer only if reached).

## Phase 5: Build, test, verify

- [ ] T011 `tests/mdview_htmlgen_test/`: add a source-view generation assertion
  (raw text escaped in `<pre>`, no Markdown parsing) if source generation is in
  htmlgen; else a viewer-level note. Rebuild + run (target: all pass).
- [ ] T012 Build Debug x64 clean (`mdview.spl`); fix any errors.
- [ ] T013 Commit as Pavel Stupka (no co-author trailers); update memory + notes.

## Dependencies
- T001–T002 independent of T003–T010; do in order. T007 before T008 (Navigate
  contract). T012 after code; T011 alongside.
