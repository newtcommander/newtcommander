# Tasks: Fix Find Window Dark-Mode Rendering

**Input**: Design documents from `/specs/044-fix-find-dark-mode/`
**Prerequisites**: plan.md, spec.md, research.md (R1–R10), data-model.md,
contracts/theme-engine-additions.md, quickstart.md

**Tests**: The spec requests measurable contrast criteria (SC-002) backed
by the existing saltests WCAG suite — one test task is included in Polish.
No TDD was requested; verification is build gates + the quickstart GUI
walkthrough.

**Organization**: Tasks are grouped by user story. All code changes are
gated on `IsDarkThemeActive()` per paint message, so every task is
individually safe: light mode stays a bit-for-bit passthrough after any
subset of tasks (contract invariant 1).

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 = window opens fully dark, US2 = every text field
  readable, US3 = search progress / theme switch / light mode

## Phase 1: Setup (baseline)

**Purpose**: Confirm the working state and capture the "before" evidence
the success criteria compare against.

- [X] T001 Verify baseline on branch `044-fix-find-dark-mode`: run
      `build.cmd` (Debug x64) from the repo root — build must succeed
      before any change; launch `newtcommander.exe`, switch Options →
      Theme → Dark, press Ctrl+F and confirm the defects reproduce as
      captured in `temp/dark_find_window.png` (white separators, white
      advanced-box frame, dark-on-dark texts, light status bar). Keep a
      light-mode (Default theme) screenshot of the Find window as the
      SC-003 regression reference.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: None required — the feature-028 theme engine
(`src/themes.cpp`, dialog hooks, live-switch broadcast, High-Contrast
guard) is already in place and is the foundation this feature extends.
All three user stories can start immediately after Phase 1.

**Checkpoint**: n/a — proceed directly to user stories.

---

## Phase 3: User Story 1 - The Find window opens fully dark (Priority: P1) 🎯 MVP

**Goal**: Ctrl+F in the Dark theme shows a coherent dark window: dark
separator lines (FR-001), dark advanced-box frame (FR-003), dark edge
above the results list, dark status bar with readable idle hint (FR-004).

**Independent Test**: Dark theme → Ctrl+F → freshly opened window has no
white/light line, frame, or bar; compare against
`temp/dark_find_window.png` (quickstart walkthrough state 1).

### Implementation for User Story 1

- [X] T002 [US1] Etched-separator dark subclass (research R1) in
      `src/themes.cpp`: in `ThemeApplyChildEnumProc`'s `Static` branch
      (themes.cpp:354-362), detect `SS_ETCHEDHORZ`/`SS_ETCHEDVERT`/
      `SS_ETCHEDFRAME` (`style & SS_TYPEMASK`) and install a new static
      etched-line subclass proc (new subclass id, pattern of
      `ThemeFlatDisabledTextSubclassProc` themes.cpp:266-326): on
      `WM_PAINT` when `IsDarkThemeActive()`, paint the etched edge via
      the `ThemeDrawEdge` dark bevel pair (`COLOR_3DDKSHADOW`/
      `COLOR_3DLIGHT`, cf. themes.cpp:138-139) over a
      `COLOR_BTNFACE` fill; otherwise `DefSubclassProc`.
- [X] T003 [US1] Edit border fix (research R3) in `src/themes.cpp`:
      change the `Edit` branch (themes.cpp:348-353) from
      `L"DarkMode_Explorer"` to `L"DarkMode_CFD"` (precedent:
      src/editwnd.cpp:1709-1710 and the ComboBox branch
      themes.cpp:363-367); mirror the change on the un-darkening path so
      switching back to Default restores the same theme class as today.
- [X] T004 [US1] Status-bar dark subclass (research R5, contract
      "Status-bar subclass" section) in `src/themes.cpp`: add a
      `STATUSCLASSNAME` (`msctls_statusbar32`) branch to
      `ThemeApplyChildEnumProc` (before the trailing else,
      themes.cpp:390-398) installing a new subclass proc. Dark active:
      `WM_ERASEBKGND` fills `ThemeSysColorBrush(COLOR_BTNFACE)`;
      `WM_PAINT` custom-paints every part (`SB_GETPARTS`/`SB_GETRECT`),
      drawing plain-text parts (`SB_GETTEXT`) with
      `ThemeSysColor(COLOR_BTNTEXT)` transparent-bg and forwarding
      `SBT_OWNERDRAW` parts to the parent as `WM_DRAWITEM` with a filled
      `DRAWITEMSTRUCT` (native contract preserved), honoring
      `SBT_NOBORDERS`, and drawing the `SBARS_SIZEGRIP` glyph with the
      dark bevel colors. Dark inactive: pure `DefSubclassProc`.
