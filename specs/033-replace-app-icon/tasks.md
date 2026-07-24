# Tasks: Replace Application Icon

**Input**: Design documents from `/specs/033-replace-app-icon/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, quickstart.md

**Tests**: Not requested in the spec — no test tasks. Verification is done
per story via structural ICO checks, build, and visual inspection
(quickstart.md), plus a final validation-results record (032 convention).

**Organization**: Tasks are grouped by user story so each story is an
independently verifiable increment.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 = OS shell icon, US2 = About/splash tile, US3 = RGB
  window-icon variants, US4 = companion programs

## Phase 1: Setup (adopt the delivered artwork into the repo)

**Purpose**: `temp/` is gitignored — the approved artwork must live in
`tools/brand/` before anything can consume it.

- [ ] T001 [P] Replace master icon: copy `temp/icon/newt-commander-icon.svg`
      over `tools/brand/newt-commander-icon.svg`
- [ ] T002 [P] Adopt raster set: create `tools/brand/png/` and copy all nine
      `temp/icon/png/newt-commander-icon-{16,24,32,48,64,128,256,512,1024}.png`
      into it; sanity-check each is square RGBA at its nominal size

---

## Phase 2: Foundational (generator rewrite — blocks US1, US3, US4)

**Purpose**: `tools/brand/gen_icons.py` currently redraws the OLD design
procedurally; it must become a PNG→ICO packer before any ICO can be
regenerated (research R1). US2 does not depend on this phase.

- [ ] T003 Rewrite `tools/brand/gen_icons.py`: delete the procedural
      "Split Disc" renderer (draw_icon/variants/palette constants), add
      loading of `tools/brand/png/newt-commander-icon-<size>.png` sources,
      keep the `write_ico` packer convention (32-bpp; BMP entries ≤ 64 px,
      PNG entries ≥ 128 px); paths default relative to the script so the
      documented invocation is `python tools\brand\gen_icons.py`
- [ ] T004 Add hue-remap recolor helper to `tools/brand/gen_icons.py`:
      pixels with HSV hue in the orange band (≈15°–50°) AND saturation
      above ≈0.45 get hue replaced by a target (red/green/blue), keeping
      S/V and alpha (research R3; exact band/threshold tuned on the
      16/32 px sources)
- [ ] T005 Add `--verify` mode to `tools/brand/gen_icons.py`: structurally
      check every emitted ICO (entry count, exact dimensions, 32-bpp,
      BMP/PNG encoding rule) against the data-model table and exit non-zero
      on mismatch

**Checkpoint**: packer runs and verifies — ICO regeneration can begin

---

## Phase 3: User Story 1 — New icon in the OS shell (Priority: P1) 🎯 MVP

**Goal**: `newtcommander.exe` carries the new icon in Explorer, taskbar,
window caption, and Alt+Tab.

**Independent Test**: build, then check the exe icon in Explorer at all
four view sizes and the running app's taskbar/caption/Alt+Tab icons.

- [ ] T006 [US1] Emit `src/res/salamand.ico` (16,24,32,48,64,128,256 px,
      from the raster set verbatim) in `tools/brand/gen_icons.py` main()
      and regenerate the file
- [ ] T007 [US1] Verify US1: `python tools\brand\gen_icons.py --verify`
      passes for salamand.ico; `build.cmd` succeeds; visually confirm the
      new icon on `newtcommander.exe` in Explorer, taskbar, window caption,
      and Alt+Tab (fresh output path — icon cache note in quickstart.md)

**Checkpoint**: MVP — the product identity is switched in the OS shell

---

## Phase 4: User Story 2 — About dialog & splash screen tile (Priority: P2)

**Goal**: About and splash show the new icon tile, faithful to the master
artwork, in light and dark themes. No C++ changes (asset-only, plan
confirmed `logo.cpp` scales the square tile by aspect).

**Independent Test**: launch with splash enabled, open Help → About in
light and dark theme, compare the tile against
`tools/brand/png/newt-commander-icon-256.png`.

- [ ] T008 [US2] Hand-author nanosvg-safe `tools/brand/logo.svg` from the
      new master: no `filter`/`feDropShadow`, no `clip-path`/`mask`; tile
      built from plain rounded rects (edge + inset radial-gradient fill);
      keep linear/radial gradients and the `rotate(±4°)` document
      transforms; element whitelist per data-model.md validation rules
- [ ] T009 [US2] Copy `tools/brand/logo.svg` over `src/res/logo.svg`
      (`IDB_LOGO_HAND` RCDATA — file name and ID unchanged)
- [ ] T010 [US2] Verify US2: `build.cmd`; capture splash screen and Help →
      About screenshots in light and dark themes (headless `-l`/`-r` +
      `WM_COMMAND` smoke); side-by-side compare with the 256 px master
      render (soft shadow absence is acceptable per spec edge case)

**Checkpoint**: in-app branding surfaces match the new identity

---

## Phase 5: User Story 3 — Red/green/blue window-icon variants (Priority: P3)

**Goal**: Configuration → Main Window keeps offering default/red/green/blue,
now as recolored variants of the new design.

**Independent Test**: cycle all four options in Configuration → Main Window
and check window + taskbar icon after each.

- [ ] T011 [US3] Emit `src/res/sal_r.ico`, `sal_g.ico`, `sal_b.ico`
      (16+32 px) in `tools/brand/gen_icons.py` main() using the T004
      hue-remap (red ≈0°, green ≈130°, blue ≈215°); regenerate the three
      files and tune band/threshold until the folder recolors cleanly while
      papers/cream pill stay neutral at both sizes
- [ ] T012 [US3] Verify US3: `--verify` passes for sal_r/g/b.ico; in the
      running app cycle Configuration → Main Window icon options and
      confirm all four variants apply and are distinguishable at taskbar
      size (screenshot)

**Checkpoint**: existing configuration feature preserved with new artwork

---

## Phase 6: User Story 4 — Companion programs (Priority: P4)

**Goal**: crash reporter, installer, and uninstaller stop shipping the
original Open Salamander icon.

**Independent Test**: inspect the built `salmon.exe`, `setup.exe`, and
`remove.exe` icons in Explorer.

- [ ] T013 [US4] Emit `src/salmon/res/salmon.ico`,
      `src/setup/res/setup.ico`, and `src/setup/remove/icon1.ico` (full
      16–256 px set each, research R4) in `tools/brand/gen_icons.py`
      main() and regenerate the three files
- [ ] T014 [US4] Verify US4: `--verify` passes for all three; build the
      salmon and setup projects and confirm the new icon on their
      executables in Explorer

**Checkpoint**: no surface anywhere still ships pre-rebrand artwork

---

## Phase 7: Polish & Cross-Cutting

- [ ] T015 [P] Update `tools/brand/README.md`: new artwork description,
      PNG source-of-truth pipeline (packer, not rasterizer), per-file size
      table, hue-remap variants, new palette; note that `logo.svg` is
      hand-maintained and `--verify` exists
- [ ] T016 Full validation sweep per quickstart.md: regenerate everything
      idempotently (SC-005), `--verify` all six ICOs, `build.cmd` clean,
      SC-001..SC-006 evidence (screenshots + checks) recorded in
      `specs/033-replace-app-icon/validation-results.md`; audit that no
      old "Split Disc" or Open Salamander icon remains on any enumerated
      surface (SC-003, FR-011)

---

## Dependencies & Execution Order

- **Phase 1 (T001, T002)**: no dependencies; T001 ∥ T002
- **Phase 2 (T003→T004→T005)**: needs T002; same file → sequential
- **US1 (T006→T007)**: needs Phase 2
- **US2 (T008→T009→T010)**: needs only T001 — can run in parallel with
  Phase 2 / US1 / US3 / US4 (different files)
- **US3 (T011→T012)**: needs Phase 2 (specifically T004)
- **US4 (T013→T014)**: needs Phase 2; independent of US1/US2/US3 outputs
- **Polish (T015 ∥, T016 last)**: T016 needs all stories complete

### Parallel opportunities

- T001 ∥ T002 (different targets)
- US2 track (T008–T010) ∥ the whole packer track (T003–T007, T011–T014)
- T015 ∥ any story verification
- ICO emission tasks T006/T011/T013 touch the same script → sequential by
  design (each extends main() and regenerates)

## Implementation Strategy

MVP = Phase 1 + Phase 2 + US1 (T001–T007): the OS-shell identity switches.
Then incrementally US2 (in-app branding), US3 (variants), US4 (companions),
each independently verifiable; finish with the polish sweep. Commit after
each story checkpoint.
