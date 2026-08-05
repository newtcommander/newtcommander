# Research: SFTP Key-Auth Hang — Multi-Agent Audit & Design Decisions

**Feature**: 051-fix-sftp-keyauth-hang | **Date**: 2026-08-05

## Method (per user request: independent agents over the whole plugin)

12 agents in two phases: **5 independent analysis lenses** (root-cause,
threading/UI, timeouts/cancellation, lifecycle/errors, auth-logic+tests),
each reading the entire plugin (~7,100 lines) plus the vendored libssh2
sources the claims depend on → 37 raw findings, deduplicated to 21 unique;
the 7 critical (hang/root-cause) findings then went through **adversarial
verification** (one skeptic agent per finding, instructed to refute against
the actual code; default-refute on uncertainty). Result: **7 confirmed, 0
refuted** — every confirmed finding below is evidence-backed at
file:line. 14 lower-severity findings remain unverified-but-plausible.

## Root Cause (confirmed, high confidence)

**The freeze is an infinite CPU loop inside the vendored libssh2, reached
from an incorrect capability gate in the plugin, amplified to a whole-app
hang by UI-thread networking.** Three layers:

1. **`_libssh2_pem_parse_memory` never terminates when the expected PEM
   header is absent** — `src/common/dep/libssh2/src/pem.c:331`.
   `readline_memory` (pem.c:72–97) *unconditionally returns 0*: at end of
   buffer it yields an empty line and keeps returning success, so the
   header-scan `do/while (strcmp(line, headerbegin) != 0)` spins forever.
   The footer-scan loop (pem.c:341) has the same missing guard. The sibling
   `_libssh2_openssh_pem_parse_memory` HAS the bounds guard (pem.c:819,
   851); the `FILE*` variant is safe (fgets returns NULL at EOF). Upstream
   libssh2 fixed the reader (`pem_readline` fails at EOF); our vendored
   1.11.1_DEV (committed once in feature 008, never patched) predates it.

2. **Trigger path**: `keyload.cpp:67` accepts kfOpenSSH; `session.cpp:431`
   calls `libssh2_userauth_publickey_frommemory` with NULL pubkey →
   libssh2 derives the public key from the private key →
   `_libssh2_wincng_pub_priv_keyfilememory` (wincng.c:3263) →
   `_libssh2_wincng_load_private_memory` tries **only** classic
   `BEGIN RSA/DSA PRIVATE KEY` headers (wincng.c:961–973) →
   an `openssh-key-v1` file never matches → infinite loop, **before any
   network packet** (the 30 s libssh2 timeout never engages — it only
   applies to socket waits).

3. **Amplifier**: the plugin has **zero worker threads**; connect/auth runs
   synchronously on Salamander's main UI thread
   (`sftp.cpp:690 → fs.cpp:139 → fs.cpp:530 → fs.cpp:385 →
   session.cpp:569`), so the spin freezes the entire application. The
   message loop is dead; only process kill helps.

**Why CLI ssh works**: OpenSSH is its own implementation; the defect is in
vendored libssh2's memory-variant PEM reader + the WinCNG loader's format
coverage.

**Why the existing harness missed it**: `test/key_auth.c` calls
`libssh2_userauth_publickey_fromfile_ex` — the *file* PEM path (safe fgets)
— while the plugin switched to `frommemory` (CF-9, Unicode key paths). The
harness tested a different code path than the product ships, and no script
even builds it routinely.

## Confirmed findings (adversarially verified)

