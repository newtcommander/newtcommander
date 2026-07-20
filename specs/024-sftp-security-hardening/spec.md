# Feature Specification: SFTP Plugin — Security & Functional Review, Handshake Fix, Unicode Dialog Fix

**Feature Branch**: `024-sftp-security-hardening`
**Created**: 2026-07-20
**Status**: Draft
**Input**: User: Perform an overall security and functional review of the SFTP
module. The result is both fixing identified bugs and improving the stability
and above all the security of the whole plugin. This is a fundamental plugin
in which file handling and overall security are especially important. The
critical bug hit today: could not connect to an SSH server using **password**
login — got "SSH handshake failed: %s: Unable to exchange encryption keys". A
second, less critical bug: the file name is displayed incorrectly (mojibake) in
the pop-up when setting attributes / changing the owner — again the special
characters (e.g. emoji) encoding problem we solved before. Start by using
several agents to analyze the security and quality of the whole codebase, then
consolidate all findings and fix everything. Work fully autonomously through
implementation and the tests you can run.

## Problem Statement

The SFTP plugin (features 008/009/017/018) is a security-critical component: it
authenticates to remote servers, handles credentials, verifies host keys, and
transfers files. Two concrete defects are reported, and the request is a broad
security + quality review of the whole plugin with all findings fixed.

1. **Cannot connect — handshake fails at key exchange (Critical).** With
   password authentication the connection dies during the SSH handshake with
   *"Unable to exchange encryption keys"*. Root cause (confirmed from the
   libssh2 source): that exact message is emitted by `session.c` when
   `_libssh2_kex_exchange` returns `LIBSSH2_ERROR_KEX_FAILURE` out of
   `kex_agree_methods` — i.e. **algorithm negotiation failed: the client and
   server share no common key-exchange or host-key algorithm** (a DH
   *computation* failure would instead say "Unrecoverable error exchanging
   keys"). The vendored libssh2 WinCNG build has `LIBSSH2_ECDSA = 0` (because
   `LIBSSH2_ECDSA_WINCNG` is **not** defined — `sftp.props` defines only
   `LIBSSH2_WINCNG`) and `LIBSSH2_ED25519 = 0`. So the client offers only
   legacy `diffie-hellman-group*` key exchange and only RSA/DSA host keys — no
   `ecdh-sha2-nistp256/384/521`, no `curve25519-sha256`, and no `ecdsa`/
   `ed25519` host-key verification. Modern OpenSSH servers that present an
   Ed25519-only host key, or restrict `KexAlgorithms` to curve25519/ecdh,
   produce exactly this negotiation failure. This is simultaneously a
   **functional** bug (cannot connect to a large and growing share of servers)
   and a **security** posture problem (the client is pinned to the weakest end
   of the algorithm spectrum).

2. **Mojibake of the file name in the change-owner/group dialog (Medium).** The
   change-owner/group pop-up (feature 018) shows the target file name garbled
   for non-ACP characters (accents, CJK, emoji). Root cause (confirmed):
   `dialogs.cpp` `OwnerGroupProc` (`WM_INITDIALOG`) sets the target label with
   `SetDlgItemTextA(hwnd, IDT_OG_TARGET, d->Label)` — the ANSI setter — whereas
   the label is a UTF-8 server name. The chmod (attributes) dialog already uses
   the correct UTF-8 helper `SetDlgItemTextU8`; the owner/group dialog was
   missed. This is the same class of bug fixed in features 010/015 elsewhere.

3. **Broader hardening.** Beyond the two reported bugs, a multi-agent audit
   (crypto/transport, authentication & secrets, file-operations & path handling,
   memory safety & C++ quality, encoding/UI) consolidates additional defects to
   fix — with a bias toward correctness- and security-relevant issues over
   style. See `research.md` for the consolidated, verified list.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Password login connects to a modern SSH server (Priority: P1)

A user opens the SFTP connect window, enters host / user / password, and
connects to a current OpenSSH server (one that offers curve25519/ecdh key
exchange and presents an Ed25519 or ECDSA host key). The handshake completes and
the remote directory is listed.

**Why this priority**: Directly reported and fully blocking — the plugin cannot
be used at all against a large fraction of real-world servers.

