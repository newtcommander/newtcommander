# Tasks: SFTP Dialogs — Ephemeral Quick Connect, Empty Bookmarks, Untruncated Localized Texts

**Input**: Design documents from `/specs/053-sftp-connect-dialog/`
**Prerequisites**: plan.md, spec.md (with Clarifications), research.md (D1–D12), data-model.md, contracts/sftp-plugin-persistence.md, quickstart.md, investigation.md

**Tests**: No automated UI tests exist for this plugin and none are requested. Verification is the layout estimator, the Translator's layout validator, the existing SFTP connect harness, and the quickstart scenarios — all listed as explicit tasks.

**Organization**: Tasks are grouped by user story. US1 (ephemeral Quick Connect) is the MVP; US2 (empty bookmarks) interacts with US1 only through the Save button; US3 (untruncated texts) is fully independent of both and can be done first, last, or in parallel.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 (Quick Connect ephemeral), US2 (empty bookmarks), US3 (no clipped text)

## Phase 1: Setup

**Purpose**: Capture the "before" state so every later claim is checkable.

- [X] T001 Record the baseline for verification: run the layout estimator over `translations/*/sftp.slt` (script in specs/053-sftp-connect-dialog/quickstart.md Scenario 4) and save its output plus a copy of the eight `translations/<lang>/sftp.slt` files to a scratch directory; export `HKCU\Software\Tandem Commander\0.1\Plugins Configuration\SFTP` to a `.reg` file (it must contain a `QuickConnect` subkey — create one by using Quick Connect with a saved password on the current build if absent)

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The one shared change both US1 and US2 depend on — the field-read function they both go through.

**⚠️ CRITICAL**: T002 must land before the US1 and US2 tasks that touch the same function, or they will conflict in `ConnectReadFields`.

- [X] T002 In `ConnectReadFields` (src/plugins/sftp/dialogs.cpp:769-796) restructure the field intake so later tasks can hook it cleanly: gate the host and port validation (lines 777-788) behind the existing `forConnect` flag, normalize an invalid/blank port to `SFTP_DEFAULT_PORT` on the non-connect path, and normalize empty host/user to NULL at line 791 to match the neighbouring key-file/initial-path fields; keep the secret-handling block (804-885) and the `selectedBookmark` blob-reuse semantics untouched

