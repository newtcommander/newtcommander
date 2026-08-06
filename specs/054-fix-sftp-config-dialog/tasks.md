# Tasks: SFTP — Reachable Settings, Reliable Connect, Tight Dialog Layout

**Input**: Design documents from `/specs/054-fix-sftp-config-dialog/`
**Prerequisites**: plan.md, spec.md (3 clarifications), research.md (D1–D6), data-model.md, contracts/connect-attempt-behaviour.md, quickstart.md

**Tests**: No automated UI tests exist for this plugin and none are requested. Verification is the quickstart scenarios against the live reference server, run-time measurement of the rendered dialog, and the existing connect harness — all listed as explicit tasks. Note the harness does **not** cover the address loop (research D5), so it is a guard against collateral damage, not evidence for US2.

**Organization**: Tasks are grouped by user story. The three stories touch three different files and are **fully independent** — any of them can be done first, and each is separately shippable.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 (settings reachable), US2 (connect reliability), US3 (dialog fits language)

## Phase 1: Setup

**Purpose**: Capture the "before" state and confirm the test environment, so every later claim is checkable.

- [X] T001 Confirm the regression environment and record the baseline: `docker ps` shows container `tandem-sftp` up; verify `127.0.0.1:2222` connects in ~1 ms and `::1:2222` hangs without refusing (PowerShell `TcpClient.ConnectAsync` with a timeout, per specs/054-fix-sftp-config-dialog/quickstart.md prerequisites); time today's failure of `localhost` in the connect dialog (expect ~20 s "Došlo k vypršení časového limitu"); copy the 11 `translations/<lang>/sftp.slt` files to a scratch directory for the later text-invariance proof

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: None. The three stories share no code — US1 is a resource template, US2 is `session.cpp`, US3 is `dialogs.cpp`. There is deliberately no foundational phase; start with whichever story you want.

---

## Phase 3: User Story 1 — SFTP settings can be opened and changed (Priority: P1) 🎯 MVP

**Goal**: Pressing Configure opens a normal, visible, closable settings window instead of freezing the application (research D1).

**Independent Test**: quickstart.md Scenario 1 — Plugins Manager → Test (to load the plugin) → Configure → a titled window appears, a value can be changed and persisted, Cancel discards, the application stays responsive throughout.

### Implementation for User Story 1