**Independent Test**: Against an OpenSSH ≥ 9.x server, connect with password
auth; the handshake succeeds and a listing appears. (Because a live server is
required and the environment has none available headless, the autonomous proof
is: the build now compiles ECDSA in, the `libssh2_kex_methods[]` set now
includes `ecdh-sha2-nistp256/384/521`, host-key verification accepts
`ecdsa-sha2-*`, and a transport-level unit/harness check confirms the negotiated
algorithm set. The final live connect is the user's.)

**Acceptance Scenarios**:

1. **Given** a server offering `ecdh-sha2-nistp256` (and not the legacy DH
   groups), **When** the client negotiates the handshake, **Then** a common KEX
   algorithm exists and the handshake proceeds past key exchange.
2. **Given** a server presenting an `ecdsa-sha2-nistp256` host key, **When** the
   client verifies the host key, **Then** the key type is supported and the
   TOFU trust flow runs (accept / compare / reject) instead of a negotiation
   failure.
3. **Given** a legacy server that only offers `diffie-hellman-group14-sha256`,
   **When** the client connects, **Then** it still works (no regression — the DH
   group methods remain available).

---

### User Story 2 - File names display correctly in the owner/group dialog (Priority: P2)

A user right-clicks a file whose name contains non-ASCII characters (e.g. an
emoji), chooses Change Owner/Group, and the pop-up shows the **correct** file
name in the target label.

**Why this priority**: Reported; cosmetic but erodes trust and can mislead the
user about which file they are about to modify.

**Independent Test**: Statically confirm the label is set through the UTF-8
path (`SetDlgItemTextU8`); optionally verify at runtime with an emoji-named
file that the dialog title/label renders correctly.

**Acceptance Scenarios**:

1. **Given** a file named with characters outside the system ANSI code page,
   **When** the change-owner/group dialog opens, **Then** the target label shows
   the exact name, not mojibake.
2. **Given** the chmod (attributes) dialog, **When** it opens for the same file,
   **Then** it continues to render the name correctly (no regression).

---

### User Story 3 - Credentials are handled safely and predictably (Priority: P2)

Saved passwords/passphrases are never silently corrupted or leaked, and
non-ASCII passwords are handled consistently.

**Acceptance Scenarios**:

1. **Given** a bookmark with a stored password shown as the `********`
   placeholder, **When** the user connects without retyping, **Then** the stored
   blob is used and never overwritten by the placeholder text.
2. **Given** an upload targeted at `sftp:otheruser@host/...`, **When** the
   current session is authenticated as a different user, **Then** the operation
   is not silently run under the wrong identity.
3. **Given** the connect dialog is cancelled after fields were read, **When** the
   dialog closes, **Then** no decrypted secret lingers in a process-global
   buffer.

---

### Edge Cases

- Legacy servers offering only DH-group KEX and an RSA host key must keep
  working (no regression from enabling ECDSA).
- ECDSA in the WinCNG backend requires Windows 10+ (`BCryptDeriveKey` with
  `BCRYPT_KDF_RAW_SECRET`); the project targets Windows 11, so this is
  satisfied — but the `WINVER`/`_WIN32_WINNT` macros in `sftp.props` currently
  say `0x0601` (Win7) and must not undercut the requirement.
- Ed25519/curve25519 is **not** implemented by this libssh2 WinCNG backend
  (`wincng.c` has no `_libssh2_curve25519_*`); enabling it is out of scope for
  this fix. ECDSA (ecdh + ecdsa host keys) is the interoperability floor that
  resolves the reported failure, since every server offering curve25519 also
  offers ecdh-sha2-nistp256 by default.
- Non-ACP characters in a password or key passphrase.
- A malicious server returning names with `..`, path separators, or control
  characters must not escape the local download directory.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The SSH client MUST negotiate successfully with modern OpenSSH
  servers that offer `ecdh-sha2-nistp256/384/521` key exchange and/or present
  `ecdsa-sha2-nistp256/384/521` host keys. Concretely, the vendored libssh2
  WinCNG build MUST be compiled with `LIBSSH2_ECDSA_WINCNG` defined so the
  `ecdh-sha2-*` KEX methods and `ecdsa-sha2-*` host-key algorithms are present.
- **FR-002**: The client MUST continue to interoperate with servers that only
  offer legacy `diffie-hellman-group14-sha256`/`group16`/`group18` KEX and
  RSA host keys (no regression).
- **FR-003**: The plugin's `WINVER`/`_WIN32_WINNT` build macros MUST be
  consistent with the Windows 10+/11 requirement of the enabled crypto (raise
  from `0x0601`), without breaking the existing build.
- **FR-004**: The change-owner/group dialog MUST display the target file name
  via the UTF-8 path (`SetDlgItemTextU8`) so non-ACP names render correctly.
- **FR-005**: All other places in the plugin's UI that display a server-provided
  (UTF-8) name/path via an ANSI/ACP call MUST be corrected to the UTF-8 path
  (as consolidated in `research.md`), including reading passwords/passphrases
  from edit controls where non-ACP characters are currently mangled.
- **FR-006**: The stored-secret placeholder (`********`) MUST never be used as
  or written over a real password/passphrase; the "typed a new secret" decision
  MUST exclude the placeholder value.
- **FR-007**: An upload/copy target FS path MUST match the current session's
  user as well as host and port before being executed on that session.
- **FR-008**: Decrypted secrets MUST NOT linger in process-global buffers after
  the connect dialog is cancelled or the connect flow aborts.
- **FR-009**: All additional defects confirmed by the audit and recorded in
  `research.md` (memory-safety, path-handling, resource-leak, error-handling,
  and encoding issues rated Medium or above, plus low-risk quick wins) MUST be
  fixed, with no regression to connecting, listing, or transfers.
- **FR-010**: Host-key verification MUST remain safe: no unknown or changed host
  key is ever accepted without explicit user interaction, and the full key blob
  (not just a truncated fingerprint) remains the basis of comparison.

### Key Entities

- **libssh2 WinCNG crypto configuration** (`sftp.props`, `wincng.h`,
  `crypto_config.h`): the compile-time algorithm set. Enabling
  `LIBSSH2_ECDSA_WINCNG` unlocks ecdh KEX + ecdsa host keys.
- **Owner/Group dialog** (`dialogs.cpp` `OwnerGroupProc`): the mojibake site.
- **Credential flow** (`dialogs.cpp` connect dialog, `session.cpp`
  `Authenticate`, `sftp.cpp` config load/save): passwords, passphrases,
  placeholder handling, secret memory hygiene.
- **File-system interface** (`fs.cpp`, `operats.cpp`): path building, target
  validation, download sanitization.

## Success Criteria *(mandatory)*

- **SC-001**: After the fix, the client's compiled KEX method set includes
  `ecdh-sha2-nistp256/384/521` and host-key verification accepts `ecdsa-sha2-*`
  — verifiable statically (compile flags / `libssh2_kex_methods[]`) and, where a
  server is available, by a successful password-auth connect to a modern server.
- **SC-002**: The legacy DH-group + RSA path still connects (no regression).
- **SC-003**: The change-owner/group dialog shows an emoji/CJK file name
  correctly (mojibake gone); the chmod dialog is unchanged.
- **SC-004**: Every Medium+ audit finding in `research.md` is fixed or has a
  documented, justified deferral; no secret is corrupted by the placeholder, no
  cross-user upload, no lingering plaintext on cancel.
- **SC-005**: Debug x64 (and Release x64 where the running-exe lock allows)
  build clean; the SFTP dev test harness builds and passes; no regression in
  connect/list/transfer behavior that can be verified without a live server.

## Assumptions

- Builds on features 008/009/010/015/017/018 (working transport, transfers,
  host-key store, UTF-8 control text, connect UX, context menu). The scope is
  the transport crypto configuration, the reported dialog encoding bug, and the
  consolidated audit findings — not a rewrite of the SSH/transfer layers or a
  libssh2 version bump.
- Ed25519/curve25519 support is out of scope because the vendored libssh2
  WinCNG backend does not implement it; ECDSA is the correct, sufficient,
  low-risk interoperability fix for the reported handshake failure.
- Live end-to-end connect verification against the user's server is the user's
  final test; autonomous verification is by build, static algorithm-set
  confirmation, the test harness, and code review.
- The Salamander password-manager API is the correct secret mechanism; audit
  fixes address how the plugin *uses* it, not the manager itself.
