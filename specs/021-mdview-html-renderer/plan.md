# Implementation Plan: mdview HTML Rendering Surface

**Branch**: `021-mdview-html-renderer` | **Date**: 2026-07-19 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `specs/021-mdview-html-renderer/spec.md`
**Analysis basis**: [`../020-mdview-plugin/analysis/html-renderer.md`](../020-mdview-plugin/analysis/html-renderer.md)

## Summary

Replace the mdview plugin's rendering surface (hand parser → RTF → RichEdit)
with an HTML pipeline: **md4c** (vendored, MIT) parses Markdown → a custom
**HTML generator** emits a self-contained HTML document + per-theme CSS →
rendered by an embedded **WebView2** control (Windows 11 Evergreen OS
runtime), locked down to a static, script-free, offline surface. The viewer
window/thread/menu/config/encoding infrastructure of feature 020 is preserved;
only the child rendering control and the render path change. All ten security
invariants are enforced by WebView2 configuration + a re-verification test
corpus. Embedded raw HTML is rendered natively (no sanitizer); safety derives
solely from the engine lockdown. The v1 RTF/RichEdit path is removed; on engine
init failure the viewer falls back to the internal text viewer.

## Technical Context

**Language/Version**: C++20 (`/std:c++latest`), MSVC v143 (VS2022); md4c compiled as C.
**Primary Dependencies**: WebView2 SDK (headers + `WebView2LoaderStatic.lib`, BSD-3, vendored); md4c (single `.c`/`.h`, MIT, vendored); WebView2 Evergreen Runtime (Win11 OS component, not distributed); existing shared plugin libs + `splunicode.h`.
**Storage**: Plugin registry key (existing config: scheme, zoom, follow-system, window placement); WebView2 user-data folder at `%LOCALAPPDATA%\Open Salamander\mdview.WebView2\`.
**Testing**: Standalone golden-file unit test of the Markdown→HTML generator (md4c + htmlgen) built as a tiny console exe; adversarial security corpus; build (compile+link) verification; runtime plugin-load smoke check. GUI visual verification is a documented manual step (as with all prior mdview features).
**Target Platform**: Windows 11+ (x64 primary, Win32 also built); pure WinAPI + COM.
**Project Type**: Desktop application plugin (`.spl` DLL) for Open Salamander.
**Performance Goals**: First rendered paint < ~1 s for a typical README (warm faster); parsing/layout off the UI thread (WebView2 renderer is out-of-process); no UI-thread block beyond interactive latency.
**Constraints**: Self-contained build (no package managers, no build-time downloads); GPLv2-compatible deps; 10 security invariants (FR-050..057) enforced by config+test; single rendering backend (FR-038a); UI Consistency (Principle VI) — WebView2 lives in its own child HWND, no process-wide class changes; consent dialog uses `DIALOGEX`/`DS_SHELLFONT`.
**Scale/Scope**: One plugin, ~2 new source pairs (htmlgen, webview host) + surgical edits to viewer.cpp/render.cpp; vendored md4c (~1 `.c`) + WebView2 SDK (headers + 2 arch libs). New hand-written code ~1500–2200 LOC.

## Constitution Check

*GATE: evaluated pre-research and re-checked post-design.*

| Principle | Assessment |
|---|---|
| **I. Build Reproducibility** | PASS. All compiled/linked inputs are committed to the repo (md4c source; WebView2 headers + prebuilt loader `.lib` for x86+x64, license-cleared, version-pinned — same class as other committed binaries). No downloads at build time; `build.cmd` unchanged; output under `OPENSAL_BUILD_DIR`. The WebView2 *runtime* is an OS component, not a build input. |
| **II. Backward Compatibility** | PASS (with owner-ratified change). The user-facing product (F3 Markdown viewer, masks `*.md;*.markdown`) is unchanged; all v1 behaviors are preserved (US5/FR-030..041). The internal rendering surface change and raw-HTML policy change are the FR-070 amendments to feature-020 decisions, ratified by the owner's directive. No plugin API change. |
| **III. Incremental Modernization** | PASS. Delivered in phases behind a render-surface seam: P1 md4c+htmlgen+CSS (engine-independent, golden-tested) → P2 WebView2 swap → P3 images/consent/polish. Each phase is reviewable and revertible. Only mdview is touched. |
| **IV. Windows Platform Commitment** | PASS. Pure WinAPI + COM; targets Win11+; WebView2 SDK is BSD-3 (GPLv2-compatible); md4c is MIT (GPLv2-compatible). No cross-platform layer. VS2022. |
| **V. Plugin Architecture Preservation** | PASS. Work is contained in the mdview plugin; plugin interface (`spl_view.h` ViewFile/CanViewFile) unchanged. |
| **VI. UI Consistency** | PASS. WebView2 renders inside its own child HWND and does not call `InitCommonControlsEx`, embed a manifest, or subclass standard classes — no process-wide visual side effects. The remote-image consent dialog (if modal) uses `DIALOGEX` + `DS_SHELLFONT` + `FONT 8, "MS Shell Dlg"`. |

No violations → Complexity Tracking not required. One nuance recorded: the
WebView2 runtime is an external OS component; this is explicitly permitted by
Principle IV (Win11 platform capability) and does not breach Principle I
because it is not a build input (see research.md R2).

## Project Structure

### Documentation (this feature)

```text
specs/021-mdview-html-renderer/
├── plan.md              # This file
├── research.md          # Phase 0: decisions (md4c, WebView2 vendoring/lockdown/serving)
├── data-model.md        # Phase 1: viewer/render state entities
├── contracts/
│   ├── htmlgen.md       # Markdown→HTML generation contract
│   └── webhost.md       # CMdWebHost (render-surface) contract
├── quickstart.md        # Phase 1: build + test steps
└── tasks.md             # Phase 2 (/speckit.tasks)
```

### Source Code (repository root)

```text
src/common/dep/
├── md4c/                # NEW vendored: md4c.h, md4c.c, LICENSE.md
└── webview2/            # NEW vendored: include/WebView2.h,
    ├── include/           include/WebView2EnvironmentOptions.h,
    └── lib/{x86,x64}/     lib/<arch>/WebView2LoaderStatic.lib, LICENSE.txt, VERSION.txt

