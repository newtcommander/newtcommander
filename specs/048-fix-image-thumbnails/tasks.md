# Tasks: Restore Image Thumbnails in Thumbnail View

**Input**: Design documents from `/specs/048-fix-image-thumbnails/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md,
contracts/wicsaveimage-raw-subset.md, quickstart.md

**Tests**: No automated test harness exists for this codebase; validation
tasks execute the manual scenarios from quickstart.md (S1–S8), which map
to the spec's acceptance scenarios.

**Organization**: Grouped by user story. The code change is deliberately
small (2 files); the story phases share one engine function, so US2/US3
phases are dominated by their independent validation criteria.

## Format: `[ID] [P?] [Story] Description`

## Phase 1: Setup

**Purpose**: Confirm a clean baseline so any breakage is attributable to
this feature.

- [X] T001 Verify baseline: run `build.cmd` (Debug x64 incremental) from
      repo root and confirm it succeeds before touching any source file

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The acceptance predicate that makes "no behavior change
outside thumbnails" structural — required before any story's export code.

- [X] T002 Implement the WicSaveImage acceptance predicate and guards in
      `src/plugins/pictview/wicengine.cpp` per
      `contracts/wicsaveimage-raw-subset.md`: validate handle; require
      `PVSF_USERDEFINED_OUTPUT` + non-NULL `WriteFunc`, `PVF_RAW`,
      `PV_COLOR_TC32`, `Width/Height == 0`, `CropWidth/CropHeight == 0`;
      accept-and-ignore `PVSF_SUPERFAST`; accept `PVSF_FLIP_VERT`; return
      `PVC_UNSUP_OUT_PARAMS` for everything else (unchanged behavior for
      Save As / print / batch JPEG thumbnails)

**Checkpoint**: Out-of-subset callers provably keep today's behavior.

---

## Phase 3: User Story 1 - Image previews appear in thumbnail view (Priority: P1) 🎯 MVP

**Goal**: Alt+5 shows actual image content for every WIC-supported
format again.

**Independent Test**: quickstart.md S1 — a folder of JPEG/PNG/GIF/BMP/
TIFF/ICO files shows content previews in thumbnail view.

### Implementation for User Story 1

- [X] T003 [US1] Implement the in-subset export path in
      `src/plugins/pictview/wicengine.cpp` (`WicSaveImage`): clamp
      `ImageIndex` (<0 → 0), decode via existing `DecodeFrame(img, frame,
      Progress, AppSpecific)` (reuses BkColor compositing + cancel
      polarity), then stream `Height` rows of `Width * 4` bytes from
      `DibBits` to `pSii->WriteFunc` in whole-row batches (~256 KB
      target, ≥1 row), top-down by default, bottom-up when
      `PVSF_FLIP_VERT`; short write from WriteFunc → stop and return
      `PVC_OK` (research.md D3); call `Progress(pct)` between batches,
      stop returning `PVC_OK` when it returns TRUE (cancel); map decode
      failures to their real `PVC_*` codes
- [X] T004 [US1] Build: run `build.cmd` from repo root; resolve any
      compile/link issues in `src/plugins/pictview/wicengine.cpp`
- [X] T005 [US1] Validate quickstart S1 (all formats incl. transparent
      PNG compositing and animated GIF frame 0), S4 (both panels +
      Alt+3/Alt+5 switching) and S5 (thumbnail size change re-renders)
      on the built `tandemcommander.exe`

**Checkpoint**: Previews render — MVP delivered.

---

## Phase 4: User Story 2 - Graceful fallback for non-previewable files (Priority: P2)

**Goal**: Non-images, corrupt images and unsupported formats silently
keep their standard icons; loader failures become diagnosable.

**Independent Test**: quickstart.md S2 — mixed folder with corrupt/fake/
empty files shows icons for them, zero error dialogs.

### Implementation for User Story 2

- [X] T006 [P] [US2] Fix the silent-failure contract in
      `src/plugins/pictview/thumbs.cpp`:
      `CPluginInterfaceForThumbLoader::LoadThumbnail` ends with
      `return code == PVC_OK;` instead of `return TRUE;` (line ~1023),
      so open/decode failures fall through the loader chain per
      `spl_thum.h` instead of silently ending it
- [X] T007 [US2] Build (`build.cmd`) and validate quickstart S2 (corrupt
      JPEG, text-as-.jpg, zero-byte PNG, plain .txt → icons, no dialogs)
      and S7 (viewer unchanged; Save As still unavailable; print dialog
      preview behavior unchanged)

**Checkpoint**: Failures are silent for the user, honest for the core.

---

## Phase 5: User Story 3 - Panel stays responsive while thumbnails load (Priority: P3)

**Goal**: Large folders fill in progressively; navigation never blocks.

**Independent Test**: quickstart.md S3 — 100+ photo folder is
immediately navigable, leaving mid-load is instant.

### Implementation for User Story 3

- [X] T008 [US3] Validate quickstart S3 with 100+ high-resolution photos:
      immediate scroll/keyboard response after Alt+5, progressive
      thumbnail fill, instant folder change mid-load (cancellation via
      chunked `WriteFunc`/`ProcessBuffer` returns), no stale thumbnails
      in the next folder; if any single `WriteFunc` batch measurably
      stalls cancellation, reduce the batch size constant in
      `src/plugins/pictview/wicengine.cpp` and rebuild

**Checkpoint**: All three stories independently verified.

---

## Phase 6: Polish & Cross-Cutting Concerns

- [X] T009 [P] Format touched files with clang-format (repo config):
      `src/plugins/pictview/wicengine.cpp`,
      `src/plugins/pictview/thumbs.cpp`; verify UTF-8-BOM preserved
- [X] T010 Record results in
      `specs/048-fix-image-thumbnails/validation-results.md`: build
      gates, S1–S8 outcomes (incl. S6 fresh-config run and S8 parity
      note), deviations if any; tick the completed tasks in this file

---

## Dependencies & Execution Order

- **T001** → **T002** → **T003** → **T004** → **T005** (US1 chain)
- **T006** depends only on T002's file layout decisions being settled;
  it edits a different file than T003 → parallelizable with T003
- **T007** needs T004 (a build containing both T003 and T006)
- **T008** needs T004 (and benefits from T006 being in the build)
- **T009, T010** last; T009 parallel with T010

### Parallel Opportunities

```text
After T002:   T003 (wicengine.cpp)  ∥  T006 (thumbs.cpp)
After T005:   T007  ∥  T008 (independent validation passes)
Final:        T009  ∥  T010
```

## Implementation Strategy

MVP = Phases 1–3 (T001–T005): thumbnails render again. Phase 4 hardens
failure honesty (one line + validation). Phase 5 is validation-only
unless chunk sizing needs tuning. Total: 10 tasks, 2 source files.
