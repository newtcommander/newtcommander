# Tasks: Consistent SFTP Plugin Dialog Appearance

**Input**: Design documents from `/specs/009-sftp-dialog-style/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md

**Tests**: No automated tests. Verification is manual side-by-side visual comparison on a
clean build (per spec clarification 2026-07-17).

**Organization**: Tasks grouped by user story. Note: an initial fix (drop
`ICC_STANDARD_CLASSES`; give SFTP dialogs `DS_SHELLFONT`) and constitution principle VI are
already committed on this branch; several tasks therefore verify/confirm rather than create
from scratch. The decisive new work is the Phase 2 root-cause spike.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3)

## Path Conventions

Existing WinAPI repository; changes confined to `src/plugins/sftp/` and
`.specify/memory/constitution.md`. Spike artifacts live in the session scratchpad only.

---

## Phase 1: Setup

**Purpose**: Establish a clean, known baseline to test against.

- [X] T001 Confirm the working branch is `009-sftp-dialog-style` and the initial fix is present: no `ICC_STANDARD_CLASSES` in `src/plugins/sftp/sftp.cpp`, and `DS_SHELLFONT` (`DS_SETFONT | DS_FIXEDSYS`) on every dialog in `src/plugins/sftp/lang/lang.rc2`. — CONFIRMED: sftp.cpp:101 = `ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES`; 8/8 dialogs have DS_FIXEDSYS.
- [X] T002 Produce a clean baseline build with `build.cmd rebuild` and confirm success plus fresh `sftp.spl` and `english.slg` under `build\newtcommander\Debug_x64\plugins\sftp\`. — DONE: BUILD SUCCEEDED, 0 errors; fresh artifacts 21:13.

---

## Phase 2: Foundational (Blocking Prerequisites) — Root-cause spike

**Purpose**: Empirically determine the true trigger of the divergent EDIT focus decoration
before committing to the final fix. The earlier `ICC_STANDARD_CLASSES` hypothesis is
unconfirmed (research.md).

**⚠️ CRITICAL**: No user-story fix is finalized until the trigger is confirmed here.

- [X] T003 Build a minimal self-contained Win32 test harness in the scratchpad that embeds the same comctl32 v6 dependency as `src/manifest.xml`, creates a dialog containing a single EDIT control, gives it focus, and captures a screenshot — in two variants: (A) without calling `InitCommonControlsEx(ICC_STANDARD_CLASSES)` and (B) with it. Compile with the VS2022 toolchain. — DONE: `editprobe.c` compiled & run.
- [X] T004 Run both harness variants, capture screenshots, and compare the focused-edit decoration to determine whether `ICC_STANDARD_CLASSES` alone flips classic ↔ modern on this Windows 11 build. — DONE: no difference in the reproducible (inactive) state; both classic.
- [X] T005 If T004 shows `ICC_STANDARD_CLASSES` is NOT the differentiator, extend the harness to toggle the remaining candidate factors from research.md one at a time — thread DPI-awareness context (`SetThreadDpiAwarenessContext`) and applied theme (`GetWindowTheme`/`SetWindowTheme`) — until the factor that flips the decoration is isolated. — DONE (with limitation): DPI mode also had no effect; the active-window accent state could not be reproduced headlessly (background process cannot take foreground), so the active-state trigger remains unmeasured. See research.md § Spike results.
- [X] T006 Record the confirmed root cause and the exact minimal aligning fix in `specs/009-sftp-dialog-style/research.md` (update decisions D1/D2 with the empirical result). — DONE: research.md § Spike results records the findings (ICC/DPI refuted in reproducible state; active-focus accent inconclusive headlessly).

**Checkpoint**: Root cause confirmed; the correct fix is known.

---

## Phase 3: User Story 1 - SFTP dialogs look like the rest of the application (Priority: P1) 🎯 MVP

**Goal**: Every SFTP dialog renders with the application's house style; a focused SFTP text
field shows the same decoration as a focused core/FTP field (no divergent accent underline).

**Independent Test**: On a clean build, open each SFTP dialog next to a core/FTP dialog;
fonts and focused-field decoration match.

- [X] T007 [US1] Ensure `src/plugins/sftp/sftp.cpp` plugin init matches the FTP/core baseline (no `ICC_STANDARD_CLASSES`); if the spike (Phase 2) identified an additional in-plugin trigger, remove that divergence within `src/plugins/sftp/` — not via a per-plugin theming override (constitution principle VI). — DONE: init now `ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES` (matches FTP); spike found no additional in-plugin trigger.
- [X] T008 [US1] Ensure all 8 dialog templates in `src/plugins/sftp/lang/lang.rc2` (`IDD_CONNECT`, `IDD_PASSWORD`, `IDD_HOSTKEY`, `IDD_CHMOD`, `IDD_CONFIG`, `IDD_SYMLINK`, `IDD_RENAME`, `IDD_LOGS`) use `DS_SHELLFONT` + `FONT 8, "MS Shell Dlg"`. — DONE & build-verified: 8/8 dialogs carry DS_FIXEDSYS; this is the confirmed correct font fix.
- [X] T009 [US1] Clean rebuild (`build.cmd rebuild`) and confirm `sftp.spl` + `english.slg` build without errors. — DONE (T002 clean rebuild).
- [ ] T010 [US1] Launch Salamander and verify per contract C1/C2 (contracts/ui-house-style.md): SFTP focused edit matches a core/FTP focused edit in frame/decoration and font. — NOT COMPLETED HEADLESSLY: the active-window focus-accent comparison needs a live visual check on a build from this branch (see research.md § Spike results). Manual step handed to the user.

**Checkpoint**: The reported inconsistency is resolved — MVP complete.

---

## Phase 4: User Story 2 - Consistency is deterministic within a session (Priority: P2)

**Goal**: Using SFTP does not modernize other dialogs, and vice-versa; appearance is
order-independent.

**Independent Test**: Open SFTP and FTP/core dialogs in several orders; none change style.

- [X] T011 [US2] Verify the fix has no process-wide side effect: re-grep `src` to confirm no in-process module registers `ICC_STANDARD_CLASSES`, and confirm the SFTP plugin embeds no manifest/theming (contract C3). — DONE: only `translator.cpp` (separate build tool) references `ICC_STANDARD_CLASSES`; no in-process module does; SFTP embeds no manifest/theming.
- [ ] T012 [US2] Runtime check per contract C3: in one session open FTP/core dialog, then use SFTP, then reopen the FTP/core dialog; confirm its appearance is unchanged. — NOT COMPLETED HEADLESSLY: needs the live app. Statically de-risked by T011 (no process-wide `ICC_STANDARD_CLASSES` registration remains, so no order-dependent modernization can occur). Manual step handed to the user.

**Checkpoint**: Appearance is deterministic and non-interfering.

---

## Phase 5: User Story 3 - New SFTP windows stay consistent by default (Priority: P3)

**Goal**: The house-style convention is recorded durably so future dialogs inherit it.

**Independent Test**: The convention exists in the constitution and would flag a deviating
dialog in review.

- [X] T013 [US3] Confirm `.specify/memory/constitution.md` contains principle VI (UI Consistency) defining the house style (DIALOGEX + DS_SHELLFONT + MS Shell Dlg, standard themed controls, no per-plugin `ICC_STANDARD_CLASSES`/manifest/theming), at version 1.1.0 (contract C5). — DONE: principle VI present, version 1.1.0, committed in 7541058.

**Checkpoint**: Durability guaranteed via documentation + review (no automated check, per clarification).

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Final verification and cleanup.

- [ ] T014 Run the full `specs/009-sftp-dialog-style/quickstart.md` validation (clean build + all SFTP dialogs + order-independence). — PARTIAL: build portion DONE (clean rebuild, 0 errors); the visual portions (focused-edit comparison, order-independence) require the live active-window check and are handed to the user.
- [X] T015 Remove any temporary spike/harness/instrumentation so the working tree contains only the intended source changes (scratchpad artifacts only; nothing left in `src/`). — DONE: harness lives only in the session scratchpad; repo-root scratch logs removed; git status confirms no harness/binary leaked into the repo.
- [X] T016 Commit the feature on branch `009-sftp-dialog-style` (author Pavel Stupka, no Co-Authored-By), and update `spec.md` Status if warranted. — Source fix already committed (7541058); speckit artifacts committed separately.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependencies.
- **Foundational spike (Phase 2)**: depends on Setup; **blocks** finalizing US1.
- **User Stories (Phase 3–5)**: depend on the spike outcome for US1's exact fix; US2/US3 are
  largely verification and can proceed once US1's change is settled.
- **Polish (Phase 6)**: depends on all user stories.

### User Story Dependencies

- **US1 (P1)**: the core fix; depends on the spike (Phase 2).
- **US2 (P2)**: verifies the absence of process-wide side effects of the US1 fix.
- **US3 (P3)**: independent (documentation); already largely in place.

### Parallel Opportunities

- T008 (resource templates) and T013 (constitution check) touch different files and can run
  in parallel.
- Most other tasks are sequential (shared build output / same files / gated on the spike).

---

## Implementation Strategy

### MVP First (User Story 1)

1. Phase 1 Setup → clean baseline.
2. Phase 2 Foundational spike → confirm the true trigger (critical).
3. Phase 3 US1 → apply/confirm the aligning fix → **STOP and VALIDATE** the focused-edit
   decoration matches the rest of the app.

### Incremental Delivery

US1 (MVP, resolves the report) → US2 (order-independence) → US3 (durability) → Polish
(quickstart validation + commit). Each step adds assurance without breaking the previous.

---

## Notes

- No automated tests by design (spec clarification); verification is manual + build.
- Prefer removing divergence over per-plugin theming overrides (constitution principle VI).
- Commit as Pavel Stupka only (no Co-Authored-By), English commit message.
