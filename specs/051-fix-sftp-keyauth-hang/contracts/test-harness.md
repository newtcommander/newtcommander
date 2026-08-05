# Contract: SFTP Key-Auth Regression Harness

**Feature**: 051-fix-sftp-keyauth-hang | **Consumers**: developer runs, future CI

The harness (`src/plugins/sftp/test/`) is a console C program linking the same
vendored libssh2 (WinCNG backend, same defines as the plugin) plus, where the
scenario targets plugin logic, the plugin's key-handling translation units.
It exercises the code paths of this fix against the local reference server.

## Invocation

```text
sftp_keyauth_test.exe [--host 127.0.0.1] [--port 2222] [--user tctest]
                      [--keydir <dir>] [--scenario <name>|all]
```

Defaults target the reference environment; `--keydir` defaults to
`%USERPROFILE%\.ssh\tandem-sftp-test`.

## Scenarios (each REQUIRED, FR-011)

| Scenario | Input | Pass criteria |
|---|---|---|
| `key-rsa` | `tctest_rsa` | auth succeeds, SFTP realpath("."), clean shutdown |
| `key-ecdsa` | `tctest_ecdsa` | as `key-rsa` |
| `key-passphrase` | `tctest_rsa_pass` + `tandem123` | auth succeeds |
| `key-passphrase-wrong` | `tctest_rsa_pass` + `wrong` | auth FAILS with key-decrypt error; no hang; distinct error surfaced |
| `key-unauthorized` | fresh throwaway key (generated per run) | server rejects; harness reports auth-failure (not connectivity) |
| `key-unsupported` | ed25519 key + `.ppk` stub file | rejected locally by format/type validation, exit before any socket I/O |
| `timeout-silent` | TCP endpoint that never speaks SSH | bounded failure ≤ configured timeout |

## Watchdog (hang detection — the defect this feature fixes)

- The harness runs every scenario under a watchdog thread; if a scenario
  exceeds **90 s** wall clock it hard-terminates the process with exit code
  **124** and prints `WATCHDOG: <scenario> hung`.
- A hang is a FAIL — never a skip.

## Exit codes

| Code | Meaning |
|---|---|
| 0 | all requested scenarios passed |
| 1 | ≥1 scenario failed (assertion) |
| 124 | watchdog fired (hang) |
| 2 | environment unusable (server unreachable at startup) — distinct so CI can flag infra vs regression |

## Output

One line per scenario: `PASS|FAIL <scenario> <ms> [detail]`; final summary
line `TOTAL passed/failed`. Machine-parsable, no interactive input ever
(passphrases passed programmatically).

## Release criterion (SC-006)

`--scenario all` → exit 0, three consecutive runs.
