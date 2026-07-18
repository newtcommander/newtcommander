# Tasks: mdview — Rendered Markdown Viewer Plugin

**Branch**: `020-mdview-plugin` | **Spec**: [spec.md](./spec.md) | **Plan**: [plan.md](./plan.md)

`[P]` = parallelizable (distinct files). Order respects dependencies. v1 scope
per plan (RichEdit + self-contained parser; images = placeholders).

## Phase A — Skeleton & build wiring (must build empty-but-loadable first)

- [x] **T001** Create `src/plugins/mdview/` tree; `precomp.h/.cpp` (windows.h,
  richedit.h, SDK headers), `mdview.h` (globals, versions), `mdview.def`
  (exports). Model on demoview.
- [x] **T002** Resources: `mdview.rh2` (ids), `mdview.rc2`, `versinfo.rh2`,
  `lang/lang.rc2` (english strings + View/Color-Scheme menu + About). Model on
  demoview/sftp.
- [x] **T003** `res/` icons (plugin + toolbar), reuse/adapt a simple bitmap.
- [x] **T004 [P]** `vcxproj/mdview.vcxproj` (+ `.filters`) + `mdview.props`;
  `vcxproj/lang_mdview.vcxproj` + `lang_mdview.props`. Model on sftp; list all
  source files.
- [x] **T005** Add both projects to `src/vcxproj/salamand.sln`; add `mdview=on`
  to `plugins.cfg`.
- [x] **T006** `mdview.cpp`: entry point, `CPluginInterface`, `Connect` →
  `AddViewer("*.md;*.markdown", FALSE)`, About, capabilities
  (VIEWER|CONFIGURATION|LOADSAVECONFIGURATION), `GetInterfaceForViewer`. Empty
  viewer that opens a blank RichEdit window — **first buildable milestone**.

## Phase B — Rendering pipeline

- [x] **T010 [P]** `themes.h/.cpp`: `Theme`/`SyntaxPalette` structs + the 10
  schemes with contrast-corrected values; `ThemeById`/`DefaultTheme`/iteration;
  debug contrast self-check (FR-060/061). (data-model.md)
- [x] **T011 [P]** `mdparser.h/.cpp`: encoding detect+decode (FR-050) and the
  block+inline parser → `Document` (FR-010/011/012/014); bounded (FR-092);
  GitHub slugger with Czech diacritics (FR-012).
- [x] **T012 [P]** `highlight.h/.cpp`: lexical highlighter + language alias
  table for the tier-1 set (FR-013); unknown → empty runs.
- [x] **T013** `rtfrender.h/.cpp`: `Document`+`Theme` → RTF (+ anchor/link
  tables). Headings, emphasis, code (shaded, highlighted), quotes, lists,
  tables, rules, links (`CFE_LINK`), image placeholders, HTML-literal inert,
  entities. (FR-014/015/020/024/060) Depends on T010–T012.

## Phase C — Viewer window

- [x] **T020** `viewer.h/.cpp`: `CPluginInterfaceForViewer` (`CanViewFile`,
  `ViewFile`), thread-per-window + lock handshake + `ViewerWindowQueue` (from
  demoview). Host a read-only `MSFTEDIT_CLASS` child; stream RTF via
  `EM_STREAMIN`; `EM_SETBKGNDCOLOR` per theme. (FR-001/080/102/103)
- [x] **T021** Keyboard/menu/chrome: Esc/Alt+F4 close, F3 never closes; arrows/
  PgUp-Dn/Home-End/wheel; Ctrl+A/C copy; menu File/Edit/View/Options/Help;
  title `<path> - Markdown Viewer`; window placement persist;
  next/prev-file. (FR-070/071/072/075)
- [x] **T022** Color-scheme submenu (10 radio items + Follow-system toggle),
  F9/Shift+F9 cycle, immediate re-render preserving scroll. (FR-062/064/065)
- [x] **T023** Search (Ctrl+F, F3/Shift+F3 via `EM_FINDTEXTEXW`) + zoom
  (Ctrl+wheel, Ctrl+±/0 via `EM_SETZOOM`, persisted). (FR-073)
- [x] **T024** Link gate `ActivateLink` (`EN_LINK`): `#anchor` scroll, local
  `.md` → new mdview window, http/https/mailto → ShellExecute, all else
  blocked; hover shows target. (FR-030/031/032/034/035)
- [x] **T025** Error/fallback states + "Open in text viewer"
  (`ViewFileInPluginViewer(NULL)`); encoding warning bar; size-gate & binary
  handling. (FR-081/083, FR-050 case 5)

## Phase D — Config & integration

- [x] **T030** `config.h/.cpp`: Load/Save with `Version` guard + defaults +
  corruption tolerance (FR-063/100); wire into `mdview.cpp`
  Load/SaveConfiguration; broadcast `WM_USER_VIEWERCFGCHNG` (FR-101).
- [x] **T031** Fixtures: `fixtures/` hand-written set + `gen-fixtures.ps1`
  (analysis/testing.md) — at least 01-basic, 03-blocks, 07-czech-utf8, 16-html.
- [x] **T032** `IMPLEMENTATION_NOTES.md` in the plugin dir: v1 scope vs spec
  FRs, documented deviations (self-contained parser, image placeholders,
  RichEdit), and the follow-up list.

## Phase E — Build & verify

- [x] **T040** Build Debug x64 (`build.cmd`); resolve compile/link errors until
  clean. (FR-009-equiv, SC-012)
- [x] **T041** `build.cmd full` (writes plugins.ver); confirm mdview loads in
  Plugin Manager and registers `*.md` mask.
- [~] **T042** Smoke test per quickstart.md (F3 renders; scheme switch; Ctrl+F;
  no-network security check; Esc/Alt+F3): **pending user interactive run** — the
  build is registerable (T041) but driving the GUI viewer is a user step, as with
  prior features. Automated verification = clean build + code review.
- [~] **T043** Release x64: **mdview.spl compiles+links in Release** (verified by
  a direct MSBuild of the plugin project); the full `build.cmd release` relink of
  salamand.exe needs the running Release instance closed (feature-019 LNK1104).

## Notes

- Deferred (documented, not v1): vendored md4c parser (full CommonMark 0.31.2),
  inline WIC/nanosvg image rendering, richer tables, follow-OS-theme live,
  per-monitor-V2 DPI. Tracked in T032.
- Every file open is long-path safe (no fixed MAX_PATH; `SplU8ToWExtAlloc`).
- Security invariants (FR-040…046) hold by construction (RichEdit, no network).
