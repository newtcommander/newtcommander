# Tasks: Theme-Adaptive Toolbar Icons (Dark Icon Set)

**Input**: Design documents from `/specs/029-dark-toolbar-icons/`
**Prerequisites**: plan.md, spec.md (clarified), research.md, data-model.md, contracts/dark-icon-override.md, quickstart.md

**Tests**: Unit tests are IN SCOPE (plan §D4 — SC-002 contrast bound is an executable check in saltests).

**Organization**: Grouped by user story; US1 (dark legibility) is the MVP.

## Format: `[ID] [P?] [Story] Description`

## Phase 1: Setup (Baseline)

**Purpose**: Confirm a green starting point so regressions are attributable.

- [X] T001 Build baseline: run `build.cmd` (Debug x64) from repo root and run `%OPENSAL_BUILD_DIR%newtcommander\Debug_x64\saltests.exe`; record clean build + "0 failed" (482 checks) before any change — DONE: build clean, saltests 482/0 (exe at build\newtcommander\Debug_x64\saltests\saltests.exe)

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The shared pure color helper that both the existing bitmap
transform and the new SVG adaptation use. Blocks all user stories.

- [X] T002 Add pure inline helper `ThemeDarkAdaptColor(int* r, int* g, int* b)` to `src/common/themes_palette.h` — exact math extracted from `ThemeAdjustBitmapForDarkMode` (src/themes.cpp:435-453): neutral (max−min<32 && max<140) → gray `220 − max*80/140`; saturated (max<120) → channels scaled by `170/max` clamped to 255; else unchanged. WinAPI-free (plain ints), English comments, UTF-8-BOM preserved
- [X] T003 Refactor `ThemeAdjustBitmapForDarkMode()` in `src/themes.cpp` to call `ThemeDarkAdaptColor()` per pixel — alpha un-premultiply/re-premultiply and transparent-key skip stay in place; behavior must be bit-identical
- [X] T004 [P] Add `TestDarkIconColorAdaptation()` to `src/saltests/saltests.cpp` and register it in `main()`: black→220; monotonic sweep over neutrals [0,140); neutrals ≥140 and white unchanged; dark saturated → max channel 170 with channel ratios preserved; bright saturated (e.g. 255,201,14) unchanged; every adapted neutral ≥3:1 contrast vs RGB(45,45,45) using the existing `ContrastRatio()` helper (SC-002); determinism (same input twice → same output)
- [X] T005 Build Debug x64 (`build.cmd`) and run saltests — all checks pass, check count increased; verifies T003 refactor broke nothing — DONE: 1075 checks, 0 failed

**Checkpoint**: Shared color math proven by tests — story work can begin.

---

## Phase 3: User Story 1 - Toolbar icons legible and coherent in Dark theme (Priority: P1) 🎯 MVP

**Goal**: Every command glyph (SVG-backed and raster-fallback) renders
dark-adapted while the Dark theme is active; per-icon hand-tuned override
mechanism in place (FR-001, FR-002, FR-007, FR-010).

**Independent Test**: quickstart.md §Manual verification steps 1-4 — switch
to Dark, audit all toolbars + Find window + menus; no dark-blob glyphs.

### Implementation for User Story 1

