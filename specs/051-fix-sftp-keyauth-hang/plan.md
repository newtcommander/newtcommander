# Implementation Plan: Fix SFTP Private-Key Authentication Hang & Stabilize SFTP Plugin

**Branch**: `051-fix-sftp-keyauth-hang` | **Date**: 2026-08-05 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/051-fix-sftp-keyauth-hang/spec.md`

## Summary

Connecting with a private key (reference: unencrypted RSA-4096, OpenSSH
format) freezes the whole application. A 12-agent audit (5 independent
analysis lenses + adversarial verification; 7 confirmed findings, 0 refuted)
established the root cause: the vendored libssh2's `_libssh2_pem_parse_memory`
spins forever when the expected PEM header is absent (`readline_memory`
cannot signal EOF), the plugin's key-format gate routes OpenSSH-container
keys into that loop (the WinCNG memory loader only parses classic RSA/DSA
PEM), and the CPU spin lands on the main UI thread because the whole connect
sequence runs there — no worker thread exists. The fix: patch the vendored
PEM reader (bounds guards), add OpenSSH-container RSA/ECDSA import and
classic-PEM passphrase decryption to the WinCNG memory path (all three
release-blocking keys work), move connect/auth to a cancellable worker
thread and keep the socket non-blocking so libssh2's 30 s timeout is
actually enforced, implement the clarified auth UX (error-code
classification, passphrase retry, password fallback), and rebuild the test
harness around the product's real API path with a hang watchdog.
*(Full findings & decisions D1–D6: [research.md](research.md).)*

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022); test harness in C
**Primary Dependencies**: vendored libssh2 1.11.1_DEV (`src/common/dep/libssh2`) built with `LIBSSH2_WINCNG` + `LIBSSH2_ECDSA_WINCNG` (Windows CNG crypto backend — RSA & ECDSA only, no ed25519, no OpenSSL); pure WinAPI UI
**Storage**: Windows Registry (`HKCU\Software\Tandem Commander\0.1`) for connection profiles; private key files read from disk at connect time
**Testing**: plugin test harness `src/plugins/sftp/test/` (`key_auth.c`, `sftp_smoke.c`) extended per FR-011; manual failure-mode matrix from spec SC-002 against reference server (Docker `tandem-sftp`, localhost:2222, user `tctest`; keys in `C:\Users\pavel\.ssh\tandem-sftp-test\`)
**Target Platform**: Windows 11+ x64
**Project Type**: desktop-app plugin (`sftp.spl` + `english.slg`, ~7,100 lines across 10 .cpp modules)
**Performance Goals**: key-auth connect < 10 s against reference server (SC-001); failed attempts surface an error ≤ 60 s (SC-003)
**Constraints**: UI thread must never block on network I/O (SC-002 zero freezes); user cancel effective ≤ 2 s in every phase (SC-004); fixes confined to the plugin (spec Assumptions)
**Scale/Scope**: single plugin + its test harness; release-blocking key matrix = RSA-4096 OpenSSH, ECDSA P-256, passphrase RSA PEM

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| # | Principle | Gate result |
|---|-----------|-------------|
| I | Build Reproducibility | PASS — no build-pipeline changes; test-harness build stays scripted inside the existing plugin vcxproj/test setup; no manual steps introduced |
| II | Backward Compatibility | PASS — bug fix; existing profiles and password auth keep working unchanged; password fallback is additive within a failed key attempt; plugin ABI (interface 105) untouched |
| III | Incremental Modernization | PASS — targeted fixes in the code being touched; no wholesale refactor of untouched modules; audit findings outside hang/crash/error family are deferred, not bundled |
| IV | Windows Platform Commitment | PASS — pure WinAPI, no new dependencies; libssh2 stays on the WinCNG backend |
| V | Plugin Architecture Preservation | PASS — all changes inside the sftp plugin; plugin interfaces unmodified |
| VI | UI Consistency | PASS with note — any new/changed prompt (passphrase, password fallback) reuses the plugin's existing `DIALOGEX`/`DS_SHELLFONT` dialog patterns; no process-wide visual side effects (`ICC_STANDARD_CLASSES` prohibition respected) |

**Post-design re-check** (after Phase 1): all gates still PASS — design adds no
new projects, dependencies, or identity/UI deviations; the worker-thread
connect model follows the core's existing plugin patterns.

## Project Structure

### Documentation (this feature)

```text
specs/051-fix-sftp-keyauth-hang/
├── plan.md              # This file
├── research.md          # Phase 0: audit findings + root cause (multi-agent)
├── data-model.md        # Phase 1: entities & state machines
├── quickstart.md        # Phase 1: reference environment + verification runbook
├── contracts/
│   └── test-harness.md  # Phase 1: regression scenario contract (FR-011)
└── tasks.md             # Phase 2 (/speckit.tasks — not created here)
```

### Source Code (repository root)

```text
src/plugins/sftp/
├── sftp.cpp / sftp.h          # plugin entry, menu, config
├── fs.cpp / fs.h              # panel FS integration, connect entry points
├── session.cpp / session.h    # SSH/SFTP session: connect, handshake, AUTH (focus)
├── keyload.cpp / keyload.h    # key format detection/validation (focus)
├── dialogs.cpp / dialogs.h    # connection/passphrase/password dialogs (focus)
├── operats.cpp / operats.h    # file operations over sftp
├── listing.cpp / listing.h    # directory listing
├── logs.cpp / logs.h          # plugin log window
├── sftputils.cpp / sftputils.h
├── hostkeys.cpp / hostkeys.h  # host key verification store
└── test/
    ├── key_auth.c             # extended: key-auth regression scenarios (FR-011)
    └── sftp_smoke.c           # existing smoke test

src/common/dep/libssh2/        # vendored 1.11.1_DEV — read-only reference;
                               # patched ONLY if root cause demands (documented)
```

**Structure Decision**: single existing plugin; no new projects. Fix lands in
`session.cpp` / `keyload.cpp` / `dialogs.cpp` / `fs.cpp` as the audit dictates;
test harness grows in place under `test/`.

## Complexity Tracking

No constitution violations — table not needed.

## Implementation status

| Area | Files | State |
|---|---|---|
| Root-cause PEM guard (D1) | `libssh2/src/pem.c`, `libssh2/readme.txt` | done — freeze eliminated (verified: 78 ms clean failure vs. previous infinite hang) |
| OpenSSH-container + encrypted-PEM key support (D2/D3) | `libssh2/src/wincng.c`, `pem.c` | done — see harness results in quickstart.md |
| Capability gate + up-front rejection (F2/FR-007) | `sftp/keyload.cpp/.h` | done — PKCS#8 and ed25519 now rejected with a remedy message |
| Error classification, passphrase retry, password fallback (D5) | `sftp/session.cpp` | done |
| Worker-thread connect + cancel + enforced timeouts (D4) | `sftp/session.cpp/.h`, `fs.cpp` | done — prompts stay on the UI thread via the `cpHostKey`/`cpPassphrase`/`cpPassword` handshake |
| Session stabilization (U3–U7, U9, U10, U14, F7) | `sftp/session.cpp`, `fs.cpp/.h`, `logs.cpp` | done |
| Regression harness + fixtures (D6/FR-011) | `sftp/test/key_auth.c`, `run_keyauth.cmd`, `harness.cpp`, `build_and_run.cmd`, `precomp_shim.h` | done |
| Manual matrix (T012/T020/T028) | — | pending — requires an interactive session with the built app |
