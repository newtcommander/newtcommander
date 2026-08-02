# Tasks: Hot Path Display Names and Custom Icons

**Input**: Design documents from `/specs/047-hot-path-names-icons/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md

**Tests**: Not requested; the project has no automated UI test infrastructure.
Each story phase ends with a manual checkpoint task driven by `quickstart.md`.

**Organization**: Tasks are grouped by user story. US1 (naming) is the MVP; US2
(icons) builds on the same foundation; US3 (compatibility) is validation of
behavior the foundational persistence layer implements by design.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 (naming), US2 (icons), US3 (compatibility)

## Phase 1: Setup

**Purpose**: Confirm a clean baseline before touching hot-path code

- [x] T001 Baseline build: run `build.cmd` (Debug x64) from repo root and confirm it succeeds unchanged; note the output tree location for later manual checkpoints

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Data model + persistence changes that both US1 and US2 require

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [x] T002 Extend the hot path model in `src/mainwnd.h`: add `int IconIndex` to `CHotPathItem` (init/free/copy paths); add `CHotPathItems::GetDisplayName()`/`GetDisplayNameLen()` (custom name if non-empty, else stored path with `$$` unescaped to `$`), `GetIconIndex()`/`SetIconIndex()` (clamped to `0..HOT_PATH_ICON_COUNT-1`); include `IconIndex` in `SwapItems()` and `Load(CHotPathItems&)`; keep `Set()`/`SetPath()`/`SetVisible()` signatures unchanged (per data-model.md)
- [x] T003 Update registry persistence in `src/mainwnd1.cpp` per contracts/registry-format.md: Save writes `Name` = effective label (custom name else user-visible path text), writes `Icon` (REG_DWORD) only when ≠ 0 (delete value when 0), deletes the subkey when `Path` is empty; Load reads `Icon` with missing/out-of-range → 0, and classifies `Name` byte-identical to the user-visible path text as unnamed (in-memory `Name = ""`); change `Load1_52` to leave `Name` empty instead of duplicating the path

**Checkpoint**: Project compiles; existing configs load and re-save byte-compatibly (labels unchanged)

---

## Phase 3: User Story 1 - Show a short custom name instead of a long path (Priority: P1) 🎯 MVP

**Goal**: Optional Name field in Hot Paths settings; every surface labels an
entry with the name when set, else the path; quick-assign stops auto-filling
the name.

**Independent Test**: quickstart.md § US1 — assign a hot path, verify path
label; set a name in settings, verify bar + Alt+F1/F2 + menus show it without
restart; clear it, verify fallback; name-without-path rejected.

### Implementation for User Story 1

- [x] T004 [P] [US1] `src/toolbar7.cpp` `CHotPathsBar::CreateButtons`: include slots with non-empty **Path** (was: Name), label buttons via `GetDisplayName` (keep 200-char truncation + `DuplicateAmpersands`); tooltip continues to show the path (contracts/display-rules.md)
- [x] T005 [P] [US1] `src/drivelst.cpp` `CDrivesList::BuildData`: membership condition becomes `Visible && Path non-empty` (drop the Name requirement); item text uses `GetDisplayName` with the existing `"%d\t%s"` / `"\t%s"` accelerator format
- [x] T006 [US1] `src/mainwnd1.cpp` `CHotPathItems::FillHotPathsMenu`: "assigned" test becomes Path non-empty; item text uses `GetDisplayName` (accelerator columns `Ctrl+<n>` / `Ctrl+Shift+<n>` unchanged); the assign-mode empty-slot listing keys on empty Path
- [x] T007 [US1] Quick-assign produces unnamed entries (FR-011): `src/fileswn1.cpp` `SetUnescapedHotPath`/`SetUnescapedHotPathToEmptyPos` and the direct-write branch in `src/mainwnd1.cpp` (~1242-1272) set only Path (Name empty, IconIndex 0); `GetUnassignedHotPathIndex` in `src/mainwnd1.cpp` searches for empty **Path**; the `HotPathSetBufferName` auto-config handoff carries an empty name with the path
- [x] T008 [P] [US1] `src/jumplist.cpp`: jump list item titles use `GetDisplayName` (membership already Visible-based; target path expansion unchanged)
- [x] T009 [US1] `src/lang/lang.rh` + `src/lang/lang.rc`: add `IDC_HOTPATH_NAME` edit box with a "&Name:" static to `IDD_CFGPAGE_HOTPATH` (above the existing Path edit; DIALOGEX/DS_SHELLFONT layout preserved per constitution VI); add any new string IDs (e.g. validation message for name-without-path if no existing string fits)
- [x] T010 [US1] `src/cfgdlg.h` + `src/dialogs4.cpp` `CCfgPageHotPath`: wire the Name edit per selected row (trim via `CleanName`, empty = unnamed); list rows render `GetDisplayName`; in-place label edit (F2) now edits the custom name and an empty result is valid; validation rejects name-without-path on page confirm (FR-004); Delete resets the whole slot; auto-config arrival (`HotPathSetBuffer*`) pre-fills Path and leaves Name empty
- [ ] T011 [US1] Checkpoint: run quickstart.md § US1 scenarios 1–7 end-to-end on a Debug build; verify SC-001 and SC-003 (all surfaces agree, no restart needed) — *deferred to a manual GUI session (see Implementation record below)*

**Checkpoint**: Naming works end-to-end; icons still the shared default everywhere

---

## Phase 4: User Story 2 - Distinguish hot paths with a chosen icon (Priority: P2)

**Goal**: Per-entry icon from a 10-item gallery (default = current shell32 icon,
9 shipped color variants), selectable in settings, rendered on every surface.

**Independent Test**: quickstart.md § US2 — pick the red variant for one entry;
bar, Alt+F1/F2, hot path menus and the settings list show it; others keep the
default; delete resets it.

### Implementation for User Story 2

- [x] T012 [P] [US2] Create original master artwork `tools/brand/hotpath-master.png`: bookmark/star motif, midtone fill + contrasting outline, legible at 16 px on light and dark backgrounds; NOT derived from Microsoft artwork (contracts/icon-set.md)
- [x] T013 [US2] Extend `tools/brand/gen_icons.py` with a hot-path section: 9-entry hue/tint table (red, orange, yellow, green, teal, blue, purple, pink, gray) generating `src/res/hotpath1.ico` … `hotpath9.ico` with 16/20/24/32 px frames; document regeneration in `tools/brand/README.md`; run the script and commit the nine `.ico` files
- [x] T014 [US2] Register icon resources: `IDI_HOTPATH_1..9` IDs in `src/salamand.rh`, `ICON "res\\hotpathN.ico"` statements in `src/salamand.rc2` (existing convention at lines 19-31)
- [x] T015 [US2] Gallery runtime lifecycle: `HOT_PATH_ICON_COUNT` (10) + `extern HICON HHotPathIcons[]` in `src/consts.h`; in `src/salamdr1.cpp` load indices 1–9 via `LoadImage` at `IconSizes[ICONSIZE_16]` beside the `HFavoritIcon` load (~line 2356), alias index 0 to `HFavoritIcon` (never double-destroyed), destroy (~2691) and reload on color/DPI change so the existing Hot Path Bar rebuild picks up fresh handles (FR-014)
- [x] T016 [P] [US2] `src/toolbar7.cpp` `CreateButtons`: per-button `tii.HIcon = HHotPathIcons[GetIconIndex(i)]` (replaces unconditional `HFavoritIcon`)
- [x] T017 [P] [US2] `src/drivelst.cpp` `BuildData`: per-item `drv.HIcon = HHotPathIcons[GetIconIndex(i)]` with `DestroyIcon = FALSE` (replaces block-wide shared icon)
- [x] T018 [P] [US2] `src/mainwnd1.cpp` `FillHotPathsMenu`: assigned items get `mii.HIcon = HHotPathIcons[GetIconIndex(i)]`; assign-mode "empty"/pseudo items keep NULL
- [x] T019 [US2] `src/lang/lang.rh` + `src/lang/lang.rc`: add `IDC_HOTPATH_ICON` combo (`CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED`) with an "&Icon:" static to `IDD_CFGPAGE_HOTPATH`; swatch-only items — no per-color strings
- [x] T020 [US2] `src/cfgdlg.h` + `src/dialogs4.cpp` `CCfgPageHotPath`: populate the combo with the 10 gallery entries, `WM_DRAWITEM` renders centered icon swatches, selection maps to `SetIconIndex` per row; build an `LVSIL_SMALL` imagelist from `HHotPathIcons` at dialog init and show each row's icon; Delete resets the icon to 0 (FR-013)
- [ ] T021 [US2] Checkpoint: run quickstart.md § US2 scenarios 1–4; verify SC-002 (≤ 4 interactions, immediate effect) and swatch legibility in the combo — *deferred to a manual GUI session; variant legibility pre-checked on renders (light + dark), see Implementation record*

**Checkpoint**: Naming and icons both work; all surfaces render per-entry icons

---

## Phase 5: User Story 3 - Existing setups and quick assignment stay familiar (Priority: P3)

**Goal**: Prove the compatibility behavior the foundational persistence layer
implements: upgrades are invisible, downgrades lose nothing, quick-assign flows
unchanged in feel.

**Independent Test**: quickstart.md § US3 — seed a pre-047-style registry entry
(`Name == Path`), verify unnamed classification and identical display; verify
save round-trip and reorder behavior.

### Implementation for User Story 3

- [ ] T022 [US3] Upgrade simulation per quickstart.md § US3 steps 1–4: seed `HKCU\Software\Tandem Commander\0.1\Hot Paths\7` with `Name == Path`, start the app, verify path label everywhere + empty Name field + default icon (FR-010); edit the path and verify the label follows (clarification #2); exit and `reg query` to confirm the write rules of contracts/registry-format.md (effective label in `Name`, `Icon` absent when default)
- [ ] T023 [US3] Quick-assign verification: Ctrl+Shift+5, Ctrl+Shift+= (first free slot from index 10), directory-line "Assign Hot Path", and the `HotPathAutoConfig` auto-open flow — each yields an entry displaying as its path, empty Name in settings, default icon (FR-011); Ctrl+digit / Ctrl+Alt+digit / Shift+digit navigation unchanged
- [ ] T024 [US3] Reorder & persistence verification: Move Up/Down keeps name+path+visibility+icon together and remaps Ctrl+digit by position (FR-012); restart the app and verify names and icons persist (FR-009); verify a `Visible`-unchecked named entry still shows name+icon on the bar and Go menu but not in Alt+F1/F2

**Checkpoint**: All three stories verified independently

---

## Phase 6: Polish & Cross-Cutting Concerns

- [x] T025 Run clang-format on all touched sources (`src/mainwnd.h`, `src/mainwnd1.cpp`, `src/fileswn1.cpp`, `src/toolbar7.cpp`, `src/drivelst.cpp`, `src/jumplist.cpp`, `src/dialogs4.cpp`, `src/cfgdlg.h`, `src/salamdr1.cpp`, `src/consts.h`) per repo config; keep UTF-8-BOM encoding (note: `normalize.ps1` needs pwsh7 which is unavailable — invoke `clang-format` directly)
- [x] T026 Translation follow-up for the `src/lang/lang.rc` changes (feature-038 tooling): regenerate the English `.slt` template, run `translate.merge` for the enabled languages in `translations/languages.cfg`, machine-translate the new "&Name:"/"&Icon:" labels (and validation string if added); build one non-English language and verify the Hot Paths page renders
- [ ] T027 Full quickstart.md non-functional pass: DPI 150 %/200 % re-render (FR-014), light/dark theme legibility of all 10 swatches (SC-005), jump list labeling, persistence across restart (FR-009)
- [x] T028 Final `build.cmd full` sanity: complete build succeeds, `src/res/hotpath*.ico` and regenerated `.slg` files present in the output tree, no plugin project affected

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: none
- **Foundational (Phase 2)**: after Setup — **blocks all stories** (T002 → T003)
- **US1 (Phase 3)**: after Phase 2
- **US2 (Phase 4)**: after Phase 2; T012–T015 (assets/runtime) are independent of US1, but T016–T020 edit the same functions/files US1 touches (`toolbar7.cpp`, `drivelst.cpp`, `mainwnd1.cpp`, `lang.rc`, `dialogs4.cpp`) — run the phases sequentially (P1 → P2) in a single-developer flow
- **US3 (Phase 5)**: validation only — after US1 (T022–T023) resp. US1+US2 (T024 icon aspects)
- **Polish (Phase 6)**: after all desired stories; T026 after T009+T019 (all lang.rc changes settled)

### Within-story ordering

- US1: T004/T005/T006/T008 in any order after Phase 2 ([P] where marked); T007 after T006 (same file); T009 → T010; T011 last
- US2: T012 → T013 → T014 → T015 → {T016, T017, T018 in parallel} and T019 → T020; T021 last
- US3: T022 → T023 → T024 (manual, single app instance)

### Parallel Opportunities

- After Phase 2: T004 (`toolbar7.cpp`), T005 (`drivelst.cpp`), T008 (`jumplist.cpp`) — different files, no shared state
- US2 asset track (T012–T013, `tools/brand/`) can start any time after planning — it touches no C++ source and conflicts with nothing
- After T015: T016/T017/T018 in parallel (three different files)

## Parallel Example: User Story 1

```text
# After Phase 2 completes, launch together:
Task: "T004 CreateButtons label+membership in src/toolbar7.cpp"
Task: "T005 BuildData label+membership in src/drivelst.cpp"
Task: "T008 Jump list titles in src/jumplist.cpp"
# Then sequentially: T006 → T007 (both src/mainwnd1.cpp), T009 → T010 (dialog)
```

## Implementation Strategy

**MVP first (US1 only)**: Phases 1–3 deliver the naming feature completely —
this is a shippable increment (icons stay as today). Stop at T011 and validate.

**Incremental delivery**: Add Phase 4 (icons) → validate at T021. Phase 5 is a
compatibility gate before merging; Phase 6 (formatting, translations, full
build) finishes the branch. Commit after each task or logical group.

---

## Implementation record (2026-08-02)

All code, asset, formatting and translation tasks are complete; `build.cmd`
and `build.cmd full` both succeed (0 errors; 180 language modules version-check
OK against the re-merged `.slt` sources).

**Deviations from task descriptions** (all behavior-preserving):

- **T007**: `src/fileswn1.cpp` needed no change — its functions only forward to
  `CMainWindow`; the whole semantic flip lives in `src/mainwnd1.cpp`.
  Additionally, quick-assign and the auto-config `EditMode` handoff also reset
  `IconIndex` to 0, so re-assigning an occupied slot yields the default icon
  (FR-011 read literally).
- **T009**: the validation message was added as `IDS_HOTPATH_NAMEWITHOUTPATH`
  (10668) in `src/texts.rh2` + `src/lang/texts.rc2` (the app's string tables do
  not live in `lang.rh`/`lang.rc` STRINGTABLEs); the new static labels use the
  shared `IDC_STATIC_4`/`IDC_STATIC_5` IDs.
- **T014**: the `IDI_HOTPATH_1..9` IDs live in `src/resource.rh2` (1011–1019),
  where the application's `IDI_*` constants actually reside — not
  `salamand.rh`.
- **T019**: the icon combo sits on the **Name row** (right side) instead of its
  own row — an owner-draw combo's closed height would have collided with the
  hint text below; the page keeps its 299×231 DLU size. Item height is set
  explicitly via `CB_SETITEMHEIGHT` because `WM_MEASUREITEM` precedes page
  attachment.
- **T025**: `normalize.ps1` requires pwsh7 (unavailable); `clang-format` from
  VS2022 LLVM was invoked directly on the touched files.
- **T026**: executed fully — `build_langs.cmd --export-templates`, then
  `translate.merge --all` (DeepL, 10,792 characters); coverage report shows
  **0 English fallbacks and 0 validation failures** across all 8 enabled
  languages; the follow-up `build.cmd full` built and version-checked all 180
  language modules.

**Deferred to a manual GUI session** (cannot be exercised from this
non-interactive environment): T011, T021, T022, T023, T024, T027 — the
step-by-step scripts are in `quickstart.md` (§ US1, § US2, § US3,
§ Non-functional checks). Icon legibility (part of T021/T027) was pre-checked
on rendered contact sheets of all variants at 16/32 px on white and dark-navy
backgrounds.
