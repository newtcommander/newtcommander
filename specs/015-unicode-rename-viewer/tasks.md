# Tasks: Unicode Rename Field + Viewer Encoding (Feature 015)

**Input**: spec.md, plan.md, research.md. Tests: manual matrix (quickstart.md)
+ generated reference files; no automated harness in this codebase.

## Phase 1: Setup & analysis

- [X] T001 Branch `015-unicode-rename-viewer`, spec, plan, checklist
- [X] T002 Root-cause the F2 `?` (ANSI dialog window vs wide helpers) — research R1
- [X] T003 Map the viewer text pipeline (byte model, no Unicode) — research R2

## Phase 2: Part A — Rename/Copy/Move/Edit-New `?` (US1) — SHIPPED (1fcde51)

- [X] T004 [US1] Thread `unicodeWnd` through `CCommonDialog`→`CDialog` (salamand.h)
- [X] T005 [US1] `CCopyMoveDialog` + `CCopyMoveMoreDialog` → Unicode (dialogs3.cpp); `CEditNewFileDialog` inherits
- [X] T006 [US1] Build Debug + Release clean; commit + ff main

## Phase 3: Part B — Viewer encoding (US2/US3)

- [X] T007 [US2] Encoding state + constants (`ContentEncoding`, VCE_*), viewer.h
- [X] T008 [US2] `ViewerDetectEncoding` (BOM/UTF-8/NUL-guard) + `ViewerDecodeChar` (viewer2.cpp)
- [X] T009 [US2] Detect on open in `FileChanged`; force text + bypass CodeTable for UTF-8; guard legacy auto-select/DefaultConvert
- [X] T010 [US2] Wide render of visible lines: `Utf8SegCells` + `MyTextOutSeg` + `Paint` segment positioning (viewer.cpp)
- [X] T011 [US2] Code-point stepping in `GetOffsetOrXAbs` (gated) (viewer2.cpp)
- [X] T012 [US2] Report actual encoding in `SetViewerCaption` (viewer3.cpp)
- [X] T013 [US2] Manual switch: `CM_VIEWER_CODING_UTF8` command + Coding-menu item + reset on legacy pick (resource.rh2, viewer3.cpp)
- [X] T014 [US3] FR-009 binary guard (NUL byte → not UTF-8 text)
- [X] T015 Generate reference test files (viewer.md) — scratchpad gen-viewer-tests.ps1
- [X] T016 Build Debug + Release clean

## Phase 4: Verification & docs

- [X] T017 Detection reasoning vs the 9 reference files (research R3)
- [X] T018 research.md / quickstart.md with documented limitations
- [ ] T019 [US3] User interactive walkthrough (open each file, F2 each dir, switch coding) — headless env cannot drive the GUI
- [ ] T020 Follow-up (documented, out of scope): UTF-16 rendering (16-bit EOL/line scanner); exact caret/selection on multi-byte lines; CJK double-width cells

## Phase 5: Commit

- [X] T021 Part A committed (1fcde51)
- [X] T022 Part B commit + ff main

## Notes

- Gated design: legacy/ASCII files are byte-for-byte unchanged (zero regression);
  the Unicode path activates only on detected UTF-8.
- UTF-16 detected but intentionally left to the existing (hex) path until the
  EOL-scanner follow-up — never rendered worse than before.
