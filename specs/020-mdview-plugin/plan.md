# Implementation Plan: mdview — Rendered Markdown Viewer Plugin

**Branch**: `020-mdview-plugin` | **Date**: 2026-07-18 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `specs/020-mdview-plugin/spec.md`

## Summary

Add **mdview**, a read-only viewer plugin (`.spl` + english `.slg`) that renders
Markdown when the user presses F3 on a `.md`/`.markdown` file. Per the clarified
decisions (spec §Clarifications) v1 uses a **static, script-free native
rendering surface** with a **self-contained Markdown parser** — no browser
engine, no active content, no network on open. Concretely (an engineering
decision made here, see research.md): the rendering surface is a standard
**RichEdit 4.1** control fed generated RTF, which realizes the clarified
"static native renderer" while delivering selection, in-document search and zoom
natively (FR-071/FR-073) and satisfying every security invariant by
construction. The plugin plumbing (entry point, viewer interface, thread-per-
window, lock handshake, menu, config) is adapted from the in-tree `demoview`
viewer plugin; the build wiring follows the newest plugin (`sftp`).

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI + shared plugin SDK (`src/plugins/shared`);
**RichEdit 4.1** (`MSFTEDIT.DLL`, `MSFTEDIT_CLASS`); **WIC** (system image codecs,
per feature 006) and vendored **nanosvg** for images. **No external/NuGet
dependencies.** A self-contained Markdown parser is written in-tree (md4c — MIT,
single-file — documented as the production-grade upgrade, deferred because
network vendoring is not reliable under autonomous execution).
**Storage**: Plugin-private registry key via the SDK registry API (scheme id,
follow-theme flag, per-polarity slots, window placement, zoom).
**Testing**: build verification (Debug + Release x64) + fixture-driven manual
runtime checks (analysis/testing.md catalog).
**Target Platform**: Windows 11+, x64 (and x86 as the plugin build matrix
requires).
**Project Type**: Salamander viewer plugin (`src/plugins/mdview/`).
**Performance Goals**: FR-090 (README < 500 ms; 1 MB < 2 s; scheme switch
< 300 ms; no scroll hitch).
**Constraints**: constitution IV (pure WinAPI), VI (no process-wide visual side
effects, house-style dialogs), GPLv2-compatible vendored deps only; interface
version 104 (UTF-8 + long-path ABI); no fixed `MAX_PATH` buffers.
**Scale/Scope**: one new plugin, ~12–16 source/resource files + 2 vcxproj; a
v1 covering the P1/P2 user stories with documented deferrals.

## Constitution Check

*GATE: Must pass before Phase 0. Re-check after design.*

- **I. Build Reproducibility** — ✅ Standard `build.cmd`; `mdview=on` added to
  `plugins.cfg`; two MSBuild projects (`mdview`, `lang_mdview`) via the shared
  props chain; no manual steps.
- **II. Backward Compatibility** — ✅ Additive plugin; no core/API change;
  registers its viewer masks on first install respecting user overrides;
  Alt+F3 still shows source.
- **III. Incremental Modernization** — ✅ New isolated module; adapts existing
  demoview/sftp patterns; touches core only where a plugin must (solution +
  `plugins.cfg`).
- **IV. Windows Platform Commitment** — ✅ Pure WinAPI + standard controls
  (RichEdit) + system WIC; no cross-platform frameworks; **no browser engine**
  (WebView2 explicitly out per clarify Q2).
- **V. Plugin Architecture Preservation** — ✅ Implemented as a self-contained
  viewer plugin using the documented SDK contract; a genuinely new capability
  behind the plugin boundary.
- **VI. UI Consistency** — ✅ RichEdit is a standard themed control used
  normally (theme colors via `EM_SETBKGNDCOLOR`/char formats, not process-wide
  restyling); any config page uses the house `DIALOGEX`/`DS_SHELLFONT`
  template; **no** `ICC_STANDARD_CLASSES`, **no** plugin manifest, **no**
  subclassing standard controls to restyle them.

**Result**: PASS. One deviation from the literal wording of clarify Q2's
option (RichEdit rather than a from-scratch GDI/DirectWrite renderer) is
recorded in Complexity Tracking with its justification; it preserves Q2's
intent (static, script-free, no browser engine) fully.

## Project Structure

### Documentation (this feature)

```text
specs/020-mdview-plugin/
├── spec.md              # clarified feature spec
├── plan.md              # this file
├── research.md          # Phase 0 — decisions & rationale
├── data-model.md        # Phase 1 — internal model (blocks/inline/theme/config)
├── contracts/
│   └── plugin-interface.md   # SDK methods implemented + internal module APIs
├── quickstart.md        # build + fixture test steps
├── analysis/            # six specify-phase agent reports (evidence)
└── tasks.md             # Phase 2 (/speckit.tasks)
```

