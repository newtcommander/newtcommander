# Tasks: Markdown Viewer Dark-Mode Polish

**Input**: Design documents from `/specs/037-mdview-dark-polish/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, quickstart.md

**Tests**: Not requested — this feature is verified by manual run-verification
with recorded evidence (quickstart.md → `validation-results.md`), the pattern
established by feature 036. No automated test tasks.

**Organization**: Tasks are grouped by user story. US1 (no white flash) and
US2 (dark menus) touch disjoint code paths and are independently deliverable.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2)
- Include exact file paths in descriptions

## Path Conventions

All source paths are under `src/plugins/mdview/` (single-plugin feature, see
plan.md Project Structure). Spec artifacts live in
`specs/037-mdview-dark-polish/`.

---

## Phase 1: Setup

**Purpose**: Confirm a green baseline before touching the plugin

- [X] T001 Baseline build: run `build.cmd` from the repository root (with `OPENSAL_BUILD_DIR` set) and confirm the mdview plugin builds green before any change

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story

No foundational tasks. Both stories consume infrastructure that already
exists (feature 036 theme services on `CSalamanderGeneralAbstract` ABI 105,
embedded WebView2 SDK, `MdTheme::docBg`, `EffectiveTheme()`); their code
paths are disjoint (US1: paint/WebView2 surface, US2: menu), so user story
work can begin immediately after T001.

**Checkpoint**: Baseline green — user stories can start (even in parallel)

---

## Phase 3: User Story 1 - No White Flash When Opening a Markdown File (Priority: P1) 🎯 MVP

**Goal**: The viewer window shows the active Markdown scheme's `docBg` from
its first visible frame — on open (F3), on additional windows, on loading
another file, and after scheme changes. No white/mismatched frame ever
(FR-001..FR-003).

**Independent Test**: quickstart.md "Verify US1" — 10× F3 open with a dark
scheme under screen recording: zero white frames (SC-001); scheme change then
reopen shows the new color immediately (SC-003).

### Implementation for User Story 1

- [X] T002 [US1] Add `BgBrush` (HBRUSH) member with create/destroy lifecycle to `CViewerWindow` in `src/plugins/mdview/viewer.h` and `src/plugins/mdview/viewer.cpp`: create from `EffectiveTheme()->docBg` at the top of `WM_CREATE` (before any paint can occur), delete in `WM_DESTROY`/destructor (data-model.md "CViewerWindow — new members")
- [X] T003 [US1] Handle `WM_ERASEBKGND` in `CViewerWindow::WindowProc` in `src/plugins/mdview/viewer.cpp`: fill the client rect with `BgBrush` and return non-zero; fall back to default handling if the brush is NULL (research.md R2; depends on T002)
- [X] T004 [P] [US1] Add `CMdWebHost::SetBackgroundColor(COLORREF)` in `src/plugins/mdview/webview.h` and `src/plugins/mdview/webview.cpp`: store a `COREWEBVIEW2_COLOR` (alpha 0xFF), apply it in `ApplyControllerReady` via QI to `ICoreWebView2Controller2::put_DefaultBackgroundColor`, re-apply immediately when called while the controller is ready; QI/put failure is non-fatal (research.md R3, mirrors the existing `ICoreWebView2Settings3..8` progressive-QI pattern)
- [X] T005 [US1] Wire the color through the scheme lifecycle in `src/plugins/mdview/viewer.cpp`: call `Web->SetBackgroundColor(docBg)` right after `Web->Create(...)` in `WM_CREATE`, and in `RebuildHtml()` (which already resolves `Theme = EffectiveTheme()`) recreate `BgBrush` + call `SetBackgroundColor` with the new `docBg` so scheme change / follow-system toggle / `WM_USER_VIEWERCFGCHNG` update both surfaces (data-model.md "State transitions"; depends on T002, T004)
- [X] T006 [US1] Build and run-verify US1 per `specs/037-mdview-dark-polish/quickstart.md` (10× F3 under recording with a dark scheme, large multi-MB file, scheme change + reopen, follow-system mode); record SC-001 and SC-003 evidence in `specs/037-mdview-dark-polish/validation-results.md`

**Checkpoint**: User Story 1 fully functional — MVP deliverable (flash gone)

---

## Phase 4: User Story 2 - Dark Menus in the Markdown Viewer (Priority: P2)

**Goal**: With the application Dark theme active at window creation, the
viewer's menu bar and all drop-down popups render owner-drawn with the engine
dark palette, consistent with the main window's menus; caption (system) menu
stays at parity with the main window; Default theme keeps today's native
rendering byte-for-byte (FR-004..FR-006).

**Independent Test**: quickstart.md "Verify US2" — Dark theme, open viewer:
bar + every popup dark with readable states (hover, disabled, separators,
check/radio); keyboard navigation intact; Alt+Space parity with main window
(SC-002). Does not require US1 to be merged.

### Implementation for User Story 2

- [X] T007 [P] [US2] Create `src/plugins/mdview/darkmenu.h`: item paint-data struct (`text`, `isSeparator`, `isBarItem`, `radio` — data-model.md "Dark-menu helper state") and the interface used by viewer.cpp: apply owner-draw to a menu bar + submenus, `WM_MEASUREITEM`/`WM_DRAWITEM` handlers, `WM_MENUCHAR` mnemonic matcher, and a free/cleanup entry; document that engine brushes from `GetThemeSysColorBrush` must never be deleted
- [X] T008 [US2] Implement `src/plugins/mdview/darkmenu.cpp` (research.md R4): walk the HMENU tree converting items to `MF_OWNERDRAW` with attached paint data; set `MENUINFO.hbrBack = GetThemeSysColorBrush(COLOR_MENU)` with `MIM_BACKGROUND | MIM_APPLYTOSUBMENUS`; measure with the `NONCLIENTMETRICS.lfMenuFont` font; draw popup/bar backgrounds (`COLOR_MENU`/`COLOR_MENUBAR`), selection (`COLOR_HIGHLIGHT`+`COLOR_HIGHLIGHTTEXT`, `ODS_SELECTED`/`ODS_HOTLIGHT`), disabled text (`COLOR_GRAYTEXT`), separators, check/radio glyphs keyed on `MFT_RADIOCHECK` + `ODS_CHECKED`, accelerator underlines with `DT_HIDEPREFIX` per UI state, and `WM_MENUCHAR` matching on the stored `&`-mnemonics (owner-drawn items lose automatic mnemonic handling); colors exclusively via `SalamanderGeneral->GetThemeSysColor` (depends on T007)
- [X] T009 [P] [US2] Add `darkmenu.cpp` (ClCompile) and `darkmenu.h` (ClInclude) to `src/plugins/mdview/vcxproj/mdview.vcxproj`
- [X] T010 [US2] Hook dark menus into the viewer in `src/plugins/mdview/viewer.cpp` and `src/plugins/mdview/viewer.h`: add `DarkMenus` bool snapshot of `SalamanderGeneral->IsDarkThemeActive()` taken in `WM_CREATE` before `BuildMenu()` (036 reopen-adopts convention, research.md R6); when `DarkMenus`, `BuildMenu()` applies the darkmenu conversion after building the bar; route `WM_MEASUREITEM`/`WM_DRAWITEM`/`WM_MENUCHAR` to the darkmenu handlers; free menu paint data in `WM_DESTROY`; verify `RefreshSchemeChecks()` (CheckMenuItem/CheckMenuRadioItem) still drives the drawn check/radio state; the `!DarkMenus` path must remain identical to today's native code (depends on T007, T008, T009)
- [X] T011 [US2] Build and run-verify US2 per `specs/037-mdview-dark-polish/quickstart.md`: dark bar + all popups (including Color Scheme submenu radio dot and Follow system / Allow remote images / View Source checks), hover + disabled + separators readable, keyboard navigation (F10/Alt, arrows, mnemonics), Alt+Space parity with the main window, side-by-side palette match with main-window menus; capture screenshots and record SC-002 evidence in `specs/037-mdview-dark-polish/validation-results.md`

**Checkpoint**: Both user stories functional and independently verified

---

## Phase 5: Polish & Cross-Cutting Concerns

**Purpose**: Regression safety and repo hygiene

- [X] T012 [P] Light-mode regression pass per `specs/037-mdview-dark-polish/quickstart.md` "Verify regression": Default theme menus/window pixel-identical to the previous release, light scheme first paint correct (no dark flash); record SC-004 in `specs/037-mdview-dark-polish/validation-results.md`
- [X] T013 Run clang-format (repo configuration) on all touched files: `src/plugins/mdview/viewer.cpp`, `viewer.h`, `webview.cpp`, `webview.h`, `darkmenu.cpp`, `darkmenu.h` — new files (`darkmenu.*`) formatted with the repo clang-format (17.0.3); edits in pre-existing files follow the surrounding local style because those files are not format-clean in HEAD and a full reformat would add ~1000 unrelated diff lines (Constitution III)
- [X] T014 Final gate: `build.cmd full` green from repo root; `git status` confirms changes are confined to `src/plugins/mdview/` and `specs/037-mdview-dark-polish/` (+ `.specify/feature.json` pointer)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependencies
- **Foundational (Phase 2)**: empty — nothing blocks the stories beyond T001
- **US1 (Phase 3)** and **US2 (Phase 4)**: each depends only on T001; they
  are mutually independent (disjoint code paths; T010 and T005 both edit
  viewer.cpp, so if the stories are worked in parallel, merge that file with
  care or serialize T005/T010)
- **Polish (Phase 5)**: T012 needs both stories' code complete; T013/T014
  close the feature

### Task Dependencies

| Task | Depends on |
|------|-----------|
| T003 | T002 |
| T005 | T002, T004 |
| T006 | T002–T005 |
| T008 | T007 |
| T010 | T007, T008, T009 |
| T011 | T007–T010 |
| T012 | T006, T011 |
| T014 | everything else |

### Parallel Opportunities

- **T004** (webview.cpp/h) alongside **T002/T003** (viewer.cpp/h) inside US1
- **T007** and **T009** (new header, vcxproj) alongside each other; T007 also
  parallel with all of US1
- Whole **US2** can proceed in parallel with **US1** (except the shared
  viewer.cpp edits in T005/T010 — coordinate or serialize)
- **T012** (manual light-mode pass) parallel with **T013** (formatting)

## Parallel Example: User Story 1

```text
# After T001, launch together:
Task: "T002 Add BgBrush lifecycle in src/plugins/mdview/viewer.h + viewer.cpp"
Task: "T004 Add CMdWebHost::SetBackgroundColor in src/plugins/mdview/webview.h + webview.cpp"
# Then: T003 → T005 → T006 (verification)
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. T001 baseline
2. T002–T005 implementation, T006 run-verification
3. **STOP and VALIDATE**: SC-001/SC-003 evidence recorded — the flash (the
   defect every open shows) is gone; ship-able increment

### Incremental Delivery

1. US1 → validate → MVP
2. US2 (T007–T011) → validate → dark menus consistent with the app
3. Polish (T012–T014) → light-mode regression evidence + format + final build

---

## Notes

- No automated tests: acceptance is visual by nature (first-frame color,
  menu rendering); evidence goes to `validation-results.md` per the 036
  pattern
- Commit after each task or logical group (Development Workflow: single
  concern per PR still applies to the feature branch as a whole)
- Constitution watchpoints while implementing: no undocumented APIs (R4),
  engine-owned brushes never deleted, light path untouched (T010), nothing
  outside `src/plugins/mdview/` (T014)