- [X] T002 [US1] Convert `IDD_CONFIG` to a normal dialog in src/plugins/sftp/lang/lang.rc2:252-254: replace `STYLE DS_SETFONT | DS_FIXEDSYS | WS_CHILD` with the style its eight sibling dialogs use — `STYLE DS_SETFONT | DS_MODALFRAME | DS_FIXEDSYS | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU` — and add `CAPTION "SFTP Configuration"` (the English text of the existing `IDS_CONFIGTITLE`, lang/lang.rc2:26); keep the 366×250 size and `FONT 8, "MS Shell Dlg"`; do not add `WS_VISIBLE` (no SFTP dialog has it)
- [X] T003 [US1] Add the confirm/cancel buttons the template lacks, immediately before `END` in src/plugins/sftp/lang/lang.rc2: `DEFPUSHBUTTON "OK", IDOK, 254, 229, 50, 14` and `PUSHBUTTON "Cancel", IDCANCEL, 309, 229, 50, 14` — right-aligned per the IDD_SYMLINK convention (dialog width 366 − 7 margin − 50 = 309). Content currently ends at y=219 in a 250-tall dialog, so they fit with a 7-unit bottom margin and no resize. `ConfigProc` already handles both commands (src/plugins/sftp/dialogs.cpp, WM_COMMAND) — do not touch it. Do not add `IDHELP`: SFTP ships no help file
- [X] T004 [US1] Remove the now-false comment in `ShowConfigDialog` (src/plugins/sftp/dialogs.cpp:695-697, "IDD_CONFIG is a child-style template … so we reuse it directly"), replacing it with one line recording that the dialog is a normal popup and why the centring below is correct for it. Leave the `DialogBoxParam` call and `ConfigProc`'s `MultiMonCenterWindow(hwnd, GetParent(hwnd), TRUE)` unchanged — research D1 established the centring becomes correct on its own once the template is a popup, because that helper works in screen coordinates (src/common/multimon.cpp:138,193)
- [X] T005 [US1] Build (`build.cmd full`) and run quickstart.md Scenario 1 and Scenario 2: the window appears titled, centred and fully on screen; OK persists a changed log size, Cancel discards, the X closes; the value survives an application restart; every label is readable and nothing overlaps (feature 053's layout for this dialog becomes visible for the first time). Record the result in quickstart.md's results block

**Checkpoint**: The SFTP settings are reachable for the first time; the application never has to be killed.

---

## Phase 4: User Story 1 (continued) — translation refresh for the new rows

**Why separate**: T002–T003 change the dialog's *structure*, and `.slt` import is strictly positional, so every language's translation source must be regenerated. This must land with US1, but it is a distinct kind of work with its own failure mode (research D4).

- [X] T006 [US1] Regenerate the English template after the resource change: `build.cmd full`, then `src\vcxproj\build_langs.cmd --export-templates --module sftp`; confirm the exported `<build>\tandemcommander\translator\templates\sftp.slt` section `[DIALOG 530]` now has a non-empty caption row and two new rows with ids 1 and 2
- [X] T007 [US1] Refresh the committed translation source. **Done differently than planned**: `merge` reported 8 gaps per language (the 2 new rows plus 6 pre-existing English fallbacks) and would have re-translated all of them, changing shipped text this feature never intended to touch. Instead a new `tools/translate/addrows.py` inserts only the genuinely missing rows in template order, and the two texts were filled from translations already present in this module (the symlink dialog's "Cancel", and `IDS_CONFIGTITLE` for the caption). Zero API calls.
- [X] T008 [US1] The three disabled languages were refreshed together with the others: `addrows.py` defaults to every registered language (like `rebrand`/`relayout`), so russian, ukrainian and chinesesimplified got the new rows and captions too - verified in the T009 comparison.
- [X] T009 [US1] **Bound and prove what the refresh changed** (quickstart.md Scenario 9): compare every `translations/<lang>/sftp.slt` against the T001 copies and assert the only text differences are the config dialog's caption and its two button captions — expected "OK" stays English (it is on the keep-English list in tools/translate/match.py) and only "Cancel" plus the caption are new. If any other quoted string changed — in particular the six English-fallback entries feature 053 measured — pin or revert those individually via translations/ui-overrides.json. Then `python -m translate.slt --verify` must report byte-exact round-trip for every file
- [X] T010 [US1] Rebuild the language modules (`src\vcxproj\build_langs.cmd --module sftp --force`) and confirm the settings window shows the translated caption and buttons in Czech, plus one more language

**Checkpoint**: US1 complete and shippable — settings reachable, correctly titled in every language, no unintended translation drift.

---

## Phase 5: User Story 2 — A reachable host connects even when one address is dead (Priority: P2)

**Goal**: Overlapping connection attempts, so a silent first address costs almost nothing, while feature 051's bounded total and prompt cancel are preserved (research D2, contracts/connect-attempt-behaviour.md).

**Independent Test**: quickstart.md Scenarios 3–6 — `localhost` connects in under 2 seconds (today: fails after 20 s), `127.0.0.1` is as fast as before, a fully unreachable host still fails within the configured timeout, cancel stops promptly and leaves no dangling attempt.

**Independent of US1 and US3** — different file, no shared code.

### Implementation for User Story 2

- [X] T011 [US2] Rewrite the address loop in `CSFTPSession::OpenSocket` (src/plugins/sftp/session.cpp:204-274) as an overlapping race: keep a set of pending candidates, start the first immediately and each subsequent one ~250 ms after the previous (do not abandon earlier ones), `select()` over **all** pending sockets each slice, and take the first that becomes writable with `SO_ERROR == 0` as the winner. Remove the `!timedOut` term from the outer loop condition (line 204) that currently ends the whole list when the shared budget expires on one address
- [X] T012 [US2] Preserve feature 051's invariants inside the rewritten loop (src/plugins/sftp/session.cpp): the shared `totalBudgetMs` still bounds the **whole** attempt regardless of address count; `IsCancelRequested()` is polled at least once per ≤250 ms slice — note `RequestCancel` can only shut down the *member* socket, which is `INVALID_SOCKET` throughout this loop, so polling is the only cancellation path and it must now cover several pending sockets; keep the `feature 051 (D4/F3)` comment at lines 268-273 and leave the winning socket non-blocking
- [X] T013 [US2] Handle candidate lifetime correctly in src/plugins/sftp/session.cpp: close every losing socket before returning (there is no close helper — `Disconnect` only closes the member socket, so anything this loop opens it must also close), publish exactly one winner under `SocketLock` as today (line 298-300), and move `freeaddrinfo(ai)` (line 275) so the `addrinfo` entries stay alive as long as any candidate references them
- [X] T014 [US2] Keep the failure classification and messages unchanged (src/plugins/sftp/session.cpp:279-296): cancelled → `IDS_CANCELLED`/`crCancelled`, budget exhausted with nothing connected → `IDS_ERR_TIMEOUT`/`crTimeout`, all candidates refused → `IDS_ERR_CONNECT`/`crConnectFailed`. No new strings (contracts/connect-attempt-behaviour.md §2)
- [X] T015 [US2] Add one session-log line naming the address that won (via `LogFmt`, src/plugins/sftp/session.cpp:140-156) — `OpenSocket` currently logs nothing, which is why this defect was invisible to users for so long. Keep it to a single line so the log stays readable
- [X] T016 [US2] Verify US2 against quickstart.md Scenarios 3–6: `localhost` connects in under 2 s; `127.0.0.1` no slower than today; a fully unreachable host fails **within** the configured timeout (not a multiple of it); cancel stops within about a second; and after each attempt `Get-NetTCPConnection -State SynSent` shows nothing left on port 2222. Record the measured timings in quickstart.md

**Checkpoint**: US2 complete — the reported `localhost` failure is gone and feature 051's guarantees are demonstrably intact.

---

## Phase 6: User Story 3 — The connect dialog fits its labels (Priority: P3)

**Goal**: The label column is measured from the loaded language at open time; the field column and the dialog width follow; fields keep their width (research D3).

**Independent Test**: quickstart.md Scenario 7 — open the connect dialog in English, Czech and French, read the controls' rectangles at run time and confirm the gap is a small consistent margin, nothing is clipped, no field shrank, and the window is narrower in English than in Czech.

**Independent of US1 and US2** — different file, no shared code.

### Implementation for User Story 3

- [X] T017 [US3] In `ConnectProc`'s `WM_INITDIALOG` (src/plugins/sftp/dialogs.cpp), measure the loaded language's label texts for the right-hand column — `IDT_HOSTADDRESS` (601), `IDT_USERNAME` (605), `IDT_PASSWORD` (609), `IDT_KEYFILE` (612), `IDT_PASSPHRASE` (615), `IDT_INITIALPATH` (618) — using the dialog's own font: `GetDC`, `SelectObject` the font from `WM_GETFONT`, `GetTextExtentPoint32`, restore, `ReleaseDC`. This is the established idiom in this codebase (src/dialogs5.cpp:3144-3155). Take the widest result plus a small fixed padding as the new label-column width
- [X] T018 [US3] Reposition the affected controls with the measured width (src/plugins/sftp/dialogs.cpp, same `WM_INITDIALOG`): set each label's width to the column width; move the input column (`IDE_HOSTADDRESS` 602, `IDE_USERNAME` 606, `IDE_PASSWORD` 610, `IDE_KEYFILE` 613 + its browse button 614, `IDE_PASSPHRASE` 616, `IDE_INITIALPATH` 619, the save-secret checkboxes 611/617, and the auth radio buttons 607/608) to start right after it, **keeping each field's current width**; move the port label/field pair (603/604) with the host row; then resize the dialog itself so the right margin matches today's. Note repositioning a dialog's controls at run time has no precedent in this tree (research D3), so keep it to one clearly-commented helper local to this dialog
- [X] T019 [US3] Handle the edge cases in the same helper (src/plugins/sftp/dialogs.cpp): if the computed width would exceed the work area, clamp so the dialog stays fully reachable rather than growing off-screen; leave the bookmark list and the bottom button row anchored as they are; re-centre after resizing so the dialog is not off-centre (`MultiMonCenterWindow` runs in this proc already)
- [X] T020 [US3] Verify US3 per specs/054-fix-sftp-config-dialog/quickstart.md Scenario 7 by **measuring the rendered dialog**, not by eye: open the connect dialog from the running build (build/tandemcommander/Debug_x64/tandemcommander.exe), enumerate its children and read each control's rectangle in English, Czech and French; assert the label→field gap is a small consistent margin, every label's rendered text fits its control, no field is narrower than before this feature, controls do not overlap, and the window is narrower in English than in Czech. Record the measured numbers in that quickstart file

**Checkpoint**: US3 complete — the wide gap is gone and feature 053's no-clipping guarantee still holds.

---

## Phase 7: Polish & Cross-Cutting Concerns

- [X] T021 Add the CHANGELOG.md entries under the existing `[Unreleased]` heading (research D6), in the user's terms: **Fixed** — the SFTP plugin's settings could not be opened at all; pressing Configure left the application unresponsive and it had to be killed. **Fixed** — connecting to a host name whose first address is silently unreachable now works (typically `localhost` on a machine with a filtered IPv6 loopback), instead of failing after the full timeout. **Changed** — the SFTP connect dialog is now sized for the language in use, so labels sit next to their fields instead of being followed by a wide gap
- [X] T022 [P] Run clang-format on the touched C++ files (src/plugins/sftp/dialogs.cpp, src/plugins/sftp/session.cpp) per the repo config — note pwsh7/normalize.ps1 is unavailable on this machine, so invoke clang-format directly
- [X] T023 Run the connect regression harness `src\plugins\sftp\test\run_keyauth.cmd` (quickstart.md Scenario 8): 7 passed, 0 failed. This guards the authentication paths from collateral damage; it does **not** cover the address loop (research D5), so it is not evidence for US2
- [X] T024 Full build regression (quickstart.md Scenario 10): `build.cmd rebuild` and `build.cmd full release` both complete with 0 errors; then check every Success Criterion SC-001…SC-005 off against the recorded measurements
- [X] T025 Record follow-ups this feature deliberately did not do, in specs/054-fix-sftp-config-dialog/research.md or the backlog: the connect address race is not covered by any harness (an SDK-free extraction into sftputils.cpp would make it testable, per research D5); `Config.ConnectRetries`/`RetryDelay` are configurable and persisted but never read by any connect code; and run-time dialog sizing could suit other dialogs but was deliberately kept to this one

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (T001)**: run first — it captures the baselines T009 and T016 compare against
- **US1 (T002–T010)**: T002/T003 (template) → T004 (comment) → T005 (verify) → T006–T010 (translation refresh, strictly ordered: export → merge → disabled languages → prove → rebuild)
- **US2 (T011–T016)**: T011 → T012/T013/T014 (same function, sequence them) → T015 → T016
- **US3 (T017–T020)**: T017 → T018 → T019 → T020, strictly ordered (each builds on the previous)
- **Polish (T021–T025)**: T021/T022 anytime after the code settles; T023/T024/T025 last

### User Story Dependencies

**None.** US1 touches `lang.rc2` (+ translations), US2 touches `session.cpp`, US3 touches `dialogs.cpp`. No shared code, no ordering constraint between stories.

### Parallel Opportunities

- The three stories are fully parallel — different files throughout
- T022 ∥ T021 (different files)
- Within US1, T006–T010 are a strict pipeline; within US2 and US3 the tasks touch one function each, so sequence them

## Implementation Strategy

**MVP**: T001 → US1 (T002–T010). That alone restores access to settings that are unreachable today — the most severe of the three defects and the one the user reported first.

**Suggested order**: T001 → US1 → US2 → US3 → polish. US2 before US3 because a broken connection is worse than a wide gap.

**Riskiest steps**: T011–T013 (the connect rewrite — it must not regress feature 051, which exists because of multi-minute waits; T016 is what proves it) and T007–T009 (the translation refresh rewrites 11 committed files; T001's copies and T009's assertion are what make it safe to attempt).
