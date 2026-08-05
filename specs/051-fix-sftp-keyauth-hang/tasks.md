# Tasks: Fix SFTP Private-Key Authentication Hang & Stabilize SFTP Plugin

**Input**: Design documents from `/specs/051-fix-sftp-keyauth-hang/`
**Prerequisites**: plan.md, spec.md, research.md (findings F1–F7, U1–U14, decisions D1–D6), data-model.md, contracts/test-harness.md, quickstart.md

**Tests**: INCLUDED — the spec explicitly mandates the automated regression harness (FR-011, SC-006).

**Organization**: Grouped by user story; US1 = MVP (the reported defect).

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Parallelizable (different files, no dependency on an incomplete task)
- **[Story]**: US1 / US2 / US3 per spec.md

---

## Phase 1: Setup

**Purpose**: Working reference environment + build baseline

- [X] T001 Verify reference environment and baseline: Docker container `tandem-sftp` up on localhost:2222, all three keys in `C:\Users\pavel\.ssh\tandem-sftp-test\` authenticate via CLI (`ssh -i … whoami` → `tctest`), and `build.cmd` produces a clean Debug x64 baseline (per quickstart.md)

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Root-cause library fix + test scaffolding every story builds on

**⚠️ CRITICAL**: T002 is the root-cause fix (F1/D1); no user story is testable before it lands. T003 is the harness skeleton all test tasks extend.

- [X] T002 Patch vendored PEM reader (D1/F1) in `src/common/dep/libssh2/src/pem.c`: make `readline_memory` return −1 once `*filedata_offset >= filedata_len`; add EOF bounds guards to both scan loops of `_libssh2_pem_parse_memory` (mirror `_libssh2_openssh_pem_parse_memory` at pem.c:819/851). Document the local patch in `src/common/dep/libssh2/readme.txt`
- [X] T003 Rework harness skeleton (D6/U2) in `src/plugins/sftp/test/key_auth.c`: switch to `libssh2_userauth_publickey_frommemory` (read key into memory, NULL pubkey — the product path), add scenario dispatcher with CLI per contracts/test-harness.md (`--host/--port/--user/--keydir/--scenario`), 90 s watchdog thread (exit 124, `WATCHDOG: <scenario> hung`), exit codes 0/1/2/124, `PASS|FAIL <scenario> <ms>` output; wire compilation+run into `src/plugins/sftp/test/build_and_run.cmd` (U13)

**Checkpoint**: `key-rsa` scenario now FAILS FAST (clean libssh2 error, no hang) — freeze eliminated, key not yet usable

---

## Phase 3: User Story 1 — Connect with a private key (P1) 🎯 MVP

**Goal**: All three release-blocking keys (RSA-4096 OpenSSH, ECDSA P-256, passphrase RSA PEM) establish a working session; unsupported keys rejected up front with an accurate message.

**Independent Test**: quickstart.md manual rows 1–4 & 6 + harness scenarios `key-rsa`, `key-ecdsa`, `key-passphrase`, `key-passphrase-wrong`, `key-unsupported` all PASS.

- [X] T004 [US1] OpenSSH-container RSA import for the WinCNG memory path (D2/F2) in `src/common/dep/libssh2/src/wincng.c`: detect the `openssh-key-v1` PEM header in `_libssh2_wincng_load_private_memory` / `_libssh2_wincng_pub_priv_keyfilememory` and route through `_libssh2_openssh_pem_parse_memory`, decode the `ssh-rsa` key blob and construct the key via `_libssh2_wincng_rsa_new` (mirror openssl.c:4762 structure); return a proper `_libssh2_error` for unsupported key types inside the container (ed25519 → clear "unsupported type" error, never a parse attempt)
- [X] T005 [US1] Wire ECDSA OpenSSH-container keys into the same pub-priv derivation path in `src/common/dep/libssh2/src/wincng.c`: make the existing `_libssh2_wincng_ecdsa_new_private_frommemory` machinery (wincng.c:2883) reachable from `_libssh2_wincng_pub_priv_keyfilememory`; verify `bcrypt_pbkdf`/cipher availability for encrypted containers — if absent, fail encrypted OpenSSH containers with an accurate error (research D2 fallback)
- [X] T006 [US1] Classic encrypted-PEM decryption in the memory path (D3/U11): port the `FILE*` variant's decrypt branch (pem.c:135–172) into `_libssh2_pem_parse_memory` in `src/common/dep/libssh2/src/pem.c` (new passphrase parameter) and stop discarding the passphrase in `_libssh2_wincng_load_private_memory` (`(void)passphrase;`, wincng.c:959); update all `_libssh2_pem_parse_memory` callers
- [X] T007 [P] [US1] Set session error text on WinCNG key-load failures (U1) in `src/common/dep/libssh2/src/wincng.c`: `_libssh2_error(session, LIBSSH2_ERROR_FILE, …)` before every bare `return -1` in `_libssh2_wincng_pub_priv_keyfilememory` and the memory loaders
- [X] T008 [US1] Correct the capability gate (F2) in `src/plugins/sftp/keyload.cpp` + `keyload.h`: `KeyFormatSupported` now returns TRUE for kfOpenSSH (RSA/ECDSA) and kfPEM (incl. encrypted), FALSE for kfPKCS8 (WinCNG memory loader has no PKCS#8 path — message with `ssh-keygen -p -m PEM` convert hint) and kfPuTTY (existing); fix the stale comment at keyload.h:9–10; detect ed25519 OpenSSH containers up front (read the key-type string) → dedicated "key type not supported by this build" message (FR-007)
- [X] T009 [US1] Error-code-based auth failure classification (U8/D5) in `src/plugins/sftp/session.cpp`: replace the case-sensitive `strstr` heuristics (session.cpp:442–447) with classification on `rc`/`libssh2_session_last_errno` — key-side (`LIBSSH2_ERROR_FILE`, `_KEYFILE_AUTH_FAILED`, `_METHOD_NOT_SUPPORTED`) vs server-side (`_AUTHENTICATION_FAILED`, `_PUBLICKEY_UNVERIFIED`) vs connectivity (FR-006 groundwork)
- [X] T010 [US1] Passphrase prompt-and-retry flow (D5/U12, FR-005) in `src/plugins/sftp/fs.cpp` (connect flow around fs.cpp:124) + `src/plugins/sftp/dialogs.cpp`: keep the `KeyFileLooksEncrypted` sniff as fast path; on key-decrypt failure (per T009 classification) prompt for the passphrase and retry within the same attempt; wrong passphrase → clear error + re-prompt; cancel → clean abort; reuse existing DIALOGEX house-style prompt (Constitution VI)
- [X] T011 [P] [US1] Harness scenarios (contract) in `src/plugins/sftp/test/key_auth.c`: implement `key-rsa`, `key-ecdsa`, `key-passphrase`, `key-passphrase-wrong` (expects clean decrypt-failure, distinct from server rejection), `key-unsupported` (generated ed25519 + `.ppk` stub → local rejection before socket I/O); plus keyload fixture checks (`DetectKeyFormat`/`KeyFileLooksEncrypted` on header samples) in `src/plugins/sftp/test/harness.cpp`
- [ ] T012 [US1] Manual verification (quickstart rows 1–4, 6) in the built app: connect with each reference key, wrong-passphrase retry, unsupported-key rejection; fix regressions found

**Checkpoint**: US1 delivers the MVP — the reported bug is fixed and all supported keys work

---

## Phase 4: User Story 2 — The application never freezes on a failed connection (P1)

**Goal**: Every connection attempt is bounded, cancellable ≤ 2 s, ends in a clear message; whole-app freeze class eliminated architecturally.

**Independent Test**: quickstart rows 5, 7, 8, 10 + harness `key-unauthorized`, `timeout-silent`; deliberate failure modes never require a restart.

- [X] T013 [US2] Worker-thread connect (D4/F3–F6) in `src/plugins/sftp/session.cpp` + `src/plugins/sftp/fs.cpp`: run the full connect sequence (getaddrinfo, TCP connect, handshake, host-key check, auth, sftp_init) on a worker thread started from `EnsureConnected`; UI thread shows the existing cancellable `CreateSafeWaitWindow` and pumps; host-key confirmation and all auth prompts marshal to the UI thread; connect outcome returned to the caller unchanged (`ChangePath`/`ListCurrentPath` contract preserved)
- [X] T014 [US2] Cancel-in-progress (D4/F6, SC-004) in `src/plugins/sftp/session.cpp` + `fs.cpp`: wait-window Close (and app-exit request) closes the session socket from the UI side, erroring out any in-flight libssh2 wait ≤ 2 s; panel returns to previous path; partial session state torn down via the FAILED→IDLE invariant (data-model I3)
- [X] T015 [US2] Keep the OS socket non-blocking after connect (D4/F3): remove the restore-to-blocking at `src/plugins/sftp/session.cpp:224–226` so libssh2's `_libssh2_wait_socket` enforces the 30 s `api_timeout` on every subsequent call; verify handshake/auth/sftp_init still succeed against the reference server
- [X] T016 [P] [US2] Connect-timeout budget across DNS candidates (U4) in `src/plugins/sftp/session.cpp` (OpenSocket loop at :179–227): deduct elapsed time from `ConnectTimeoutSec` across all A/AAAA candidates and poll the cancel flag in short select slices (≤ 500 ms)
- [X] T017 [P] [US2] Fast teardown of dead sessions (U5) in `src/plugins/sftp/session.cpp` (Disconnect, :604): drop `libssh2_session_set_timeout` to ~2 s (or close the socket first) before `sftp_shutdown`/`session_disconnect`/`session_free` when the session is known-failed/stale
- [X] T018 [US2] Password fallback on rejected key (clarification #2, FR-006) in `src/plugins/sftp/session.cpp` + `src/plugins/sftp/dialogs.cpp`: after a server-side key rejection (per T009), if `libssh2_userauth_list` includes `password`, offer the password prompt within the same attempt; decline ends the attempt cleanly with the auth-failure message
- [X] T019 [P] [US2] Harness scenarios (contract) in `src/plugins/sftp/test/key_auth.c`: `key-unauthorized` (throwaway key generated per run → auth-failure, not connectivity), `timeout-silent` (TCP endpoint that never speaks SSH → bounded failure within configured timeout); confirm watchdog catches an artificially induced hang (temporarily revert T002 locally to prove exit 124 — do not commit the revert)
- [ ] T020 [US2] Manual verification (quickstart rows 5, 7, 8, 10): unauthorized key + fallback decline/accept, unreachable port, cancel in every phase (stopwatch ≤ 2 s), app exit during pending connect; fix regressions found

**Checkpoint**: US1 + US2 = both P1 stories done; freeze class gone, UX per clarifications

---

## Phase 5: User Story 3 — Stable day-to-day SFTP sessions (P2)

**Goal**: Long-lived sessions behave predictably: drops surface as errors with a recovery path, no operation wedges the app, no state/resource residue.

**Independent Test**: quickstart rows 9, 11, 12 — soak listing/transfer/reconnect against the reference server incl. forced network drop.

- [X] T021 [US3] Transport-failure detection + reconnect path (U7) in `src/plugins/sftp/session.cpp` + `fs.cpp`: on fatal transport errors (`LIBSSH2_ERROR_SOCKET_RECV/SEND/TIMEOUT`) mark the session dead (`Connected = FALSE` outside Disconnect), have `EnsureConnected` detect the dead session and offer the (currently dead-code) `Reconnect()` flow; remove or wire up the never-read `FatalError` flag (fs.cpp:390)
- [X] T022 [P] [US3] ViewFile cancellable download (F7) in `src/plugins/sftp/fs.cpp` (:939–954): wrap the F3 download loop in `CreateSafeWaitWindow`/`DestroySafeWaitWindow`, test `GetSafeWaitWindowClosePressed()` each 64 KB iteration, abort via the existing `!newFileOK` cleanup
- [X] T023 [P] [US3] Bounded keepalive (U3) in `src/plugins/sftp/fs.cpp` (FSE_TIMER, :709–722) + `session.cpp`: run `Keepalive` with a short per-call timeout (or skip when a connect/operation is in flight); a dead peer costs one bounded stall, never 30 s repeating
- [X] T024 [P] [US3] Cancel-poll completeness in operation loops (U6) in `src/plugins/sftp/operats.cpp` + `session.cpp` (ListDir): verify every network loop (readdir, download, upload, delete) polls cancel between calls (now each call is 30 s-bounded via T015); add any missing poll; document the worst-case cancel latency for in-flight operations
- [X] T025 [P] [US3] Log lifecycle (U10) in `src/plugins/sftp/logs.cpp` + `logs.h` + `fs.cpp`: add `CLogs::RemoveLog(uid)` (under the existing critical section) and call it from `ReleaseObject`/destructor so closed sessions don't accumulate log objects
- [X] T026 [P] [US3] History/command-line reconnect keeps key auth (U14) in `src/plugins/sftp/fs.cpp` (:510–527): before defaulting to `saPassword`, look up host:port:user in `Config.Bookmarks`/QuickConnect and reuse that entry's AuthMethod/KeyFile via `FillParamsFromServer`
- [X] T027 [P] [US3] Secret hygiene (U9) in `src/plugins/sftp/session.cpp` (:368–371) + `src/plugins/sftp/keyload.cpp`: `SecureZeroMemory` key buffers on failure paths and the `head[]` sniff buffers before return (CF-14/15 consistency)
- [ ] T028 [US3] Manual verification (quickstart rows 9, 11, 12): `docker stop` mid-transfer + reconnect, unusual entries (symlinks, chmod-000, UTF-8 names, 50-file nested tree), side-by-side key vs password parity (FR-010); fix regressions found

**Checkpoint**: All stories complete

---

## Phase 6: Polish & Release Criteria

- [ ] T029 Release verification: 3 consecutive clean `--scenario all` harness runs (SC-006) + full 12-row manual matrix pass (SC-002, zero freezes) + cancel timing spot-checks (SC-004); record results in `specs/051-fix-sftp-keyauth-hang/quickstart.md` (results section)
- [X] T030 [P] Findings disposition cross-check (SC-007): update the disposition columns in `specs/051-fix-sftp-keyauth-hang/research.md` to reflect what was actually implemented; any intentionally-unfixed finding gets an explicit deferral reason
- [X] T031 [P] Code standards sweep: clang-format + UTF-8-BOM check on all touched files; new comments in English; vendored-patch notes complete in `src/common/dep/libssh2/readme.txt`; update `CLAUDE.md` 051 entry status

---

## Dependencies

```text
T001 → T002 → ┬ T004 → T005 ─┐
              │ T006 ─────────┤
              │ T007 [P] ─────┤            (US1: libssh2 layer)
              │               ├→ T008 → T009 → T010 → T012
