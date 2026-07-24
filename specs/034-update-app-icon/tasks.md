# Tasks: Update Application Icon to Revised Artwork

**Input**: Design documents from `/specs/034-update-app-icon/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, quickstart.md

**Tests**: Not requested in the spec — no test tasks. Verification is done
per story via structural ICO checks, build, and visual inspection
(quickstart.md), plus a final validation-results record (032/033 convention).

**Organization**: Tasks are grouped by user story so each story is an
independently verifiable increment.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 = OS shell icon, US2 = About/splash mark, US3 = RGB
  window-icon variants, US4 = companion programs

## Phase 1: Setup (adopt the revised artwork into the repo)

**Purpose**: `temp/` is gitignored — the approved revision must live in
`tools/brand/` before anything can consume it.

- [X] T001 [P] Replace master icon: copy `temp/icon/newt-commander-icon.svg`
      over `tools/brand/newt-commander-icon.svg`
- [X] T002 [P] Replace raster set: copy all nine
      `temp/icon/png/newt-commander-icon-{16,24,32,48,64,128,256,512,1024}.png`
      over `tools/brand/png/`; sanity-check each is square RGBA at its
      nominal size with fully transparent corners (tile-less silhouette)

---

## Phase 2: Foundational (regenerate the shipped ICOs — blocks US1, US3, US4)

**Purpose**: `tools/brand/gen_icons.py` is already the PNG→ICO packer from
033 (research R2) — one run over the new rasters regenerates all six shipped
ICOs consumed by US1/US3/US4. US2 does not depend on this phase.

- [X] T003 Refresh doc text in `tools/brand/gen_icons.py` (module docstring
      still describes the 033 "folder-tile" design): describe the revised
      tile-less artwork and reference feature 034; no logic changes
- [X] T004 Run `python tools/brand/gen_icons.py` to regenerate
      `src/res/salamand.ico`, `src/res/sal_r.ico`, `sal_g.ico`, `sal_b.ico`,
      `src/salmon/res/salmon.ico`, `src/setup/res/setup.ico`,
      `src/setup/remove/icon1.ico`; then `python tools/brand/gen_icons.py
      --verify` must pass for all six

**Checkpoint**: all shipped ICOs carry the revised rasters — story
verification can begin

---

## Phase 3: User Story 1 — Revised icon in the OS shell (Priority: P1) 🎯 MVP

**Goal**: `newtcommander.exe` carries the revised icon in Explorer, taskbar,
window caption, and Alt+Tab, with no tile remnant at any size.

**Independent Test**: build, then check the exe icon in Explorer at all
four view sizes and the running app's taskbar/caption/Alt+Tab icons.

- [X] T005 [US1] Structural + transparency check of `src/res/salamand.ico`:
      beyond `--verify`, spot-check every frame keeps fully transparent
      corner pixels (no square tile remnant — SC-001) with a small Pillow
      script against the packed entries
- [X] T006 [US1] Verify US1: `build.cmd` succeeds; visually confirm the
      revised icon on `newtcommander.exe` in Explorer (four view sizes),
      taskbar, window caption, and Alt+Tab (fresh output path — icon cache
      note in quickstart.md)

**Checkpoint**: MVP — the OS-shell identity shows the revised artwork

---

## Phase 4: User Story 2 — About dialog & splash screen mark (Priority: P2)

**Goal**: About and splash show the revised tile-less mark, faithful to the
master artwork, legible in light and dark themes. No C++ changes (asset-only;
`logo.cpp` scales the square SVG, splash background is always brand navy).

**Independent Test**: launch with splash enabled, open Help → About in
light and dark theme, compare the mark against
`tools/brand/png/newt-commander-icon-256.png`.

- [X] T007 [P] [US2] Re-author nanosvg-safe `tools/brand/logo.svg` from the
      revised master (research R3): keep the folder group incl.
      `translate(128 133) scale(1.28 1.38)` and `rotate(±4°)` transforms;
      drop `feDropShadow`; delete the 033 tile rects (no background); set
      `width="256" height="256"`; convert the four folder gradients to
      `gradientUnits="userSpaceOnUse"` with explicit local coordinates;
      element whitelist per data-model.md validation rules
- [X] T008 [US2] Copy `tools/brand/logo.svg` over `src/res/logo.svg`
      (`IDB_LOGO_HAND` RCDATA — file name and ID unchanged)
- [X] T009 [US2] Verify US2: nanosvg-render `logo.svg` at 256 px and
      pixel-diff against `temp/icon/png/newt-commander-icon-256.png`
      (shadow-masked, 033 method); `build.cmd`; capture splash and Help →
      About screenshots in light and dark themes (headless `-l`/`-r` +
      `WM_COMMAND` smoke) and confirm legibility without an own background
      tile

**Checkpoint**: in-app branding surfaces match the revised identity

---

## Phase 5: User Story 3 — Red/green/blue window-icon variants (Priority: P3)

**Goal**: Configuration → Main Window keeps offering default/red/green/blue,
now as recolored variants of the revised design.

**Independent Test**: cycle all four options in Configuration → Main Window
and check window + taskbar icon after each.

- [X] T010 [US3] Verify US3: inspect the regenerated `src/res/sal_r.ico`,
      `sal_g.ico`, `sal_b.ico` frames at 16+32 px — folder recolored
      cleanly, papers/cream pill stay neutral (033 hue-band tuning expected
      to transfer, research R4; re-tune only if strays appear); in the
      running app cycle Configuration → Main Window icon options and
      confirm all four variants apply and are distinguishable at taskbar
      size (screenshot)

**Checkpoint**: existing configuration feature preserved with revised artwork

---

## Phase 6: User Story 4 — Companion programs (Priority: P4)

**Goal**: crash reporter, installer, and uninstaller follow the revision —
no surface keeps the 033 tile icon.

**Independent Test**: inspect the built `salmon.exe`, `setup.exe`, and
`remove.exe` icons in Explorer.

- [X] T011 [US4] Verify US4: `--verify` passes for
      `src/salmon/res/salmon.ico`, `src/setup/res/setup.ico`,
      `src/setup/remove/icon1.ico`; build the salmon and setup projects and
      confirm the revised icon on their executables in Explorer

**Checkpoint**: no surface anywhere still ships the 033 tile artwork

---

## Phase 7: Polish & Cross-Cutting

- [X] T012 [P] Update `tools/brand/README.md`: revised artwork description
      (tile-less folder), palette section (drop tile edge/radial/sheen
      colors), logo.svg construction notes (no tile reconstruction), keep
      pipeline/size/hue-remap tables accurate
- [X] T013 Full validation sweep per quickstart.md: regenerate everything
      idempotently (SC-005: second `gen_icons.py` run produces byte-identical
      ICOs), `--verify` all six ICOs, `build.cmd` clean, SC-001..SC-006
      evidence (screenshots + checks) recorded in
      `specs/034-update-app-icon/validation-results.md`; audit that no
      enumerated surface still shows the 033 tile icon or older artwork
      (SC-003, FR-011)

---

## Dependencies & Execution Order

- **Phase 1 (T001, T002)**: no dependencies; T001 ∥ T002
- **Phase 2 (T003→T004)**: needs T002; same file/tool → sequential
- **US1 (T005→T006)**: needs Phase 2
- **US2 (T007→T008→T009)**: needs only T001 — can run in parallel with
  Phase 2 / US1 / US3 / US4 (different files)
- **US3 (T010)**: needs Phase 2
- **US4 (T011)**: needs Phase 2; independent of US1/US2/US3 outputs
- **Polish (T012 ∥, T013 last)**: T013 needs all stories complete

### Parallel opportunities

- T001 ∥ T002 (different targets)
- US2 track (T007–T009) ∥ the packer track (T003–T006, T010–T011)
- T012 ∥ any story verification
- One packer run (T004) produces all six ICOs — US1/US3/US4 phases are pure
  verification and can proceed in any order after it

## Implementation Strategy

MVP = Phase 1 + Phase 2 + US1 (T001–T006): the OS-shell identity switches to
the revision. Then incrementally US2 (in-app branding), US3 (variants), US4
(companions), each independently verifiable; finish with the polish sweep.
Commit after each story checkpoint.