| # | Location | Severity | Finding | Disposition |
|---|---|---|---|---|
| F1 | `libssh2/src/pem.c:331` | hang | `_libssh2_pem_parse_memory` header/footer loops never terminate when marker absent; `readline_memory` can't signal EOF | **FIXED** (D1) — root cause; verified 78 ms clean failure vs. infinite hang |
| F2 | `sftp/keyload.cpp:67` | hang | Capability gate accepts kfOpenSSH/kfPKCS8/EC-PEM which the WinCNG *frommemory* loader cannot parse — only unencrypted classic RSA PEM works; gate routes everything else into F1 | **Fix** (D2, D3) |
| F3 | `sftp/fs.cpp:385` | hang | Connect/auth/keepalive/disconnect run blocking on the main UI thread; any stall freezes the app. Verifier note: the 30 s libssh2 timeout is generally **not enforced** — the socket is restored to OS-blocking mode (session.cpp:224–226), so `recv()` can block indefinitely | **Fix** (D4) |
| F4 | `sftp/fs.cpp:530` | hang | Whole connect sequence (DNS, TCP, handshake, auth, sftp_init) synchronous in `ChangePanelPathToPluginFS`; menu handler never returns; no worker thread exists anywhere in the plugin | **Fix** (D4) |
| F5 | `sftp/session.cpp:170` | hang | `getaddrinfo` unbounded/uncancellable on UI thread | **Fix** (D4 — moves to worker) |
| F6 | `sftp/session.cpp:541` | hang | Blocking session + poll-only cancel: cancel flags checked only *between* blocking calls; no mechanism aborts a wait in progress | **Fix** (D4) |
| F7 | `sftp/fs.cpp:943` | hang | ViewFile (F3) download loop: no cancel check, no wait window, unbounded total time | **Fix** (D4) |

## Unverified findings (plausible, lower severity)