- [X] T005 [P] [US1] Second theming pass (research R6) in
      `src/finddlg1.cpp`: at the end of `CFindDialog`'s `WM_INITDIALOG`
      handling (after the status bar creation at finddlg1.cpp:2930-2938,
      menu bar at :2954-2962, and header toolbars exist), call
      `ThemeApplyToDialog(HWindow)` once more so late-created children
      (status bar) receive the central theming — `NotifDlgJustCreated`
      runs before this body (src/common/winlib.cpp:726) and cannot see
      them. `ThemeApplyToDialog` is idempotent (THEME_DARKENED_PROP
      sentinel, themes.cpp:403-418).
- [X] T006 [P] [US1] Header-strip white edge (research R2) in
      `src/finddlg2.cpp`: in `CFindTBHeader::WindowProc` handle
      `WM_NCPAINT` — when `IsDarkThemeActive()`, draw the
      `WS_EX_STATICEDGE` frame (self-applied at finddlg2.cpp:339-341)
      with `ThemeDrawEdge` on the window DC, precedent
      src/filesbx1.cpp:1342-1370 (panel NC frame); otherwise default
      handling.

**Checkpoint**: Dark theme → Ctrl+F: both separators, the line above the
results list, the advanced-box frame, and the status bar body all render
dark on first open (US1 acceptance scenarios 1–3). Light mode unchanged.

---

## Phase 4: User Story 2 - Every text field is readable in every state (Priority: P2)

**Goal**: No dark-on-dark text anywhere (FR-002): disabled
advanced-options text, "Found Items: (N)", results header labels,
disabled toolbar captions — across all window states.

**Independent Test**: Dark theme → walk the window states (initial,
"Search file content" enabled, advanced options set/cleared) and confirm
every field, label, and caption is readable (quickstart states 1–3).

### Implementation for User Story 2

- [X] T007 [US2] Disabled-edit dark repaint subclass (research R4) in
      `src/themes.cpp`: extend the Edit handling from T003 to also
      install a subclass proc (own id) that, on `WM_PAINT` when
      `IsDarkThemeActive()` AND the edit is disabled
      (`!IsWindowEnabled`), fills `COLOR_BTNFACE` and draws the text
      with `ThemeSysColor(COLOR_GRAYTEXT)` (dark 150,150,150 → ≥3:1 on
      45,45,45), mirroring `ThemeFlatDisabledTextSubclassProc`
      (themes.cpp:266-326); enabled edits and light mode fall through to
      `DefSubclassProc`. Fixes "No Advanced Options"
      (`IDC_FIND_ADVANCED_TEXT`, disabled via
      `EnableWindow(..., dirty)` at src/finddlg1.cpp:1739).
- [X] T008 [P] [US2] "Found Items: (N)" text color in
      `src/finddlg2.cpp`: in `CFindTBHeader`'s `WM_ERASEBKGND` paint
      (finddlg2.cpp:555-573), add
      `SetTextColor(hdc, ThemeSysColor(COLOR_BTNTEXT))` before the
      `DrawText` at :568 (passthrough-safe: `ThemeSysColor ≡
      GetSysColor` in light mode).
- [X] T009 [P] [US2] Results-header label color (research R7) in
      `src/finddlg1.cpp`: in `CFoundFilesListView::WindowProc`
      (finddlg1.cpp:860), handle `WM_NOTIFY`/`NM_CUSTOMDRAW` from the
      list view's `SysHeader32` child: `CDDS_PREPAINT` →
      `CDRF_NOTIFYITEMDRAW`; `CDDS_ITEMPREPAINT` under
      `IsDarkThemeActive()` → draw the column label with
      `ThemeSysColor(COLOR_BTNTEXT)`; if the themed header ignores the
      DC text color, self-draw the label and return `CDRF_SKIPDEFAULT`.
      Light mode → `CDRF_DODEFAULT`. (Do not use
      `SetPreferredAppMode`/undocumented ordinals — plan constraint.)
