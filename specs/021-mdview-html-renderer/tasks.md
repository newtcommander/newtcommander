# Tasks: mdview HTML Rendering Surface

**Input**: Design documents in `specs/021-mdview-html-renderer/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/
**Tests**: included (the owner directive requires a properly tested implementation).

Format: `[ID] [P?] [Story] Description`. [P] = parallelizable (different files).
Pinned versions: md4c `master` (single-file); WebView2 SDK **1.0.4078.44**.

---

## Phase 1: Setup & Governance

- [ ] T001 Ratify FR-070: amend feature-020 `specs/020-mdview-plugin/spec.md`
  Decisions Log — Q1 (raw HTML → rendered) and Q2 (browser engine adopted),
  per `analysis/html-renderer.md` §9.
- [ ] T002 [P] Vendor md4c → `src/common/dep/md4c/{md4c.h, md4c.c, LICENSE.md}`
  (copy from scratchpad ref; MIT).
- [ ] T003 [P] Vendor WebView2 SDK → `src/common/dep/webview2/`:
  `include/WebView2.h`, `include/WebView2EnvironmentOptions.h`,
  `lib/x64/WebView2LoaderStatic.lib`, `lib/x86/WebView2LoaderStatic.lib`,
  `LICENSE.txt`, `VERSION.txt` (extract from scratchpad `webview2.nupkg`
  `build/native/…`; BSD-3, v1.0.4078.44).
- [ ] T004 [P] Add third-party notices for md4c (MIT) and WebView2 SDK (BSD-3)
  to `doc/` notices file.

**Checkpoint**: deps committed; build inputs self-contained.

---

## Phase 2: Foundational (blocks all rendering)

- [ ] T005 Build config: edit `src/plugins/mdview/vcxproj/mdview.props` — add
  WebView2 include dir + `..\..\..\common\dep\webview2\lib\$(ShortPlatform)`
  lib dir + `WebView2LoaderStatic.lib;shlwapi.lib;ole32.lib` deps; raise
  `WINVER`/`_WIN32_WINNT` to `0x0A00`.
- [ ] T006 Build config: edit `mdview.vcxproj` — add `..\..\..\common\dep\md4c\md4c.c`
  with `<ObjectFileName>$(IntDir)lib_md4c.obj` + `<PrecompiledHeader>NotUsing`;
  add `htmlgen.cpp`, `webview.cpp` ClCompile items; add ClInclude for new headers.
- [ ] T007 [P] `render.cpp/.h`: remove RTF path (`MdRenderMarkdown`, `BuildHeader`,
  all `emit*`, RTF color table, CHARRANGE model); KEEP `MdDetectDecode`,
  `MdThemes[]` + lookups, `MdSlug`. Update `render.h` accordingly (remove
  `MdRenderResult`/RTF structs; keep `MdRenderLimits`, `MDCF_*`, `HlRun`).
- [ ] T008 [P] [US1] htmlgen contract types: `htmlgen.h` (`MdHtmlResult`,
  `MdImageRef`, `MdRenderHtml`, `MdBuildThemeCss`) per contracts/htmlgen.md.
- [ ] T009 [US1] Implement `htmlgen.cpp`: md4c `MD_PARSER` callbacks
  (`MD_DIALECT_GITHUB`, NO `MD_FLAG_NOHTML`) → HTML; block/span/text handlers;
  HTML-escaping of NORMAL/CODE text; verbatim emit of `MD_TEXT_HTML`/
  `MD_BLOCK_HTML` (raw HTML pass-through, FR-020/022); heading `id=` via
  `MdSlug` (dedup); tables with `text-align` from `MD_ALIGN_*`; task-list
  checkboxes; `HighlightCode`→`hl-*` spans; image `src` classification +
  rewrite to `https://mdview.invalid/img/<n>`; caps.
- [ ] T010 [US1] Implement `MdBuildThemeCss`: base stylesheet (body inset,
  reading measure ~46rem, table/code/blockquote styling, `img{max-width:100%}`,
  `mark`) + `:root` variables from `MdTheme`/`MdSyntax`.