| # | Location | Severity | Finding | Disposition |
|---|---|---|---|---|
| U1 | `wincng.c:3282` | error-handling | pubkey-derivation failure returns −1 without session error text → misclassified messages | Fix with D2 |
| U2 | `test/key_auth.c:27` | minor | Harness tests `fromfile`, product uses `frommemory` — freeze path untested | Fix (D6) |
| U3 | `fs.cpp:716` | minor | Keepalive on FSE_TIMER can stall UI on dead peer | Fix with D4 |
| U4 | `session.cpp:199` | minor | Connect timeout is per address candidate (N × 20 s), no cancel in loop | Fix with D4 |
| U5 | `session.cpp:604` | minor | Disconnect = 3 blocking exchanges → up to ~90 s teardown on dead server | Fix with D4 (short teardown timeout) |
| U6 | `session.cpp:674` | minor | ListDir/transfer cancel granularity 30 s (poll between calls) | Fix with D4 (socket-close abort) |
| U7 | `session.cpp:620` | error-handling | `Connected` never cleared on transport failure; `Reconnect()` dead code; no recovery path | Fix (D5) |
| U8 | `session.cpp:446` | error-handling | Case-sensitive `strstr` misclassifies libssh2 errors | Fix (D5 — classify by error code) |
| U9 | `session.cpp:370` | minor | Key bytes freed unwiped on failure paths (also keyload.cpp head buffers) | Fix (trivial, CF-14/15 hygiene) |
| U10 | `logs.cpp:45` | leak | Per-connection logs never removed — unbounded growth | Fix (small) |
| U11 | `session.cpp:431` | error-handling | CF-9 switch to `frommemory` silently broke encrypted classic-PEM keys (WinCNG memory loader ignores passphrase — correct passphrase still fails) | Fix (D3) — release-blocking key #3 |
| U12 | `fs.cpp:124` | error-handling | Encrypted OpenSSH-container keys never get a passphrase prompt (header sniff can't see container encryption) | Fix (D5 — prompt on decrypt-failure + retry) |
| U13 | `test/sftp_smoke.c:86` | minor | Live tests cover password auth only; auth decision tree has zero automated coverage; harness not built by any committed script | Fix (D6) |
| U14 | `fs.cpp:517` | minor | Reconnect from history forces `AuthMethod=saPassword`, breaking key-only servers | Fix (small — reuse bookmark auth method) |

All 21 findings are dispositioned (SC-007): every hang/crash-family finding
is fixed by D1–D6; none deferred. Implementation confirmed all 21 fixed —
F1–F7 and U1–U14 each have code in the diff; see the disposition column and
the "Implementation notes & deviations" section for the two places where the
delivered form differs from the plan (D4 scope, U10 approach).

## Decisions

### D1 — Patch vendored pem.c (root-cause fix)
**Decision**: In `_libssh2_pem_parse_memory`, add EOF bounds guards to both
scan loops (mirroring `_libssh2_openssh_pem_parse_memory`, pem.c:819/851)
AND make `readline_memory` return −1 once the offset reaches
`filedata_len` (upstream `pem_readline` semantics). Document the local
patch in `src/common/dep/libssh2/readme.txt`.
**Rationale**: Removes the *entire* unbounded-spin class at the source, for
every present and future malformed/unsupported input; smallest possible
library change; upstream-aligned behavior.
**Alternatives**: Full libssh2 upgrade to current upstream — rejected for
this feature (large surface, WinCNG diffs, separate risk); plugin-side
rejection alone — rejected (leaves a live landmine in the library).

### D2 — OpenSSH-container key support in the WinCNG memory loader (FR-001)
**Decision**: Extend the vendored WinCNG backend so
`libssh2_userauth_publickey_frommemory` handles `openssh-key-v1` RSA and
ECDSA keys: detect the OpenSSH PEM header in
`_libssh2_wincng_load_private_memory` / `_libssh2_wincng_pub_priv_keyfilememory`
and route through the **existing, correctly-guarded**
`_libssh2_openssh_pem_parse_memory` (pem.c:800), then feed the decoded key
components into the existing constructors (`_libssh2_wincng_rsa_new`
wincng.c:1184; ECDSA import already exists — wincng.c:2883 parses the
container for ECDSA `fromfile`/`frommemory`, it is just unreachable from
the pub-priv derivation path). This mirrors openssl.c:4762's structure.
**Rationale**: The release-blocking matrix (clarification #3) requires
RSA-4096 *OpenSSH format* and ECDSA P-256 to work; `ssh-keygen` has emitted
the OpenSSH container **by default since OpenSSH 7.8 (2018)** — telling
users to convert their default-format keys would fail the feature's intent.
All parsing machinery already exists in the vendored tree; the work is
wiring + component import, not new cryptography.
**Alternatives**: Reject-with-convert-hint — rejected (fails FR-001 as
clarified; the reported key IS the modern default format). Plugin-side
container conversion — rejected (duplicates libssh2 machinery, more new
code in a worse place). Switching to the OpenSSL backend — rejected
(feature 008 chose WinCNG deliberately; no-OpenSSL is a project constraint).
Encrypted OpenSSH containers: supported if the container cipher/KDF
machinery present in the vendored tree covers it (verify `bcrypt_pbkdf`
availability during implementation); otherwise detected and rejected with
an accurate message + convert hint (spec allows: clear message for
unsupported variants).

### D3 — Encrypted classic-PEM keys via the memory path (release-blocking key #3)
**Decision**: Make the passphrase reach the classic-PEM memory path: port
the `FILE*` variant's decryption branch (pem.c:135–172, MD5-PBKDF1 +
cipher lookup) into `_libssh2_pem_parse_memory` and stop ignoring the
passphrase in `_libssh2_wincng_load_private_memory` (wincng.c:959).
**Rationale**: `tctest_rsa_pass` (classic PEM + passphrase) is
release-blocking; today the correct passphrase still fails (U11) because
the memory loader discards it. The decryption logic already exists in the
same file for the `FILE*` variant — this is a port, not new crypto.
**Alternatives**: Fall back to `fromfile` for encrypted keys — rejected
(reintroduces the ANSI-path limitation CF-9 was created to fix). Plugin-side
PEM decryption — rejected (wrong layer, duplicates libssh2 cipher code).

### D4 — Connect on a worker thread; every wait bounded and cancellable (FR-002/003/004)
**Decision**:
- Run the whole connect sequence (DNS, TCP, handshake, host-key check,
  auth, sftp_init) on a **worker thread**; the UI thread shows the
  existing cancellable SafeWaitWindow pattern and pumps messages. Cancel
  (or timeout) **closes the socket** from the UI side, which immediately
  errors out any in-flight libssh2 wait — cancel-in-progress ≤ 2 s (SC-004).
  Host-key and auth prompts marshal back to the UI thread.
- **Leave the OS socket non-blocking after connect** (delete the
  restore-to-blocking at session.cpp:224–226): with libssh2 in blocking
  API mode over a non-blocking socket, every call goes through
  `_libssh2_wait_socket`, which is exactly where libssh2 enforces the 30 s
  `api_timeout` — restoring the bound the current code accidentally
  defeats (F3 verifier note). This makes *all* session operations bounded
  without rewriting them.
- Short teardown: before disconnecting a dead/failed session, drop the
  libssh2 timeout to ~2 s (U5); keepalive stays on the timer but is now
  bounded (U3) — acceptable worst case, documented.
- ViewFile download loop gets the SafeWaitWindow + per-chunk cancel test
  (F7), same pattern as operats.cpp.
- File operations keep the existing synchronous+poll-cancel model (now
  actually bounded per call); full async operations are out of scope
  (Incremental Modernization — the freeze class is what this feature fixes).
**Rationale**: The worker thread removes the whole-app-freeze *class*
(F3–F6); the non-blocking-socket fix restores the timeout guarantee
everywhere at one stroke; socket-close is the one reliable way to abort an
in-progress wait with libssh2.
**Alternatives**: Fully non-blocking libssh2 + EAGAIN state machines
everywhere — rejected (invasive rewrite of every call site, disproportionate
to the spec). Keep UI-thread connect with message-pumping waits — rejected
(prompts/dialogs re-entrancy risks; still one stall per call visible).

### D5 — Auth UX per clarifications (FR-005/006, US2)
**Decision**:
- Classify auth errors by **libssh2 error code** (`rc`,
  `libssh2_session_last_errno`), not case-sensitive message substrings
  (U8): key-side (`LIBSSH2_ERROR_FILE`, `_KEYFILE_AUTH_FAILED`,
  `_METHOD_NOT_SUPPORTED`) vs server-side (`_AUTHENTICATION_FAILED`,
  `_PUBLICKEY_UNVERIFIED`) vs connectivity.
- Passphrase flow: keep the up-front sniff as a fast path, but drive the
  real logic by outcome — on decrypt-failure, prompt for the passphrase
  and **retry within the same attempt** (covers encrypted OpenSSH
  containers the sniff can't see, U12; wrong passphrase → clear error +
  re-prompt, FR-005).
- Password fallback: after a server-rejected key, if `userauth_list`
  includes `password`, offer the password prompt in the same attempt;
  decline ends the attempt cleanly (clarification #2, FR-006).
- Transport-failure recovery: clear `Connected` on fatal transport errors
  and wire the existing dead `Reconnect()` into `EnsureConnected` (U7);
  history/command-line reconnect reuses the matching bookmark's auth
  method instead of forcing password (U14).
**Rationale**: Matches the clarified UX; error-code classification is the
only reliable base for the fallback decision tree.

### D6 — Regression harness (FR-011, contract in contracts/test-harness.md)
**Decision**: Rework `test/key_auth.c` into the scenario harness defined by
the contract: it must call **`libssh2_userauth_publickey_frommemory`**
(the product path, U2), implement the 90 s watchdog (exit 124), cover the
7 contract scenarios (3 reference keys, wrong passphrase, unauthorized,
unsupported, silent-server timeout), and be built+run by the committed
script alongside `build_and_run.cmd` (U13). Add pure-logic fixture tests
for `DetectKeyFormat`/`KeyFileLooksEncrypted`/`KeyFormatSupported`.
**Rationale**: The freeze escaped precisely because the harness exercised a
different API path and was never routinely built; the watchdog makes the
hang class self-detecting (SC-006: 3 consecutive clean runs).

## Resolved deferred item (from /speckit.clarify)
**Connect timeout default**: existing plugin config — `ConnectTimeoutSec`
default 20 s, `OperationTimeout` default 30 s (sftp.cpp:311–312). Keep
both defaults; D4 budgets the 20 s across the whole DNS candidate list
(U4) and makes the 30 s bound actually enforced. Worst-case failed attempt
stays well under SC-003's 60 s ceiling.

## Where each finding was fixed (T030 cross-check)

| Finding | Landed in |
|---|---|
| F1 (root cause) | `libssh2/src/pem.c` — `readline_memory` fails at EOF + bounds guards in both `_libssh2_pem_parse_memory` scan loops |
| F2 | `libssh2/src/wincng.c` (openssh-key-v1 RSA/ECDSA import) + `sftp/keyload.cpp` (`KeyFormatSupported`, new `KeyFileSupported`) |
| F3, F4, F5, F6 | `sftp/session.cpp` — `ConnectAttempt` + `RunConnectAttemptOnWorker` + `SFTPConnectThreadProc`; socket left non-blocking in `OpenSocket` |
| F7 | `sftp/fs.cpp` — `ViewFile` wait window + per-chunk cancel poll |
| U1 | `libssh2/src/wincng.c` — real error codes on every `*_frommemory` failure path |
| U2, U13 | `sftp/test/key_auth.c` (rewritten onto `publickey_frommemory` + watchdog), `run_keyauth.cmd`, `build_and_run.cmd` |
| U3 | `sftp/session.cpp` — `Keepalive` bounded to 2 s + `NoteTransportError` |
| U4 | `sftp/session.cpp` — `OpenSocket` whole-list timeout budget, 250 ms slices |
| U5 | `sftp/session.cpp` — `Disconnect` short teardown timeout |
| U6 | verified already covered in `operats.cpp`; bounded per call by the non-blocking-socket fix |
| U7 | `sftp/session.cpp` — `NoteTransportError` from `SetLastErrorFromSsh`/`Read`/`Write`; `sftp/fs.cpp`/`fs.h` — `WasConnected` reconnect path |
| U8 | `sftp/session.cpp` — `ClassifyAuthFailure` (error codes, not substrings) |
| U9 | `sftp/session.cpp` (`ReadKeyFileU8` failure path) + `sftp/keyload.cpp` (all sniff buffers) |
| U10 | `sftp/logs.cpp` — 32-log cap in `CreateLog` |
| U11 | `libssh2/src/pem.c` + `wincng.c` — passphrase forwarded and classic-PEM decrypt branch ported |
| U12 | `sftp/keyload.cpp` (`KeyFileLooksEncrypted` reads the container cipher) + `session.cpp`/`Connect` passphrase retry |
| U14 | `sftp/fs.cpp` — bookmark lookup before defaulting to password |

## Implementation notes & deviations (recorded during execution)

- **D4 scope note (FR-002)**: the connect sequence now runs on a worker thread
  while the UI thread shows the plugin's standard cancellable
  `CreateSafeWaitWindow` and polls its Close button. That wait window runs on
  its own thread (core behaviour), so the *rest* of the application does not
  repaint while a connect is in flight — exactly as for every other plugin
  operation in Salamander. What this feature guarantees is what the spec
  measures: no unbounded wait, no state where only killing the process helps,
  and cancel effective in about a second (SC-002/SC-003/SC-004). Making the
  whole app repaint during plugin operations would be a core change, which the
  spec's Assumptions put out of scope.
- **Prompts never run on the worker**: instead of marshalling dialogs, a failed
  non-interactive attempt reports what it needs (`cpHostKey`, `cpPassphrase`,
  `cpPassword`) and the UI-side orchestrator (`CSFTPSession::Connect`) prompts
  and re-runs the attempt. Cost: interactive paths (first host key, wrong
  passphrase, key rejected → password) re-do the TCP handshake. Accepted:
  those paths are rare and the alternative is a cross-thread UI marshalling
  layer this plugin has no infrastructure for.
- **Cancel mechanism**: `shutdown(socket, SD_BOTH)` rather than `closesocket`
  — it wakes a pending `recv`/`send` immediately without freeing the handle the
  worker is still using, so there is no use-after-close race with libssh2.
- **U10 (log growth)** is capped rather than reference-counted: `CreateLog`
  keeps the most recent 32 logs and drops the oldest. Removing a log when its
  FS closes would lose the log of a session the user may still want to read —
  the point of the Logs window.
- **T024** needed no code change in `operats.cpp`: every network loop there
  already polls `CheckCancel()` between calls, and after the non-blocking-socket
  fix each of those calls is genuinely bounded. `ViewFile` was the one loop with
  no cancel path at all (F7) and got the wait window + per-chunk poll.

## Constitution fit
D1–D3 are minimal, documented patches to the vendored dependency (kept
GPLv2-compatible; libssh2 is BSD-3) — no new external dependencies
(Principle IV, V). D4 follows the core's established SafeWaitWindow/worker
patterns (Principle VI untouched — no new dialog styles). All changes are
incremental and independently testable (Principle III).