- [X] T010 [P] [US2] Disabled toolbar caption (research R8) in
      `src/toolbar2.cpp`: in the disabled-item text path
      (toolbar2.cpp:648 and :659), when `IsDarkThemeActive()` draw a
      single pass with `ThemeSysColor(COLOR_GRAYTEXT)` instead of the
      classic `COLOR_BTNHILIGHT`-over-`COLOR_BTNSHADOW` emboss; keep the
      emboss untouched in light mode. Fixes the "Focus" caption in the
      Find header (and disabled items on all toolbars, an intended
      app-wide consistency gain).

**Checkpoint**: Dark theme: "No Advanced Options" reads gray-on-dark,
"Found Items: (0)" and header "Name"/"Path" read light-on-dark, "Focus"
is flat readable gray (US2 acceptance scenarios 1–3). US1 unaffected.

---

## Phase 5: User Story 3 - Search progress, theme switching, and light mode stay correct (Priority: P3)

**Goal**: Status-bar text stays readable during and after a search
(FR-004), the transient progress bar renders dark (research R9), and live
theme switches cover everything (FR-006). Light mode and High Contrast
are untouched (FR-005/FR-007) — guaranteed by the passthrough gating of
every prior task and verified in Polish.

**Independent Test**: Dark theme → run a search over a large tree: path
text and result summary light-on-dark throughout; toggle Default ↔ Dark
with the window open (once mid-search) — everything follows
(quickstart states 4–5 + live-switch section).

### Implementation for User Story 3