- [ ] T011 [US1] `webview.h`/`webview.cpp`: `CMdWebHost` skeleton per
  contracts/webhost.md — async `Create` (env→controller via WRL Callback),
  `CoInitializeEx`, user-data folder `%LOCALAPPDATA%\Open Salamander\mdview.WebView2\`,
  `IsReady`, `Resize`, `Destroy`, `OnReady`/`OnInitFailed` callbacks.
- [ ] T012 [US1] `viewer.cpp/.h`: replace RichEdit host with `CMdWebHost`;
  refactor `OpenFile` to render-deferred; `WM_CREATE` starts host; controller-
  ready → `RenderDocument` (UTF-16→UTF-8 → `MdRenderHtml` → navigate via
  interceptor); `WM_SIZE`→`Resize`; remove `HRich`, `StreamInCb`, `EM_STREAMIN`.

**Checkpoint**: a Markdown file renders as HTML in WebView2 (US1 core).

---

## Phase 3: US1 — Faithful rendering (P1, MVP)

- [ ] T013 [US1] Content serving: `WebResourceRequested` + `AddWebResourceRequestedFilter(L"*")`
  default-deny; serve `doc.html` from `SHCreateMemStream`; wire `MdHtmlResult`.
- [ ] T014 [US1] Golden-file test harness `tests/mdview_htmlgen_test/`: console
  exe linking md4c + htmlgen + render (themes/slug) + highlight; runs fixtures
  → compares to `.html` goldens; add fixtures (table+alignment, nested lists,
  code+highlight, blockquote, task list, headings/anchors).
- [ ] T015 [US1] Verify tables render as grids with alignment, body inset/reading
  measure (SC-001, SC-002) — golden assertions + runtime smoke.

**Checkpoint**: US1 independently demonstrable.

---

## Phase 4: US2 — Safe untrusted documents (P1)

- [ ] T016 [US2] Settings lockdown at controller-ready (webview.cpp): script off;
  context-menu/devtools/statusbar/error-page off; browser-accel-keys off;
  zoom-control off; autofill/password off; SmartScreen off; host-objects off;
  web-message off; pinch/swipe off; `--disable-background-networking` via
  `CoreWebView2EnvironmentOptions`. QI Settings→Settings3/4/8 guarded.
- [ ] T017 [US2] Navigation gate: `add_NavigationStarting` + `add_NewWindowRequested`
  cancel all except initial `doc.html` + same-doc `#fragment`; `add_ProcessFailed`
  → `OnProcessFailed`. Default-deny in interceptor for non-`mdview.invalid`.
- [ ] T018 [P] [US2] Security corpus `tests/.../fixtures/security/` (script,
  onerror, javascript:, remote img/beacon, form, meta-refresh, iframe,
  path-traversal src) + generator-level assertions.
- [ ] T019 [US2] Runtime security assertion notes: procedure to confirm zero
  script effect + zero content-network (quickstart.md §security).

**Checkpoint**: adversarial corpus inert (SC-006).

---

## Phase 5: US5 — No regression / parity (P1)

- [ ] T020 [US5] Accelerator routing: `add_AcceleratorKeyPressed` → CM_* ids
  (Ctrl+F/F3/Shift+F3, Ctrl+±/0, F9, Ctrl+U, Esc, next/prev).
- [ ] T021 [US5] Zoom via `put_ZoomFactor(g_zoom/100)`, 50–300%, persisted.
- [ ] T022 [US5] Schemes: F9 cycle + follow-system → regenerate CSS + re-navigate
  with scroll-restore `#fragment`; RefreshSchemeChecks preserved.
- [ ] T023 [US5] Find: mark-injection — regenerate HTML with `<mark id=mdfind-N>`,
  navigate `#mdfind-N`; Ctrl+F dialog (IDD_FIND) + F3/Shift+F3 cycle; not-found msg.
- [ ] T024 [US5] Link gate (`OnActivateLink`): internal `#anchor`→ScrollToAnchor;
  local `.md`→SpawnViewer (new window); other local→path-only; http/https/mailto
  →ShellExecuteW (NO ftp); else blocked.
- [ ] T025 [US5] Encoding/long-path/size-gate/OpenAsText preserved end-to-end;
  title bar shows encoding.
- [ ] T026 [US5] Init-failure + ProcessFailed → error state + OpenAsText
  fallback (FR-038a, single backend).

**Checkpoint**: v1 parity checklist passes (SC-005).

---

## Phase 6: US4 — Embedded HTML (P2)

- [ ] T027 [US4] Confirm raw HTML pass-through renders (kbd/sub/sup/table/br) —
  fixtures + runtime; verify active/script parts inert under lockdown (ties US2).

## Phase 7: US3 — Inline images + remote consent (P2)

- [ ] T028 [US3] Interceptor image serving: local relative under DocDir → bytes;
  absolute/UNC/traversal → refused placeholder.
- [ ] T029 [US3] Remote-image consent: placeholder + per-doc consent action
  (menu/affordance naming host); on consent, interceptor fetches via WinHTTP
  (no cookies, redirect-capped); session-scoped, no global (FR-012).
- [ ] T030 [US3] Consent dialog (if modal) `DIALOGEX`/`DS_SHELLFONT` (Principle VI).

## Phase 8: US6 — Extensibility (P3)

- [ ] T031 [US6] Confirm htmlgen/CSS seam supports adding a rendered enhancement
  without touching window/thread/file layers (design note in code comments).

---

## Phase 9: Build, Polish & Verify

- [ ] T032 Build Debug x64 (`msbuild … /t:mdview /p:Configuration=Debug /p:Platform=x64`);
  iterate to clean compile + link of `mdview.spl`.
- [ ] T033 Run golden-file + security generator tests; fix mismatches.
- [ ] T034 Runtime smoke: launch Salamander Debug x64, confirm mdview loads,
  F3 renders (document GUI visual confirmation as manual step).
- [ ] T035 [P] Update `IMPLEMENTATION_NOTES.md`; refresh memory; finalize notices.

---

## Dependencies

- T001–T004 (setup) before everything.
- T005–T012 (foundational) block all user stories.
- US1 (T013–T015) is the MVP; US2/US5 layer on the same host; US3/US4/US6 follow.
- T032 build after code lands; T033 tests after build; T034 runtime last.

## Notes
- Single rendering backend (FR-038a); RTF path removed in T007.
- Commit after logical groups (as Pavel Stupka; no Co-Authored-By).
- GUI visual confirmation is the one manual step (T034).