**Checkpoint**: The dialog builds; connecting still validates exactly as before, while New/Save no longer refuse empty fields (US2's behaviour arrives here, its remaining tasks harden it).

---

## Phase 3: User Story 1 — Quick Connect leaves nothing behind (Priority: P1) 🎯 MVP

**Goal**: Quick Connect stores nothing anywhere — no fields, no secrets, not in the registry and not in memory between dialog openings — and cannot be asked to (research D5, D5a, D8).

**Independent Test**: quickstart.md Scenario 1 — use Quick Connect with a password, reopen the dialog and restart the application (fields empty both times), confirm the `QuickConnect` registry subkey does not exist, and confirm the save-password/save-passphrase/Save controls are greyed out.

### Implementation for User Story 1

- [X] T003 [US1] Stop persisting Quick Connect in `CPluginInterface::SaveConfiguration` (src/plugins/sftp/sftp.cpp:593-601): delete the `CreateKey`/`SaveServer` block (597-601) and **keep** `registry->DeleteKey(regKey, CFG_QUICKCONNECT)` at 596 — that retained call is the purge of stale data; update the feature-017 comment at 593-594 to say why the subtree is deleted and never written
- [X] T004 [US1] Stop loading Quick Connect in `CPluginInterface::LoadConfiguration` (src/plugins/sftp/sftp.cpp:517-524): remove the block (`LoadServer` stays for bookmarks); note in the comment that the load key is read-only so the purge cannot happen here
- [X] T005 [P] [US1] Remove Quick Connect from the password-manager re-encryption sweep in `CSFTPServerList::EncryptPasswords` (src/plugins/sftp/sftp.cpp:285-295, lines 293-294) — it can no longer hold blobs
- [X] T006 [US1] Add a full reset for the transient entry: give `CSFTPServer` a reset-to-constructor-defaults operation (or reuse the constructor values explicitly) in src/plugins/sftp/sftp.cpp near `Clear()` (183-195), because `Clear()` frees only strings/blobs and leaves `Port`, `AuthMethod`, `SavePassword`, `SavePassphrase`, `KeepAlive*` and `UseCompression` stale; document the distinction in src/plugins/sftp/sftp.h next to the `QuickConnect` member (130) as "transient, reset on every dialog open"
- [X] T007 [US1] Call that reset on `Config.QuickConnect` in `WM_INITDIALOG` of the connect dialog (src/plugins/sftp/dialogs.cpp:1003-1030) **before** `ConnectFillBookmarkList` (1008) and the field load (1019/1024), so selecting Quick Connect always shows blank fields and port 22
- [X] T008 [US1] Force the save-secret flags FALSE for Quick Connect in the field-read path (src/plugins/sftp/dialogs.cpp:795-796) rather than relying on the UI state — `IsDlgButtonChecked` ignores `EnableWindow`, so a greyed-out checkbox still reports its last state; with the flags FALSE the encryption branches (827-839, 863-875) never run and no master-password prompt can be raised for Quick Connect
- [X] T009 [US1] Disable the two save-secret checkboxes **and** the Save button while the Quick Connect row is selected, in `ConnectUpdateButtons` (src/plugins/sftp/dialogs.cpp:965-972) — it already branches on "is a bookmark" and runs after the selection is set (called from 1027, 1077, 1100, 1128, 1146, 1165, 1187), unlike the field-load path where at init the fields load before the row is selected; also uncheck them for Quick Connect so the UI matches the forced flags (research D8)
- [X] T010 [US1] Make the Quick Connect rule survive the auth-method radio buttons: `ConnectSetAuthMode` (src/plugins/sftp/dialogs.cpp:725-734) re-enables the checkboxes from three call sites (756, 1035, 1038), so pass the "is Quick Connect" state in (or re-apply `ConnectUpdateButtons` after it) and verify by toggling Password/Private key with Quick Connect selected
- [X] T011 [US1] Clear the transient entry after the connect request has been handed on: in the `IDB_CONNECT` handler (src/plugins/sftp/dialogs.cpp:1199-1209) reset `Config.QuickConnect` after `d->Result->CopyFrom(entry)` succeeds, so nothing survives in memory for the rest of the session
- [X] T012 [US1] Guarantee the stale-data purge even when the application never saves configuration (research D5a): bump `CURRENT_CONFIG_VERSION` to 2 (src/plugins/sftp/sftp.cpp:39), detect a loaded `Config.Version` below 2 in `LoadConfiguration` (471), and from the plugin's own lifecycle (`CPluginInterface::Connect` at sftp.cpp:632 or `Release` at 455) trigger one forced purge-only save via `SalamanderGeneral->CallLoadOrSaveConfiguration(FALSE, …)` following the pattern in src/plugins/checkver/checkver.cpp:202; guard it so it runs at most once
- [ ] T013 [US1] Verify US1 end to end against specs/053-sftp-connect-dialog/quickstart.md: Scenario 1 (all nine steps, including the auto-save-OFF purge in step 8 and the no-master-password-prompt check in step 9) and Scenario 2 (bookmarks keep their stored secrets and enabled controls); record the results in that file's results block

**Checkpoint**: Quick Connect is fully ephemeral and provably stores nothing; bookmarks are unaffected.

---

## Phase 4: User Story 2 — A bookmark can be created empty (Priority: P2)

**Goal**: Creating or saving a bookmark needs only a non-empty name; address and port are required only when connecting (research D6).

**Independent Test**: quickstart.md Scenario 3 — create a named bookmark with every field empty, confirm it survives closing/reopening the dialog and a restart, confirm Connect on it still reports the missing host, and confirm a partially filled bookmark can be saved.

**Dependency**: T002 delivers the core behaviour; the tasks below harden the surrounding paths.

### Implementation for User Story 2

- [X] T014 [US2] Tighten the entry-list label fallback in `ConnectFillBookmarkList` (src/plugins/sftp/dialogs.cpp:904): the address arm tests only `!= NULL`, so an empty-string address would render a blank row; check for a non-empty address instead (the hardcoded English `"(unnamed)"` on the same line stays — it is unreachable while blank names are rejected, and changing it would need a new string id)
- [X] T015 [US2] Move the last-used-entry assignment in the `IDB_CONNECT` handler (src/plugins/sftp/dialogs.cpp:1203) to after `ConnectCommitToEntry` validates (1204), so a failed connect from an empty bookmark does not make it the entry the dialog seeds next time (research D9)
- [X] T016 [US2] Confirm no code path assumes a bookmark has an address: re-read the address readers found in the code map (src/plugins/sftp/fs.cpp:60, 534; dialogs.cpp:741, 1117; sftp.cpp:237, 420) and the bookmark load loop (sftp.cpp:494-513) after the T002 normalization, and fix any that would misbehave with an absent address; note the outcome in specs/053-sftp-connect-dialog/investigation.md
- [ ] T017 [US2] Verify US2 end to end against specs/053-sftp-connect-dialog/quickstart.md Scenario 3 (all seven steps), including the save/load round trip of an empty named bookmark and the still-rejected blank name; record the results in that file

**Checkpoint**: Empty and partially filled bookmarks work; connecting is still guarded.

---

## Phase 5: User Story 3 — Localized texts are fully visible (Priority: P3)

**Goal**: Zero clipped or overlapping controls in every SFTP dialog in all eight shipped languages, with no text and no accelerator changed (research D1, D3, D10, D11, D12).

**Independent Test**: quickstart.md Scenario 4 — the estimator reports 0 clipped controls (from 26), `build_langs.cmd --module sftp --check-layout` reports no warnings, and a visual pass in Czech and French shows nothing cut off or overlapping.

**Fully independent** of US1/US2 — different files (`lang/lang.rc2`, `translations/`), no shared code.

### Implementation for User Story 3

- [X] T018 [US3] Re-lay-out the connect dialog in src/plugins/sftp/lang/lang.rc2:121-160 to the measured targets: dialog width 340 → 408, label column `cx` 40 → 99 (ids 601, 605, 609, 612, 615, 618), input column `x` 165 → 226 keeping `cx` 120 (and the key-file edit + browse button pair), port label 18 → 30 at `x` 289 → 348 with its edit at `x` 307 → 378, `IDC_AUTHKEY` 70 → 74; keep the bookmark list/buttons column and the bottom button row anchored, change no text and no accelerator
- [X] T019 [P] [US3] Re-lay-out the configuration dialog in src/plugins/sftp/lang/lang.rc2:237-264: width 260 → 350, label column (ids 3000-3007) → 176, edit column `x` 130 → 185, `IDC_ENABLELOG` 120 → 147, `IDC_STATIC_10` 70 → 139 at `x` 130 → 157 with `IDE_LOGMAXSIZE` at `x` 205 → 301, group box 3008 to the new width; **resolve the pre-existing 44-unit overlap** between `IDC_COLVIEW_RIGHTS` (x 14-174) and `IDC_SHOWOCTAL` (x 130-248, needs 208) — moving the checkbox below the radio buttons into the free vertical space is the cheaper option and reduces the required width (research D11)
- [X] T020 [P] [US3] Re-lay-out the host-key dialog in src/plugins/sftp/lang/lang.rc2:173-187: width 300 → 355, `IDB_HOSTKEY_TRUST` 76 → 123, `IDB_HOSTKEY_ONCE` 76 → 87 at `x` 117 → 164, `IDCANCEL` `x` 243 → 301; the trust button must also fit the caption it is given at run time (`IDS_HOSTKEY_ACCEPTNEW`, ~111 units — dialogs.cpp:182), which the 123 already covers (research D10)
- [X] T021 [P] [US3] Re-lay-out the permissions dialog in src/plugins/sftp/lang/lang.rc2:189-220: width 210 → 257, left label column (ids 3003-3006) 30 → 46 with the checkbox/edit column `x` 40 → 55 (and the group columns at `x` 90 → 100, 140 → 150, plus `IDC_STATIC_1` 40 → 50), `IDC_RECURSE` 190 → 208, `IDC_SETTIME` 90 → 143 with `IDE_MTIME` at `x` 100 → 153, and re-centre the OK/Cancel pair
- [X] T022 [P] [US3] Re-lay-out the symlink dialog in src/plugins/sftp/lang/lang.rc2:266-277: width 240 → 294, `IDT_SYMLINKNAME` 50 → 107 with both edits at `x` 60 → 116 keeping `cx` 173, and re-centre OK/Cancel
- [X] T023 [P] [US3] Re-lay-out the owner/group dialog in src/plugins/sftp/lang/lang.rc2:222-235 without changing its width (220 fits): `IDC_OG_SETOWNER` 108 → 127 and `IDC_OG_SETGROUP` likewise, both edits at `x` 120 → 136 (reduce their `cx` only as far as the right margin requires), `IDC_OG_RECURSE` 200 → 208
- [X] T024 [P] [US3] Enlarge the password prompt static in src/plugins/sftp/lang/lang.rc2:162-171: `IDT_PROMPTTEXT` currently has a 2 × 206 = 412-unit budget for a run-time message needing ~407 units *before* a file path is substituted, so give it a third line (and the dialog the matching height) — the estimator cannot see this because the control is empty in the template (research D10)
- [X] T025 [US3] Rebuild so `english.slg` carries the new geometry, then export a current-structure template: `build.cmd` followed by `src\vcxproj\build_langs.cmd --export-templates --module sftp`; confirm the template lands in `<build>\tandemcommander\translator\templates\sftp.slt`
- [X] T026 [US3] Preview the language refresh: `python -m translate.merge --module sftp --dry-run` — it must report **zero gaps** for every language (zero gaps means no text is fetched or replaced). If any language shows gaps, stop and investigate before writing anything
- [X] T027 [US3] Apply the refresh: `python -m translate.merge --module sftp --all`, then `python -m translate.slt --verify` to confirm the writer round-trips every committed file byte-for-byte
- [X] T028 [US3] **Prove FR-008 mechanically**: compare the eight `translations/<lang>/sftp.slt` files against the copies saved in T001 and assert that every quoted string is byte-identical and only numeric coordinate fields differ (a short script over `translate.slt.load` comparing `row.text` per `(section, control id)` is the reliable form); also confirm the 24 hand-curated Czech overrides are still present. Record the result — if any text changed, revert and switch to a template-only approach
- [X] T029 [US3] Rebuild the shipped language modules and run both layout checks: `src\vcxproj\build_langs.cmd --module sftp --force`, then the estimator script from quickstart.md Scenario 4 (**expect 0 clipped controls, from 26**) and `src\vcxproj\build_langs.cmd --module sftp --check-layout` (expect no `LAYOUT WARNINGS` for sftp — this is the only check that also catches overlap)
- [ ] T030 [US3] Visual pass per specs/053-sftp-connect-dialog/quickstart.md Scenario 4: with the Czech UI open all nine dialogs defined in src/plugins/sftp/lang/lang.rc2 (connect, configuration, permissions, host key, symlink, owner/group, password prompt, rename, logs) and confirm nothing is cut off, nothing overlaps, and the input fields are no narrower than before; repeat for French (the worst case for width) and capture screenshots of the connect and configuration dialogs for the PR

**Checkpoint**: SC-004 evidenced — zero clipped controls, no overlap, unchanged wording and accelerators.

---

## Phase 6: Polish & Cross-Cutting Concerns

- [X] T031 Make the translation client lazy in tools/translate/merge.py (lines 362-370): construct it only when `collect_gaps` actually returns gaps, so a geometry-only refresh runs offline and deterministically and cannot be blocked by a missing key (research D3)
- [X] T032 [P] Add the CHANGELOG.md entries under the existing `[Unreleased]` heading (research D7), in the user's terms: **Fixed** — texts cut off in the SFTP plugin's dialogs in every non-English language (26 controls across six dialogs), the password prompt overflowing, and two overlapping controls in the plugin's settings; **Changed** — Quick Connect no longer remembers anything between uses and can no longer store passwords (any previously saved Quick Connect password is deleted on first run); **Changed** — a bookmark can now be created and saved empty, with the server address required only when connecting
- [X] T033 [P] Run clang-format on the touched C++ files (src/plugins/sftp/dialogs.cpp, src/plugins/sftp/sftp.cpp, src/plugins/sftp/sftp.h) per the repo config — note pwsh7/normalize.ps1 is unavailable on this machine, so invoke clang-format directly
- [ ] T034 Run the SFTP connect regression harness `src\plugins\sftp\test\run_keyauth.cmd` (quickstart.md Scenario 6) to confirm the dialog and configuration changes did not disturb the authentication paths fixed in feature 051
- [X] T035 Full build regression (quickstart.md Scenario 7): `build.cmd rebuild` and `build.cmd full release` both complete with 0 errors, and every Success Criterion SC-001…SC-005 is checked off
- [X] T036 Record the follow-ups this feature deliberately did not do, in specs/053-sftp-connect-dialog/investigation.md and (if the project keeps one) the backlog: three duplicate keyboard accelerators in the **English** connect dialog, the hardcoded English `"Close"` caption (dialogs.cpp:1012) and `"(unnamed)"` fallback (dialogs.cpp:904), the logs dialog being resizable with no `WM_SIZE` handler, and a possible product-wide truncation guard

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (T001)**: no dependencies — but must run *before* any `.slt` is touched, since it captures the baseline T028 compares against
- **Foundational (T002)**: needed by US1 (T008 touches the same function) and by US2 (it *is* US2's core behaviour)
- **US1 (T003–T013)**: after T002; T003/T004/T005 are independent of the dialog tasks
- **US2 (T014–T017)**: after T002; independent of US1 except that T009 (disable Save for Quick Connect) closes the hole T002 opens in the Save path — do T009 before shipping US2
- **US3 (T018–T030)**: independent of everything else; strictly ordered internally — template edits (T018–T024) → export (T025) → dry-run (T026) → apply (T027) → prove (T028) → verify (T029, T030)
- **Polish (T031–T036)**: T031 ideally before T026/T027 (it makes the refresh offline); T032/T033 anytime; T034/T035/T036 last

### User Story Dependencies

- US1 and US2 share one function (`ConnectReadFields`) and one button rule (Save disabled for Quick Connect) — sequence them, don't parallelize across the same file
- US3 shares nothing with either and can be done first if a quick visible win is wanted

### Parallel Opportunities

- T019–T024 are separate dialogs in the same file — parallel in principle, but they all edit `lang.rc2`, so treat them as one editing session and review per dialog
- T005 ∥ T003/T004 (different function in the same file — sequence the file edits)
- T032 ∥ T033 (different files)
- The whole of US3 ∥ the whole of US1+US2 (disjoint files)

## Implementation Strategy

**MVP**: T001 → T002 → US1 (T003–T013). That alone delivers the security-relevant change the user asked for first.

**Suggested single-developer order**: T001 → T002 → T003–T013 (US1) → T014–T017 (US2) → T031 → T018–T030 (US3) → T032–T036. Commit per task or per coherent group; every commit must keep `build.cmd` green.

**Riskiest step**: T027/T028 (the language refresh). It rewrites eight committed files; T001's copies and T028's assertion are what make it safe to attempt — if any translated text changes, revert and reconsider before proceeding.
