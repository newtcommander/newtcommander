# Data Model: Fix SFTP Private-Key Authentication Hang

**Feature**: 051-fix-sftp-keyauth-hang | **Date**: 2026-08-05

## Entities

### Connection Profile
User-entered connection definition, persisted in the registry with the
plugin's existing configuration.

| Field | Type | Rules |
|---|---|---|
| host | string | non-empty |
| port | int | 1–65535, default 22 |
| user | string | non-empty |
| auth_method | enum { password, private_key } | — |
| key_file_path | string (UTF-8) | required iff auth_method = private_key; validated (exists, readable, supported format) BEFORE any network activity (FR-007) |

### Private Key File
Local file referenced by a profile; read at connect time by `keyload.cpp`.

| Attribute | Values | Rules |
|---|---|---|
| format | OpenSSH \| PEM \| PKCS#8 \| PuTTY \| unknown | PuTTY/unknown → rejected up front with remedy message (FR-007) |
| key_type | RSA \| ECDSA | ed25519 not supported by WinCNG backend — must produce a clear up-front error, never an attempt (spec: out of scope, clear message) |
| encrypted | bool | true → passphrase prompt before/at auth (FR-005) |

Release-blocking configurations (FR-001): RSA-4096/OpenSSH, ECDSA-P256/OpenSSH,
RSA-3072/PEM+passphrase.

### SFTP Session
Live authenticated connection bound to a panel; owns socket + libssh2
session/sftp handles. All state transitions must be bounded in time and
cancellable (FR-002/003/004).

## Session State Machine

```text
IDLE ──connect──▶ TCP_CONNECTING ──▶ HANDSHAKE ──▶ HOSTKEY_CHECK ──▶ AUTH ──▶ READY
                     │                  │              │               │        │
                     │                  │              │               │        ├─ op error ─▶ READY (error reported)
                     │                  │              │               │        └─ net drop ─▶ FAILED (FR-009)
                     └──────────────────┴──────────────┴───────────────┴─ cancel ≤2 s / timeout / error ─▶ FAILED ─▶ IDLE
```

**AUTH sub-states** (clarified UX):

```text
AUTH ─ key configured ─▶ KEY_VALIDATE (local; keyload.cpp)
         │ unsupported/unreadable ──▶ FAIL(clear message, no network attempt)   [FR-007]
         ▼
       KEY_AUTH (libssh2 publickey)
         │ encrypted key ──▶ PASSPHRASE_PROMPT ──ok──▶ KEY_AUTH  (wrong → error + retry) [FR-005]
         │ server rejects key & server allows password
         ▼
       PASSWORD_FALLBACK_PROMPT ──accept──▶ PASSWORD_AUTH ──▶ READY | FAIL
         └─ decline ──▶ FAIL (clean end of attempt)                              [FR-006]
```

**Invariants**:

- I1: No state may be entered on the UI thread if it performs network I/O;
  the UI thread must keep pumping messages in every state (FR-002 / SC-002).
- I2: Every non-IDLE/READY state has (a) an upper time bound and (b) a cancel
  transition effective ≤ 2 s (FR-003/004, SC-003/004).
- I3: FAILED → IDLE releases every resource (socket, libssh2 handles, worker
  thread, progress UI); a subsequent connect starts from a clean slate (FR-008/009).
- I4: Auth outcome messages distinguish authentication failure from
  connectivity failure (FR-006).
