# Tasks: SFTP Connect Window UX + Password Persistence (Feature 017)

**Input**: spec.md, plan.md, research.md, audit/A|B|C.md.

## Phase 1: Audit (parallel agents + orchestrator)
- [X] T001 Branch 017; brief; 3 parallel audits (A password, B UX, C crypto/keys)
- [X] T002 Consolidate → research.md (P1/P2/P3 password; UX gaps; K1/K2 keys)

## Phase 2: Password persistence (US1) — P1/P2/P3
- [X] T003 [US1] P1: persist/restore Config.QuickConnect under "QuickConnect" subkey (sftp.cpp Save/LoadConfiguration)
- [X] T004 [US1] P2: reuse+preserve the stored blob for quick-connect (guard reuse on s->SavePassword; commit via ConnectCommitToEntry with the entry as its own fallback)
- [X] T005 [US1] P3: write field edits/password back onto the selected bookmark, preserving ItemName (ConnectCommitToEntry; IDB_CONNECT + IDB_SAVEBOOKMARK)
- [X] T006 [US1] FR-007: unchecking "Save password/passphrase" clears the stored blob on commit

## Phase 3: Bookmark UX (US2) — U1..U5
- [X] T007 [US2] U1: explicit Save button (IDB_SAVEBOOKMARK) + handler; resource button
- [X] T008 [US2] U2: wire Duplicate (IDB_COPYBOOKMARK — was dead); resource button
- [X] T009 [US2] U3: visible "Quick Connect" row (item data QC_ITEM); select/seed logic
- [X] T010 [US2] U4: Delete confirmation + relabel "Delete"; Rename/Delete disabled for Quick Connect
- [X] T011 [US2] U5: double-click a row = Connect (LBN_DBLCLK)
- [X] T012 [US2] Resource: strings (IDS_QUICKCONNECT/BOOKMARKNAME/BOOKMARKSAVED/CONFIRM_DELETEBOOKMARK), IDB_SAVEBOOKMARK, dialog relayout

## Phase 4: Keys (US3)
- [X] T013 [US3] K1: wire DetectKeyFormat/KeyFormatSupported before saPrivateKey auth → clear message (session.cpp)
- [ ] T014 [US3] K2 (deferred, documented): passphrase prompt/retry parity — auth-flow, untestable without a live server

## Phase 5: Verify
- [~] T015 Build Debug + Release (plugin) clean
- [ ] T016 Registry round-trip proof: save a quick-connect + bookmark with a password → reload → blob present, no plaintext (verify against the plugin key)
- [ ] T017 [US1/US2/US3] User interactive test against a live SFTP server (create/edit/save bookmark; save password survives restart; connect)

## Phase 6: Commit
- [ ] T018 Commit + ff main

## Deferred (documented follow-ups, not blocking the report)
- Temp-staging so Cancel undoes structural edits (New/Duplicate/Rename/Delete); FTP `TmpFTPServerList` pattern.
- Move Up/Down reorder; right-click context menu (IDM_SRVCONTEXTMENU).
- Advanced fields UI: UseCompression, per-bookmark keepalive, TargetPanelPath.
- K2: passphrase prompt on encrypted key / re-prompt on crKeyUnlock.
