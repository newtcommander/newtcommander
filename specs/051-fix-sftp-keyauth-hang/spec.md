# Feature Specification: Fix SFTP Private-Key Authentication Hang & Stabilize SFTP Plugin

**Feature Branch**: `051-fix-sftp-keyauth-hang`
**Created**: 2026-08-05
**Status**: Draft
**Input**: User description: "Kdyz dam pripojeni pomoci klice tctest_rsa, tak to nefunguje, cely program se zasekne a musim jej restartovat. Alokuj nekolik nezavislych agentu a proved analyzu celeho kodu SFTP pluginu s cilem opravy popsane chyby a celkove stabilizace chovani."

## Problem Statement

Connecting to an SFTP server using private-key authentication (reference key:
`tctest_rsa`, an unencrypted RSA 4096 key in OpenSSH format) does not work.
Worse than merely failing, the entire application freezes and must be killed
and restarted. The same key authenticates successfully against the same server
with a standard command-line SFTP client, so the defect is on the client
(plugin) side.

Beyond fixing this specific defect, the goal is an overall stabilization of
the SFTP plugin: a connection or session problem must never take down or
freeze the whole application — the worst acceptable outcome of any SFTP
operation is a clear error message and a usable application.

## Clarifications

### Session 2026-08-05

- Q: Má být součástí dodávky automatizovaný regresní test klíčové autentizace? → A: Rozšířit existující automatizovaný harness pluginu o scénáře této opravy (úspěšný key-auth, neautorizovaný klíč, passphrase, timeout/hang watchdog) spouštěné proti lokálnímu referenčnímu serveru.
- Q: Co má plugin udělat, když server nabídnutý klíč odmítne, ale povoluje i přihlášení heslem? → A: Zobrazit srozumitelnou hlášku o odmítnutí klíče a nabídnout zadání hesla, pokud ho server povoluje (fallback jako u běžných SFTP klientů).
- Q: Které typy klíčů musí projít release-blocking test maticí? → A: Všechny tři referenční klíče — RSA 4096 (OpenSSH formát), ECDSA P-256 a passphrase-chráněný RSA (PEM). ed25519 zůstává nepodporovaný, mimo rozsah.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Connect with a private key (Priority: P1)

A user configures an SFTP connection to use their private key file
(unencrypted RSA key, OpenSSH format), connects, and browses the remote
directory — exactly as they can today with password authentication.

**Why this priority**: This is the reported defect. Key-based login is the
standard authentication method for SFTP; today it not only fails but freezes
the application, making the feature unusable and destroying trust.

**Independent Test**: Against the local reference test server, configure a
connection with the `tctest_rsa` key, connect, and verify the remote home
directory listing appears.

**Acceptance Scenarios**:

1. **Given** a reachable SFTP server that accepts the user's public key,
   **When** the user connects using the matching private key file,
   **Then** the session is established and the remote directory listing is
   shown, with no password prompt.
2. **Given** an established key-authenticated session, **When** the user
   browses folders, downloads and uploads files, **Then** all operations
   behave identically to a password-authenticated session.
3. **Given** a passphrase-protected private key, **When** the user connects,
   **Then** they are prompted for the passphrase and the connection proceeds
   after a correct entry.

---

### User Story 2 - The application never freezes on a failed connection (Priority: P1)

Whatever goes wrong during an SFTP connection attempt — bad key, wrong
passphrase, rejected authentication, unreachable server, protocol error —
the user gets a clear error message, can cancel a pending attempt at any
time, and continues working in the application without restarting it.

**Why this priority**: The freeze is the most damaging part of the reported
defect. An application-wide hang requiring a forced restart is unacceptable
regardless of the underlying cause; failure must always be graceful.

**Independent Test**: Deliberately trigger failure modes (key not authorized
on the server, wrong passphrase, server port closed, cable-pull mid-connect)
and verify each one ends with an error dialog and a fully responsive
application.

**Acceptance Scenarios**:

1. **Given** a private key the server does not accept, **When** the user
   connects, **Then** an authentication-failure message is shown — with a
   password prompt offered as fallback when the server permits password
   login — and the application remains fully usable whether the user
   accepts or declines.
2. **Given** a connection attempt in progress, **When** the user cancels it,
   **Then** the attempt aborts promptly and the panel returns to its
   previous location.
3. **Given** a server that stops responding mid-handshake, **When** the
   user waits, **Then** the attempt times out with an error message instead
   of waiting indefinitely.
4. **Given** any failed connection attempt, **When** the user retries or
   switches to another panel/path, **Then** no restart is needed and other
   open sessions are unaffected.

---

### User Story 3 - Stable day-to-day SFTP sessions (Priority: P2)

A user works in a long-lived SFTP session — listing large directories,
transferring files, reconnecting after network interruptions — and the
plugin behaves predictably: errors are reported, sessions can be closed and
reopened, and no operation can wedge the application.

**Why this priority**: The user explicitly asked for overall behavioral
stabilization of the plugin beyond the single reported bug. Robustness
defects of the same family (hangs, missing timeouts, unreported errors)
should be found and fixed in one pass.

**Independent Test**: Run a session soak against the reference server:
directory listings (including 50-file and nested trees), up/downloads
(including a 10 MB file), renames, deletes, symlink and no-permission
entries, forced network drop, reconnect — verifying the application stays
responsive and every failure surfaces as a message.

**Acceptance Scenarios**:

1. **Given** an established session, **When** the network connection drops
   mid-operation, **Then** the operation ends with an error message and the
   application remains responsive.
2. **Given** remote entries with unusual properties (symlinks, broken
   symlinks, no-read-permission files, non-ASCII names), **When** the user
   lists and manipulates them, **Then** the plugin renders and handles them
   without crashes or hangs.
3. **Given** a closed or failed session, **When** the user reconnects,
   **Then** a fresh session is established without residue from the
   previous one (no leaked state, no stuck progress windows).

---

### Edge Cases

- Passphrase-protected key + wrong passphrase entered (once, and repeatedly).
- Key file missing, unreadable, empty, or in an unsupported format
  (e.g. PuTTY `.ppk`, or a key type the client cannot use) — must be
  rejected up front with a clear message, never attempted and hung.
- Server accepts the key but the account has no usable home directory.
- Cancel pressed at every phase: during TCP connect, during handshake,
  during authentication, during first listing.
- Server reachable but not an SSH service (wrong port), or SSH service that
  never completes the handshake.
- Connection attempt while another SFTP session is active in the other panel.
- Application exit requested while a connection attempt is in progress.
- Very slow server responses (multi-second latency) — operations must remain
  cancellable and must not be misreported as success or freeze the UI.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Users MUST be able to establish an SFTP session using a
  private key file against a server that accepts the matching public key,
  and then browse and transfer files normally. Release-blocking key
  configurations: unencrypted RSA (OpenSSH format), unencrypted ECDSA
  P-256, and passphrase-protected RSA (classic PEM) — i.e. all three
  reference keys.
- **FR-002**: The application MUST remain responsive (able to repaint,
  receive input, and be closed normally) during every phase of an SFTP
  connection attempt and session operation, for both password and key
  authentication.
- **FR-003**: Users MUST be able to cancel an in-progress connection
  attempt at any phase; cancellation MUST take effect promptly and return
  the panel to its previous state.
- **FR-004**: Every connection attempt MUST conclude in bounded time —
  success, user cancellation, or a timeout/error message — never an
  indefinite wait.
- **FR-005**: Passphrase-protected keys MUST trigger a passphrase prompt;
  an incorrect passphrase MUST produce a clear error and allow retry
  without restarting the application.
- **FR-006**: Authentication failures (key not accepted, no matching
  method) MUST surface as a user-readable message distinguishing
  authentication failure from connectivity failure; when the server rejects
  the key but permits password authentication, the plugin MUST offer a
  password prompt as a fallback within the same connection attempt, and a
  declined fallback MUST end the attempt cleanly.