src/plugins/mdview/
├── mdview.cpp/.h        # UNCHANGED (entry, config, Connect/AddViewer, broadcast)
├── render.cpp/.h        # KEEP MdDetectDecode, MdThemes+lookups, MdSlug; REMOVE RTF emit + MdRenderMarkdown
├── highlight.cpp        # KEEP (HlRun runs; consumed by htmlgen via MDCF_*→CSS adapter)
├── htmlgen.cpp/.h       # NEW: md4c MD_PARSER callbacks → HTML string + link/anchor model + theme CSS
├── webview.cpp/.h       # NEW: CMdWebHost — WebView2 env/controller/lockdown/events/serving
├── viewer.cpp/.h        # EDIT: host CMdWebHost instead of RichEdit; wire nav/zoom/find/consent/fallback
├── precomp.h            # EDIT: add WebView2.h / wrl.h includes (or include in webview.cpp only)
├── mdview.def, *.rc/.rh # resources (+ consent dialog IDD)
└── vcxproj/
    ├── mdview.vcxproj   # EDIT: add md4c.c (NotUsing PCH + ObjectFileName), htmlgen.cpp, webview.cpp
    └── mdview.props     # EDIT: WebView2 include+lib dirs, WebView2LoaderStatic.lib, raise WINVER

tests/ (new, minimal)
└── mdview_htmlgen_test/ # standalone console exe: md4c+htmlgen over fixtures → golden HTML compare
```

**Structure Decision**: Single-plugin desktop feature. New rendering logic is
split into `htmlgen.*` (engine-independent, unit-testable) and `webview.*`
(WebView2 host) behind a thin owner interface, so P1 can be built and tested
before P2. Vendored libs follow the established `src/common/dep/<lib>/` +
per-file `ObjectFileName` pattern (libssh2→sftp precedent).

## Phase notes

- **Phase 0 (research.md)**: resolve md4c integration specifics, WebView2 SDK
  vendoring exact layout, content-serving strategy (WebResourceRequested vs
  NavigateToString), lockdown mapping to invariants, async init in the existing
  thread model, script-free find, consent flow, DPI/zoom, encoding path.
- **Phase 1 (data-model / contracts / quickstart)**: define the render state
  entities, the htmlgen and webhost contracts, and the build/test steps.
- **Phase 2 (tasks.md)**: ordered tasks via `/speckit.tasks`.

## Governance gate (FR-070)

Implementation is authorized by the owner's directive to complete the feature.
As the first implementation step, the feature-020 Decisions Log Q1 (raw HTML)
and Q2 (rendering surface) are amended per `analysis/html-renderer.md` §9. This
is recorded in `specs/020-mdview-plugin/spec.md` before code changes land.
