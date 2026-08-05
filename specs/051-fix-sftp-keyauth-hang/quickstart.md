# Quickstart: Reference Environment & Verification Runbook

**Feature**: 051-fix-sftp-keyauth-hang | **Date**: 2026-08-05

## Reference SFTP server (local, Docker)

Container `tandem-sftp` (image `tandem-sftp-test`: Ubuntu 24.04 + OpenSSH 9.6,
`--restart unless-stopped`), reachable at **localhost:2222**.

```powershell
docker start tandem-sftp      # if not running (Docker Desktop must be up)
docker ps --filter name=tandem-sftp
```

| Credential | Value |
|---|---|
| user / password | `tctest` / `tandem123` |
| RSA-4096 key (OpenSSH, no passphrase) | `C:\Users\pavel\.ssh\tandem-sftp-test\tctest_rsa` |
| ECDSA P-256 key (no passphrase) | `C:\Users\pavel\.ssh\tandem-sftp-test\tctest_ecdsa` |
| RSA-3072 key (classic PEM, passphrase `tandem123`) | `C:\Users\pavel\.ssh\tandem-sftp-test\tctest_rsa_pass` |

All three public keys are in the container's `authorized_keys`. Server-side
sanity check (bypasses the plugin):

```powershell
ssh -i $env:USERPROFILE\.ssh\tandem-sftp-test\tctest_rsa -p 2222 -o BatchMode=yes tctest@127.0.0.1 whoami   # → tctest
```

Test data in the remote home: `readme.txt`, `docs/` (1 MB binary),
`large-10mb.dat`, `empty-dir/`, `empty-file.txt`, `nested/` (50 files, 3
levels), `dir with spaces/`, `čeština-ěščřž.txt`, `link-to-readme` (symlink),
`broken-link` (dangling), `no-read-access.txt` (chmod 000).

**Failure-mode servers** (for the matrix):
- Unreachable: connect to `localhost:2223` (nothing listens).
- Silent (accepts TCP, never speaks SSH): `docker exec -d tandem-sftp bash -c "nc -lkp 4444"` → connect to a port published ad hoc, or simpler: `Test-NetConnection`-style dead port + firewall DROP variant via stopping the container mid-handshake.
- Unauthorized key: generate a throwaway key not present in `authorized_keys`.
- Mid-session drop: `docker stop tandem-sftp` while a transfer runs.

## Build & run the plugin

```batch
build.cmd                :: Debug x64 incremental (repo root)
build.cmd full           :: + runtime data & plugins.ver
```

Binary: `%OPENSAL_BUILD_DIR%\...` (defaults to `.\build\`); run
`tandemcommander.exe`, plugin `sftp.spl` auto-registers on a full build.

## Automated regression suite (FR-011)

Location: `src/plugins/sftp/test/` (`key_auth.c` extended + watchdog).
Scenarios and pass criteria: see [contracts/test-harness.md](contracts/test-harness.md).
Release criterion: 3 consecutive clean runs against the reference server (SC-006).

## Manual verification matrix (SC-002)

Each row: application must never freeze; failures end in a clear message.

| # | Scenario | Expected |
|---|---|---|
| 1 | Connect with `tctest_rsa` | session + listing < 10 s, no password prompt |
| 2 | Connect with `tctest_ecdsa` | as above |
| 3 | Connect with `tctest_rsa_pass`, correct passphrase | prompt → session |
| 4 | `tctest_rsa_pass`, wrong passphrase (×2) | clear error, retry allowed, no freeze |
| 5 | Unauthorized key | auth-failure message + password fallback offer; decline ends cleanly |
| 6 | ed25519 / `.ppk` / missing / unreadable key file | up-front rejection message, no network attempt |
| 7 | Unreachable port 2223 | connectivity error ≤ 60 s |
| 8 | Cancel during connect (each phase) | effective ≤ 2 s, panel restored |
| 9 | `docker stop` mid-transfer | operation error, app responsive, reconnect works |
| 10 | Exit app during pending connect | clean shutdown |
| 11 | Browse `nested/`, symlinks, `no-read-access.txt`, UTF-8 names | rendered, no crash |
| 12 | Same ops via password auth | identical outcomes (FR-010) |

## Verification results

### Automated

| Suite | Command | Result |
|---|---|---|
| Pure-logic harness (incl. key-format fixtures) | `src\plugins\sftp\test\build_and_run.cmd` | **66 passed / 0 failed** |
| Key-auth regression harness | `src\plugins\sftp\test\run_keyauth.cmd` | **7 passed / 0 failed**, exit 0 |

```
PASS key-rsa 78
PASS key-ecdsa 109
PASS key-passphrase 79
PASS key-passphrase-wrong 78 rejected as expected (err=-48)
PASS key-unauthorized 203 server rejected key (err=-18)
PASS key-unsupported 172 both rejected (ed25519 err=-33)
PASS timeout-silent 16015 bounded failure in 15000 ms
TOTAL 7 passed / 0 failed
```

The error codes matter as much as the verdicts: a wrong passphrase reports −48
(`KEYFILE_AUTH_FAILED`, key-side) and never a server verdict; an unauthorized
key parses cleanly and gets −18 (`AUTHENTICATION_FAILED`, server-side); ed25519
fails immediately with −33 (`METHOD_NOT_SUPPORTED`). That separation is what the
plugin's fallback decision tree relies on (FR-006).

Baseline before the fix (recorded for the record): `key-rsa` **hung forever** —
the application had to be killed. After the `pem.c` bounds-guard patch alone the
same scenario failed in **78 ms** with a clean libssh2 error, which is the
Phase-2 checkpoint: the freeze class is gone even when a key is unusable.

### Build

`build.cmd full` → **BUILD SUCCEEDED**, 180 language modules (20 modules × 8
enabled languages), no C++ warnings introduced.

New UI strings change the resource layout, and `.slt` import is positional, so
the language build fails loudly until the translation source is refreshed — it
did, for all 8 languages. The documented two-stage flow fixes it:

```bat
src\vcxproj\build_langs.cmd --export-templates --module sftp
cd tools && python -m translate.merge --module sftp
```

That run sent 3,901 characters to DeepL, reported 0 validation failures, and
left every pre-existing entry untouched (only the 5 new strings were
translated). This is worth knowing for any future feature that adds strings:
the failure is a guardrail, not a defect.

### Manual

Rows 1–12 of the matrix above are run against the built app; record outcomes
here when executing T012 / T020 / T028.
