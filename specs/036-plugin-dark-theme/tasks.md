# Tasks: Dark Theme for Plugin Windows and Dialogs

**Input**: Design documents from `/specs/036-plugin-dark-theme/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md

**Tests**: Not requested — verification is per-story run verification (quickstart.md) plus the per-plugin audit checklist (`audit.md`).

**Organization**: Foundational mechanism first (blocks everything), then user stories; US2's plugin sweep is split by mechanism kind (winliblt entry calls / raw dialog procs / top-level windows) rather than per-plugin, because the winliblt hook themes ~12 plugins centrally.

## Format: `[ID] [P?] [Story] Description`

---

## Phase 1: Setup

- [X] T001 Confirm a green incremental Debug x64 baseline (`build.cmd`) before interface changes; note core + plugin link OK

---

## Phase 2: Foundational (blocks all user stories)

- [X] T002 Append the 6 theme virtuals at the END of `CSalamanderGeneralAbstract` in `src/plugins/shared/spl_gen.h` with full doc comments per `contracts/plugin-theme-api.md`; bump `LAST_VERSION_OF_SALAMANDER` 104 → 105 in `src/plugins/shared/spl_vers.h` including the history row (feature 036, 6 appended methods)
- [X] T003 Implement the 6 methods on `CSalamanderGeneral` (`src/plugins.h:1871` declarations + definitions beside its existing methods) as one-line delegations to `src/themes.cpp` (`IsDarkThemeActive`, `ThemeSysColor`, `ThemeSysColorBrush`, `ThemeApplyToDialog`, `ThemeApplyToTopLevel`, `ThemeHandleCtlColor`); include `themes.h` where needed
- [X] T004 Add `SetupWinLibTheme(CSalamanderGeneralAbstract*)` to `src/plugins/shared/winliblt.h/.cpp` (module-global provider, `SetupWinLibHelp` pattern) and hook the central procs per `contracts/winliblt-theming.md`: `CDialog::CDialogProc` + `CPropSheetPage::CPropSheetPageProc` — WM_INITDIALOG → `ThemeApplyToDialog`, WM_CTLCOLOR* → `ThemeHandleCtlColor` (provider unset ⇒ byte-identical behavior)
- [X] T005 Build (`build.cmd`): core + all enabled plugins compile and link with the extended interface; zero warnings introduced in changed files

**Checkpoint**: Theme services reachable from every plugin; nothing visually changed yet (no plugin calls them).

---

## Phase 3: User Story 1 — SFTP plugin follows the Dark theme (P1) 🎯 MVP

**Goal**: Every SFTP surface dark; the named pain case fixed.

- [X] T006 [US1] Add the two theme touchpoints (WM_INITDIALOG → `ThemeApplyToDialog`; WM_CTLCOLOR* → `ThemeHandleCtlColor` via a small local helper) to all raw dialog procs in `src/plugins/sftp/dialogs.cpp` (Connect, HostKey, Password, Rename, Symlink, Chmod, OwnerGroup, Config, Resume) — SalamanderGeneral is the plugin's existing interface pointer
- [X] T007 [US1] Theme the modeless SFTP log window in `src/plugins/sftp/logs.cpp`: `ThemeApplyToTopLevel` at creation, chrome + log text colors via `GetThemeSysColor` (dark background, light text per clarification), edit/list child via `ThemeApplyToDialog`-equivalent child theming
- [X] T008 [US1] Build + run-verify (quickstart §Runtime 1): Dark theme → SFTP Connect dialog, Organize mode, Configuration dialog, log window all dark and readable (screenshots); Default theme → unchanged light look
**Checkpoint**: US1 = MVP delivered.

---

## Phase 4: User Story 2 — Every shipped plugin theme-consistent (P2)

**Goal**: The whole enabled plugin set (plugins.cfg) follows the theme.

- [X] T009 [P] [US2] Add `SetupWinLibTheme(SalamanderGeneral);` to the plugin entry of every winliblt-based enabled plugin: ftp, pictview, regedt, renamer, dbviewer, undelete, filecomp, 7zip, checksum, peviewer, uniso, mdview, folders, portables (each in its `SalamanderPluginEntry` after interfaces are obtained)
- [X] T010 [P] [US2] Add the two theme touchpoints to the raw dialog procs of `src/plugins/zip/dialogs.cpp`, `dialogs2.cpp`, `dialogs3.cpp` (~17 procs; shared local helper) — includes pack/unpack/SFX option dialogs
- [X] T011 [P] [US2] Add the two theme touchpoints to the raw dialog procs in `src/plugins/uncab/dialogs.cpp` (4 dialogs)
- [X] T012 [P] [US2] diskmap: `ThemeApplyToTopLevel` on the map frame window + chrome colors via `GetThemeSysColor` (custom-drawn map visualization content itself stays as designed); theme its config/about surfaces
- [X] T013 [US2] mdview viewer window (`src/plugins/mdview/viewer.cpp`): dark title bar + toolbar/status chrome; document rendering colors switch to dark (light text on dark background) when `IsDarkThemeActive()` — content-dark clarification
- [X] T014 [US2] dbviewer table window + filecomp compare panes: dark backgrounds/light text for the data/content areas via `GetThemeSysColor`/`IsDarkThemeActive` branch at their color-resolution points
- [X] T015 [US2] regedt value/view windows, renamer preview list, peviewer report window: dark content per clarification (winliblt covers their dialogs; this task is the non-dialog windows/content colors)
- [X] T016 [US2] pictview viewer frame: dark title bar + chrome (toolbar/status); image canvas and its background rendering NOT recolored; ftp's non-dialog windows (welcome message/log) themed dark
- [X] T017 [US2] Build (`build.cmd`) — full enabled-plugin set green; fix any fallout from T009–T016
- [X] T018 [US2] Runtime audit per quickstart §Runtime 2–3 with Dark active; record per-plugin results (surface → OK/N-A/issue) in `specs/036-plugin-dark-theme/audit.md`; fix issues found and re-audit until SC-001/SC-002 hold for all audited surfaces

**Checkpoint**: All enabled plugins consistent in Dark; audit evidence captured.

---

## Phase 5: User Story 3 — Theme switch without restart (P3)

- [X] T019 [US3] Run-verify switch semantics (quickstart §Runtime 4): plugin dialog open during Dark↔Default switch keeps consistent old look, no crash/mixed surface; reopened + newly opened plugin windows always match the active theme; record in audit.md

---

## Phase 6: Polish & Cross-Cutting

- [X] T020 [P] Default-theme regression pass (quickstart §Runtime 5): reopen all US1/US2 representative surfaces with Default active — zero visual change vs. pre-036 (SC-004)
- [X] T021 [P] Compatibility + consistency gates: repo grep confirms no plugin gained `ICC_STANDARD_CLASSES`/manifest; `spl_gen.h` docs match `contracts/plugin-theme-api.md`; `spl_vers.h` history row present; loader version-handshake path untouched for ≤104 (code inspection)
- [X] T022 Write `specs/036-plugin-dark-theme/validation-results.md` mapping SC-001…SC-005 to T008/T018/T019/T020/T021 evidence

---

## Dependencies & Execution Order

- Phase 2 (T002 → T003/T004 → T005) blocks everything; T003 and T004 can run in parallel after T002.
- US1 (T006 → T007 → T008) after T005.
- US2: T009–T016 mostly parallel (different plugins/files); T013–T016 touch content colors and may need per-plugin iteration; T017 gates T018.
- US3 (T019) after US2's build (needs themed plugins to observe).
- Polish last; T020/T021 parallel.

## Implementation Strategy

MVP = Phases 1–3 (mechanism + SFTP). Then the US2 sweep in mechanism-sized
slices with one consolidated build + audit loop, US3 switch verification,
and the polish gates. Commit per phase (repo convention).