T003 ─────────┴──────────────→ T011 [P] ──┘
                                            (US1 complete = MVP)
T002 → T013 → T014 → T015 → T018 → T020    (US2 core, same files session.cpp/fs.cpp — sequential)
       T016 [P], T017 [P], T019 [P]        (US2 parallel side tasks)
US2(T013–T015) → T021, T023                (US3 tasks touching session threading)
T022, T024–T027 [P]                        (US3 independent — only T015 assumed)
all → T028 → T029 → T030 [P], T031 [P]
```

- **US1 vs US2**: independent in logic but overlapping in `session.cpp`/`fs.cpp` — implement US1 first (MVP), then US2, to avoid merge churn.
- **US3**: T022/T024–T027 can run in parallel any time after Foundational; T021/T023 after the US2 worker-thread lands.

## Parallel Opportunities

- **US1**: T007 ∥ T011 alongside T004–T006 (different files)
- **US2**: T016 ∥ T017 ∥ T019 alongside T013–T015 chain
- **US3**: T022 ∥ T023 ∥ T024 ∥ T025 ∥ T026 ∥ T027 (six independent files/areas)
- **Polish**: T030 ∥ T031

## Implementation Strategy

**MVP = Phase 1–3 (US1)**: T002 alone converts the freeze into a clean error;
T004–T010 make the three release-blocking keys actually work. Ship-worthy
increment on its own.

**Increment 2 = Phase 4 (US2)**: architectural de-freeze (worker thread +
cancel + enforced timeouts) and the clarified fallback UX.

**Increment 3 = Phase 5 (US3) + Phase 6**: stabilization sweep of the
remaining audited findings and the release gates.

**Totals**: 31 tasks — Setup 1, Foundational 2, US1 9, US2 8, US3 8, Polish 3.

## Execution notes

- **T012 / T020 / T028 (manual matrix) and the manual half of T029** need an
  interactive session with the built application: connecting from the panel,
  watching a dialog, pressing Cancel and timing it. They are listed as open
  because they were not performed, not because anything failed — every
  automated gate that could be run has been run and is green (7/7 key-auth
  scenarios, 66/66 logic assertions, clean `build.cmd full`). The matrix lives
  in `quickstart.md` with expected outcomes per row.
- **T030** is complete: `research.md` now carries a per-finding "where it was
  fixed" table plus the two documented deviations (D4 scope, U10 approach).
- **T031** is complete: clang-format applied to every touched plugin file,
  encodings verified unchanged, vendored-patch notes written into
  `src/common/dep/libssh2/readme.txt`, `CLAUDE.md` 051 entry finalized.
- **Translations**: the 5 new UI strings changed the resource layout, so the
  positional `.slt` import failed for all 8 languages until the documented
  two-stage refresh was run (`build_langs.cmd --export-templates --module sftp`
  then `translate.merge --module sftp`; 3,901 DeepL characters, 0 validation
  failures, pre-existing entries untouched). `build.cmd full` is green:
  180 language modules.