### Source Code (repository root)

```text
src/plugins/mdview/
├── precomp.h / precomp.cpp     # PCH: windows.h, richedit.h, SDK headers
├── mdview.cpp                  # plugin entry, CPluginInterface, Connect/AddViewer, config
├── mdview.h                    # plugin-wide declarations, globals, versions
├── mdview.def                  # exports: SalamanderPluginEntry, SalamanderPluginGetReqVer
├── viewer.cpp / viewer.h       # CViewerWindow: RichEdit host, thread, lock, menu, keys, search, zoom, links
├── mdparser.cpp / mdparser.h   # self-contained Markdown → document model
├── rtfrender.cpp / rtfrender.h # document model + theme → RTF; anchor table; plain-text index
├── themes.cpp / themes.h       # 10 color schemes + roles + contrast self-check (debug)
├── highlight.cpp / highlight.h # best-effort lexical syntax highlighting (tier-1 languages)
├── config.cpp / config.h       # Load/SaveConfiguration, defaults, corruption tolerance
├── mdview.rh2 / mdview.rc2     # resource ids + version/string/menu resources
├── versinfo.rh2                # version info
├── lang/lang.rc2               # english .slg strings + menu
├── res/                        # plugin + toolbar icons
└── vcxproj/
    ├── mdview.vcxproj (+ .filters) / mdview.props
    └── lang_mdview.vcxproj / lang_mdview.props
```

Also touched (minimal, required for a plugin): `src/vcxproj/salamand.sln`
(add both projects), `plugins.cfg` (`mdview=on`), `doc/third_party.txt`
(attribution if/when md4c is vendored — noted, not required for the hand-parser
v1).

**Structure Decision**: Standard Salamander viewer-plugin layout modeled on
`demoview` (plumbing) and `sftp` (build wiring).

## Phased Approach

- **Phase 0 — research.md**: lock the parser decision (self-contained vs md4c),
  the rendering surface (RichEdit + RTF) with security/fidelity analysis, image
  strategy (placeholder v1 → WIC/nanosvg follow-up), theme→RTF mapping, link
  scheme allowlist mechanics, encoding pipeline, and the demoview/sftp reuse map.
- **Phase 1 — data-model.md + contracts/ + quickstart.md**: the internal
  document model (block/inline node kinds), theme and config structures, the
  SDK methods implemented and the internal module boundaries, and the
  build/test quickstart.
- **Phase 2 — tasks.md** (`/speckit.tasks`): ordered, dependency-aware tasks.
- **Phase 3 — implement** (`/speckit.implement`): create files, wire build,
  compile Debug x64, iterate, register, smoke-test.

## Risks & Mitigations

- **Custom parser fidelity** vs FR-010 CommonMark 0.31.2 → v1 ships a pragmatic
  block+inline parser (documented subset); md4c swap is a bounded follow-up
  keeping the same document-model boundary. Recorded as a deviation.
- **RichEdit table/image fidelity** → v1 renders tables via RTF tables
  (best-effort) and images as labeled placeholders (safe, never-crash);
  inline raster images via WIC are a documented enhancement.
- **Build wiring correctness** (the classic first-plugin trap: `.def` exports,
  `english.slg`, `plugins.cfg` line) → model exactly on sftp/demoview; verify
  load in Plugin Manager.
- **Long-path/Unicode seams** (features 011–015 defect classes) → use SDK
  `SplU8ToWExtAlloc`; no fixed `MAX_PATH` buffers; UTF-8 path convention.
- **Security invariants** → guaranteed by construction with RichEdit (no script
  engine/DOM/network); links routed through an allowlist gate; images
  relative-path-only; no remote fetch.

## Complexity Tracking

| Deviation | Why needed | Simpler / literal alternative rejected because |
|-----------|-----------|-----------------------------------------------|
| Rendering via **RichEdit** instead of a from-scratch GDI/DirectWrite engine (clarify Q2 wording) | Delivers selection, search (EM_FINDTEXTEX) and zoom (EM_SETZOOM) natively, is a standard themed control (constitution VI), and preserves Q2's actual intent (static, script-free, no browser engine, invariants by construction) — enabling a buildable, working v1 | A from-scratch layout+selection+hit-test engine is very high risk to deliver correctly and would reimplement what RichEdit provides; the browser-engine alternative was rejected in clarify Q2 |
| **Self-contained parser** instead of vendored md4c | Keeps the build dependency-free and reliable under autonomous execution (no network vendoring) | Vendoring md4c is the documented production upgrade; deferred, not rejected — the document-model boundary is kept so the swap is localized |