- [X] T011 [US3] Searched-path owner-draw color in `src/finddlg1.cpp`:
      in the `WM_DRAWITEM` handler for `IDC_FIND_STATUS`
      (finddlg1.cpp:3823-3843), add
      `SetTextColor(dis->hDC, ThemeSysColor(COLOR_BTNTEXT))` next to the
      existing `SetBkMode(TRANSPARENT)` before `DrawTextW`
      (passthrough-safe in light mode; completes the contract's
      owner-draw obligation that pairs with T004's forwarding).
- [X] T012 [US3] Dark progress bar (research R9) in `src/finddlg1.cpp`:
      in `SetTwoStatusParts` where the `PROGRESS_CLASS` child is created
      (finddlg1.cpp:1471), when `IsDarkThemeActive()` call
      `SetWindowTheme(hProgress, L"", L"")` then
      `PBM_SETBKCOLOR` = `ThemeSysColor(COLOR_BTNSHADOW)` and
      `PBM_SETBARCOLOR` = `ThemeSysColor(COLOR_HIGHLIGHT)`; leave the
      native themed control untouched in light mode. Factor the
      color-application into a small helper reusable from T013.
- [X] T013 [US3] Live-switch coverage for the progress bar in
      `src/finddlg2.cpp`: in `CFindDialog::OnColorsChange()`
      (finddlg2.cpp:295-318), if a status-bar progress child is alive,
      re-apply the T012 helper (dark → strip theme + dark colors; light
      → restore the visual-styles theme) so a theme switch during a
      running search re-colors it (spec FR-006, SC-004 mid-search
      switch).

**Checkpoint**: All three stories complete — full quickstart walkthrough
states 1–5 pass in dark; live switching clean both directions.

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Test coverage for the measurable criteria, formatting, build
gates, and the documented verification the spec's success criteria
require.

- [X] T014 [P] Contrast assertions in `src/saltests/saltests.cpp`:
      extend the existing theme WCAG suite (saltests.cpp:573-700) with
      the pairs this feature newly relies on — dark
      `COLOR_BTNTEXT`(240,240,240) on `COLOR_BTNFACE`(45,45,45) ≥ 4.5:1
      and dark `COLOR_GRAYTEXT`(150,150,150) on `COLOR_BTNFACE` ≥ 3:1
      (values from `src/common/themes_palette.h:19-46`); run the full
      saltests suite green.
- [X] T015 clang-format every touched file (`src/themes.cpp`,
      `src/themes.h` if changed, `src/finddlg1.cpp`, `src/finddlg2.cpp`,
      `src/toolbar2.cpp`, `src/saltests/saltests.cpp`) per the
      repository configuration.
- [X] T016 Build gates from the repo root: `build.cmd` (Debug x64) and
      `build.cmd full release` (Release x64) — both must succeed with no
      new warnings in the touched files.
- [X] T017 Dark-theme GUI walkthrough per
      `specs/044-fix-find-dark-mode/quickstart.md`: all five window
      states + multi-instance (FR-008) + side-effect surfaces (Find
      Settings/Advanced separators, pack/archive status bar, main-window
      disabled toolbar items) — zero light artifacts, satisfying SC-001
      and visually confirming SC-002; document the pass (screenshots)
      against `temp/dark_find_window.png`.
- [X] T018 Regression passes: (a) Default theme side-by-side with the
      T001 baseline screenshot — zero visual differences (SC-003,
      FR-005); (b) Windows High Contrast on → Find window follows system
      colors exactly as before (FR-007); (c) 10 consecutive Default ↔
      Dark switches with a Find window open, one mid-search — no crash,
      corruption, or half-themed element (SC-004, FR-006).

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependencies.
- **Foundational (Phase 2)**: empty — 028 engine already present.
- **User Stories (Phases 3–5)**: all start after Phase 1; the three
  stories touch disjoint defects and are independently deliverable.
  Note the single-file serialization below.
- **Polish (Phase 6)**: T014/T015/T016 after all code tasks; T017/T018
  after T016.

### User Story Dependencies

- **US1 (P1)**: none. MVP on its own.
- **US2 (P2)**: independent of US1 (T007 edits the same `Edit` branch
  area as T003 — coordinate if done out of order, but neither requires
  the other to function).
- **US3 (P3)**: T011 pairs naturally with T004's owner-draw forwarding
  but is correct standalone (the handler also runs under native
  owner-draw); T013 depends on T012's helper.

### Within-story ordering

- US1: T002, T003, T004 sequentially (all in `src/themes.cpp`), then
  T005 [P] + T006 [P] (different files).
- US2: T007 first (in `src/themes.cpp`), then T008/T009/T010 in
  parallel (three different files).
- US3: T011 → T012 (same file), then T013.

### Parallel Opportunities

- After T004: T005 (`finddlg1.cpp`) ∥ T006 (`finddlg2.cpp`).
- After T007: T008 (`finddlg2.cpp`) ∥ T009 (`finddlg1.cpp`) ∥ T010
  (`toolbar2.cpp`).
- T014 (saltests) can run parallel to any Phase-6 formatting work.
- Cross-story: with two developers, US1's themes.cpp tasks and US3's
  finddlg1.cpp tasks (T011/T012) can proceed concurrently.

---

## Parallel Example: User Story 2

```text
# After T007 lands in src/themes.cpp, launch together:
Task: "T008 — SetTextColor for Found Items text in src/finddlg2.cpp"
Task: "T009 — header NM_CUSTOMDRAW label color in src/finddlg1.cpp"
Task: "T010 — dark single-pass disabled toolbar text in src/toolbar2.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1 only)

1. Phase 1 (T001 baseline), then T002–T006.
2. **STOP and VALIDATE**: quickstart state 1 — window opens with zero
   white artifacts except the US2 text items; light mode unchanged.
3. This already removes every defect the user's screenshot shows except
   the dark-on-dark texts.

### Incremental Delivery

1. US1 → validate → the three glaring "light" artifacts are gone.
2. US2 → validate → all text readable (screenshot defect list fully
   cleared).
3. US3 → validate → search/switch/regression states covered.
4. Polish → builds, tests, documented walkthrough → done.

---

## Notes

- Every paint-path task re-checks `IsDarkThemeActive()` per message —
  no task may cache the theme state (contract invariant; makes live
  switching and High Contrast free).
- Draw sites use `ThemeSysColor`/`ThemeSysColorBrush`/`ThemeDrawEdge`
  only — never raw `GetSysColor` (contract invariant 3).
- No `lang.rc`/template edits, no new files, no config changes.
- Commit after each task or logical group.