- [X] T006 [US1] `src/svg.cpp`: in `RenderSVGImage()`, when `IsDarkThemeActive()` and no dark override was loaded (see T007), after `nsvgParse` walk `image->shapes` for the enabled state and adapt every `NSVG_PAINT_COLOR` fill and stroke: convert nanosvg 0xAABBGGRR → r,g,b, apply `ThemeDarkAdaptColor()`, convert back preserving the alpha byte; keep the existing disabled-state recolor untouched; Default theme path byte-identical (no calls, no reads)
- [X] T007 [US1] `src/svg.cpp`: dark override probe in `RenderSVGImage()` — when `IsDarkThemeActive()`, first try `<exe>\toolbars\dark\<svgName>.svg` via the existing `ReadSVGFile()`; if it loads+parses, rasterize verbatim (skip T006 adaptation); on any failure fall back silently (TRACE_I) to the standard SVG + adaptation; never consulted in Default theme (per contracts/dark-icon-override.md)
- [X] T008 [P] [US1] `git mv src/res/toolbars/CilpboardCut.svg src/res/toolbars/ClipboardCut.svg` — fixes silent SVG miss for the Cut button (`ToolBarButtons[]` references "ClipboardCut", src/toolbar4.cpp:182)
- [X] T009 [P] [US1] Create `src/res/toolbars/dark/README.txt` — short English summary of the override contract (naming = standard glyph base name, dark-theme-only, verbatim use, fallback chain, design target background RGB(45,45,45)); anchors the directory in git
- [X] T010 [US1] `src/vcxproj/!populate_build_dir.cmd`: after the existing toolbars copy (line 116), add `call :mycopy_dir ..\res\toolbars\dark "%OPENSAL_BUILD_DIR%newtcommander\%1\toolbars\dark\"` (same helper/error semantics as the toolbars line) — DONE; additionally the primary deploy path `build.cmd :populate_runtime` (root, line 268) got `/E` on the toolbars robocopy so `dark\` deploys in `build.cmd full` (the populate script is the legacy interactive path)
- [X] T011 [US1] Run `build.cmd full` (Debug x64) and verify deploy: `toolbars\ClipboardCut.svg` and `toolbars\dark\README.txt` exist under `%OPENSAL_BUILD_DIR%newtcommander\Debug_x64\`; app starts (smoke) — DONE: build succeeded, both files deployed, app started and was cleanly terminated

**Checkpoint**: Dark theme shows adapted glyphs everywhere the shared icon
set is used (toolbars, menus, Find) — GUI audit per quickstart is the
user's final confirmation.

---

## Phase 4: User Story 2 - Default theme keeps today's icons unchanged (Priority: P2)

**Goal**: Zero visible change in the Default theme (FR-004).

**Independent Test**: quickstart.md step 5 — side-by-side with previous
release in Default theme.

### Implementation for User Story 2

- [X] T012 [US2] Code audit of the final diff (`git diff main -- src/svg.cpp src/themes.cpp src/common/themes_palette.h`): every new runtime branch is behind `IsDarkThemeActive()`; `ThemeAdjustBitmapForDarkMode` still returns immediately when not dark; no Default-theme file reads of `toolbars\dark\`; record the audit result in the task notes/commit message
- [X] T013 [US2] Runtime check: start Debug build in Default theme, verify toolbars render (visual parity check is user's); confirm via DebugView/TRACE that no `toolbars\dark\` probe is logged in Default — DONE: PrintWindow screenshot of Default theme shows the classic look (original icon colors); the dark probe sits inside `if (IsDarkThemeActive())` so Default cannot reach it (audit T012)

**Checkpoint**: Default theme provably untouched at code level.

---

## Phase 5: User Story 3 - Icons follow theme switches immediately (Priority: P3)

**Goal**: Immediate, artifact-free icon updates on Default↔Dark switches
(FR-005).

**Independent Test**: quickstart.md step 5 — 10× toggle, icons always match
the active theme.

### Implementation for User Story 3

- [X] T014 [US3] Verify by code trace + runtime test that `SetThemeMode()` → `ColorsChanged(TRUE, FALSE, TRUE)` → `InitializeGraphics()` re-runs `CreateToolbarBitmaps()`/`RenderSVGImages()` with the new theme state (src/themes.cpp:529-562, src/salamdr1.cpp:2400-2538, 3100-3110); toggle Dark↔Default repeatedly in the Debug build and watch for icon mismatch or GDI handle growth (Task Manager GDI objects stable after 10 toggles — FR-005/SC-004); no code change expected, fix here if the trace disproves the assumption

**Checkpoint**: All three stories functional.

---

## Phase 6: Polish & Cross-Cutting Concerns

- [X] T015 [P] Run clang-format on touched sources (`src/svg.cpp`, `src/themes.cpp`, `src/common/themes_palette.h`, `src/saltests/saltests.cpp`) per repo config; keep UTF-8-BOM — DONE: clang-format 17.0.3 (VS LLVM), zero changes needed
- [X] T016 Release x64 build (`build.cmd full release`) — compiles and links clean (close any running Release salamand.exe first — known LNK1104 pitfall) — DONE: Release x64 clean, `toolbars\dark\` + `ClipboardCut.svg` deployed in Release output too
- [X] T017 Final saltests run (Debug) — all checks green; update `specs/029-dark-toolbar-icons/checklists/requirements.md` notes if scope shifted; commit remaining work on branch `029-dark-toolbar-icons` — DONE: 1075 checks / 0 failed; no scope shift (checklist unchanged)

---

## Dependencies & Execution Order

- **Phase 1 → Phase 2 → user stories**: T002 blocks T003/T004/T006; T005 gates story work.
- **US1 (Phase 3)**: T006+T007 same file (`svg.cpp`) — sequential; T008, T009 parallel to them; T010 after T009 (deploy target must exist); T011 last.
- **US2 (Phase 4)**: audit tasks depend on US1 code being final.
- **US3 (Phase 5)**: independent of US2; needs US1 built.
- **Polish (Phase 6)**: after all stories.

### Parallel Opportunities

- T004 alongside T003 (different files).
- T008 + T009 alongside T006/T007.
- T015 tasks run independently of each other.

## Implementation Strategy

MVP = Phases 1-3 (US1). US2/US3 are verification-heavy (design already
guarantees them by construction); GUI walkthroughs in quickstart.md remain
the user's final acceptance step, consistent with prior features.