- **FR-007**: Unsupported or unreadable key files MUST be rejected before
  any connection attempt with a message naming the problem and, where
  applicable, the remedy (e.g. convert to a supported format).
- **FR-008**: A failed or cancelled connection attempt MUST NOT degrade the
  rest of the application: other panels, other sessions, and subsequent
  connection attempts MUST work without restart.
- **FR-009**: Session-level failures (network drop, server disconnect,
  operation error) MUST end the affected operation with an error message
  and leave the plugin able to close, reopen, or re-establish sessions.
- **FR-010**: All SFTP operations available under password authentication
  MUST behave identically under key authentication.
- **FR-011**: The plugin's existing automated test harness MUST be extended
  with regression scenarios for this fix — successful key authentication,
  unauthorized key, passphrase-protected key, and a bounded-time watchdog
  that fails the test if any scenario hangs — runnable against the local
  reference test server.

### Key Entities

- **Connection profile**: The user-entered connection definition — host,
  port, user name, authentication method (password / private key file),
  key file path.
- **Private key file**: A file on the local machine in one of the supported
  formats; may be passphrase-protected. Referenced by the connection
  profile, read at connect time.
- **SFTP session**: A live authenticated connection bound to a panel;
  created, used for listings/transfers, and destroyed on disconnect or
  failure.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Against the reference test server, a key-authenticated
  connection succeeds and shows the remote listing in under 10 seconds on
  first attempt.
- **SC-002**: Zero application freezes requiring restart across the full
  failure-mode test matrix (each of the three reference keys, unauthorized
  key, encrypted key with correct/wrong passphrase, unsupported key,
  unreachable server, silent server, cancelled attempt, mid-session
  network drop).
- **SC-003**: 100% of failed connection attempts end with a user-visible
  error message within 60 seconds of initiation (sooner where the failure
  is detectable earlier).
- **SC-004**: A user-initiated cancel of a pending connection takes effect
  within 2 seconds in every connection phase.
- **SC-005**: All routine session operations (list, download, upload,
  rename, delete, disconnect, reconnect) produce identical outcomes under
  key and password authentication in side-by-side testing.
- **SC-006**: The extended automated regression suite (key-auth success,
  unauthorized key, passphrase, hang watchdog) passes repeatably (3
  consecutive clean runs) against the reference test server.
- **SC-007**: The full plugin code audit produces a written findings list;
  every finding classified as hang/crash/data-loss risk is either fixed in
  this feature or explicitly deferred with a documented reason.

## Assumptions

- The reference test environment from the bug report is available and
  authoritative for reproduction: local containerized SFTP server
  (`localhost:2222`, user `tctest`, password `tandem123`) with the
  `tctest_rsa` (unencrypted RSA 4096, OpenSSH format), `tctest_ecdsa`
  (ECDSA P-256) and `tctest_rsa_pass` (passphrase-protected, PEM) key
  pairs installed in the server's authorized keys; keys live in
  `C:\Users\pavel\.ssh\tandem-sftp-test\`.
- The same private key authenticates successfully with a standard
  command-line client against the same server, so server-side
  configuration is correct and the defect is client-side.
- Scope is confined to the SFTP plugin; core application and other plugins
  change only if the root cause demonstrably lies outside the plugin.
- "Overall stabilization" means: defects of the hang/crash/unreported-error
  family found during a comprehensive audit of the plugin's code are fixed
  within this feature; unrelated functional enhancements are out of scope.
- Key types/formats the plugin already documents as unsupported (e.g.
  PuTTY `.ppk`) remain unsupported; the requirement is a clear up-front
  message, not new format support.
- The user has requested that the analysis phase be carried out by several
  independent agents examining the entire plugin codebase; this is a
  process requirement for the planning/implementation phase and does not
  change the outcomes specified here.
