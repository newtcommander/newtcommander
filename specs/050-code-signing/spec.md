# Feature Specification: On-Demand Release Code Signing

**Feature Branch**: `050-code-signing`
**Created**: 2026-08-04
**Status**: Draft
**Input**: User description: "Na zaklade aktualniho kontextu priprav implementaci systemu pro podepisovani aplikace" — integrate the maintainer's Authenticode certificate (Windows certificate store, Certum timestamping) into the whole product: sign the complete release build and the installer, automated but strictly on demand (normal builds must stay unsigned and fast).

## Clarifications

### Session 2026-08-04

- Q: Should the installer's content be restricted to distribution files
  (today it packages the whole output tree recursively, including debug
  symbols and linker artifacts)? → A: Both layers: the release build itself
  must leave no non-distribution files in the output tree (the tree is the
  distributable), AND the installer independently excludes those file types
  as a safety net in case they ever reappear.
- Q: Should the release process offer one chained command (build → signing →
  installer), or stay as two separate commands? → A: Chained: an optional
  argument of the build command also produces the (signed) installer after
  the (signed) build; both steps remain individually invocable.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Fully signed release build on demand (Priority: P1)

The maintainer prepares a public release. With a single explicit request added
to the normal release build command, every executable artifact the product
ships — the main application, the crash reporter, helper libraries, all plugin
modules and all language modules — comes out carrying a valid, timestamped
digital signature from the maintainer's certificate. The run ends with a
verification pass and a summary, so the maintainer knows with certainty that
nothing shipped unsigned.

**Why this priority**: Signing the shipped binaries is the core of the feature;
without it the installer signing (Story 2) has nothing trustworthy to package.
An unsigned or partially signed release triggers SmartScreen/antivirus warnings
and erodes user trust.

**Independent Test**: Run the release build with the signing request and verify
that every shipped executable artifact in the output tree reports a valid
signature from the expected certificate; run it without the request and verify
nothing is signed and build time is unchanged.

**Acceptance Scenarios**:

1. **Given** a machine with the release certificate installed, **When** the
   maintainer runs the complete release build with the signing request,
   **Then** 100 % of shipped executable artifacts in the output tree carry a
   valid, timestamped signature from that certificate and the build reports a
   signing summary (signed / skipped / failed counts).
2. **Given** the same machine, **When** the maintainer runs any build without
   the signing request, **Then** no file is signed, no signing service is
   contacted, and the build behaves exactly as before the feature existed.
3. **Given** an output tree that is already fully signed, **When** the signing
   run is repeated, **Then** every artifact is recognized as already signed,
   nothing is re-signed, and the run completes in seconds.
4. **Given** an output tree containing artifacts signed with a previous
   (different) certificate, **When** the signing run executes, **Then** those
   artifacts are re-signed with the current certificate.
5. **Given** an existing unsigned release output, **When** the maintainer
   requests signing alone (without rebuilding), **Then** the whole tree is
   signed exactly as it would be during a signed build.

---

### User Story 2 - Signed installer and uninstaller (Priority: P2)

The maintainer produces the distributable installer with a signing request. The
resulting installer executable and the uninstaller it deploys are both signed,
and the installer is guaranteed to package only signed application binaries —
it is impossible to produce a "signed installer full of unsigned files" by
accident. The guarantee has two layers: the release build itself leaves only
distribution files in its output tree, and the installer independently
excludes non-distribution artifact types even if they were present.

**Why this priority**: The installer is the first executable a user runs; its
signature drives the Windows consent dialog ("Verified publisher") and
SmartScreen reputation. It depends on Story 1 for signed content.

**Independent Test**: Build the installer with the signing request over an
unsigned build output; verify the packaged binaries were signed first, the
installer executable verifies successfully, and after installation the
uninstaller is also signed.

**Acceptance Scenarios**:

1. **Given** a complete release output, **When** the maintainer builds the
   installer with the signing request, **Then** the installer executable
   carries a valid, timestamped signature and the elevation dialog shows the
   maintainer as verified publisher (never "Unknown publisher").
2. **Given** a release output with unsigned artifacts, **When** the installer
   is built with the signing request, **Then** the artifacts are signed before
   packaging (or the build fails) — a signed installer never contains unsigned
   shipped binaries.
3. **Given** an installed product from a signed installer, **When** the user
   inspects the uninstaller, **Then** it also carries a valid signature.
4. **Given** no signing request, **When** the installer is built, **Then** it
   builds exactly as today (unsigned), with no dependency on the certificate
   or signing tools.
5. **Given** a machine with the release certificate installed, **When** the
   maintainer runs the single chained release command, **Then** one run
   produces the fully signed build output and a signed installer packaged
   from exactly that output.

---

### User Story 3 - Certificate lifecycle in one place (Priority: P3)

The certificate eventually expires or is reissued. The maintainer updates the
certificate identity in exactly one committed location; the next signing run
uses the new certificate everywhere and re-signs anything still carrying the
old one. Signatures made earlier remain valid after the old certificate
expires because every signature is timestamped.

**Why this priority**: Certificate rotation is infrequent but inevitable; a
single point of change prevents the class of errors where some artifacts keep
being signed with a stale certificate.

**Independent Test**: Change the configured certificate identity to a second
certificate, re-run signing over a signed tree, and verify all artifacts now
carry the new certificate.

**Acceptance Scenarios**:

1. **Given** a new certificate installed on the build machine, **When** the
   maintainer updates the single configured certificate identity and re-runs
   signing, **Then** all artifacts (including previously signed ones) end up
   signed with the new certificate.
2. **Given** artifacts signed and timestamped with a certificate that has since
   expired, **When** their signatures are verified, **Then** verification still
   succeeds.

---

### Edge Cases

- Timestamp service is temporarily unreachable or rate-limits: the run retries
  transient failures automatically; only persistent failure fails the run, and
  the report names exactly which files remain unsigned.
- The configured certificate is not present in the machine's certificate
  store: the run fails immediately with a clear message, before modifying any
  file.
- A signing run is interrupted or partially fails: the output tree is left
  with each file either fully signed or untouched; re-running completes the
  remainder (idempotence) — no artifact is corrupted.
- Signing is requested together with a non-release (development) build
  configuration: the request is rejected with an explanation; development
  binaries are never signed.
- Non-executable files in the output tree (data tables, configuration text,
  documentation): never modified by a signing run.
- Non-distribution build artifacts (debug symbols, import libraries, export
  files) reappear in the output tree — e.g. regenerated by an incremental
  build step: the release build removes them again before completing, and the
  installer excludes them independently, so they can never reach users.
- Executable artifacts that are not part of the current default build (shell
  extension, spawn/open helpers, self-extractor stubs) but may ship later:
  they are signed by the same mechanism once they appear in the shipped
  output, with no additional configuration.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: A signing run MUST sign every shipped executable artifact in the
  release output tree: the main application, the crash reporter, helper
  libraries, all plugin modules and all language modules (currently 206
  files), regardless of file extension.
- **FR-002**: Signing MUST happen only on an explicit request; without it,
  every build (debug and release) behaves exactly as before this feature —
  same outputs, same duration, no contact with signing or timestamp services.
- **FR-003**: Every signature MUST be timestamped by the configured timestamp
  authority so that it remains valid after the certificate expires.
- **FR-004**: A signing run MUST be idempotent: artifacts already carrying a
  valid signature from the currently configured certificate are skipped;
  artifacts unsigned or signed by any other certificate are (re-)signed.
- **FR-005**: The certificate identity and the timestamp authority MUST be
  defined in exactly one committed location; rotating the certificate is a
  single-entry change.
- **FR-006**: The installer build MUST support the same explicit signing
  request; when requested it MUST produce a signed installer and a signed
  uninstaller, and MUST ensure all packaged shipped binaries are signed before
  packaging (signing them itself if needed).
- **FR-007**: Transient timestamp-service failures MUST be retried
  automatically; a run that still cannot sign a file MUST fail with a nonzero
  result and name every file left unsigned.
- **FR-008**: Every signing run MUST end with a verification pass over the
  full target set and report a summary (signed / skipped / failed); any
  failure makes the overall run fail.
- **FR-009**: The maintainer MUST be able to sign an existing release output
  on its own, without rebuilding.
- **FR-010**: A signing request combined with a development (non-release)
  configuration MUST be rejected with a clear explanation.
- **FR-011**: A signing run MUST never modify files that are not shipped
  executable artifacts (data tables, configuration text, documentation — and
  any build artifact that escaped removal per FR-013).
- **FR-012**: The existing per-project post-build signing hook points MUST
  remain in place and become functional opt-in participants of the same
  mechanism, so binaries built outside the default set adopt signing without
  new wiring.
- **FR-013**: After every release build completes, the release output tree
  MUST contain only files intended for distribution — build-time artifacts
  (debug symbols, import libraries, export files) MUST NOT be present. This
  holds for every release build, signed or not.
- **FR-014**: The installer MUST independently exclude non-distribution
  artifact types when packaging, so that even if such files appear in the
  output tree they are never delivered to users.
- **FR-015**: The build command MUST offer an optional request that chains
  the complete release end-to-end — release build, signing, installer — in
  one run; the build and the installer step MUST also remain individually
  invocable.

### Key Entities

- **Signing profile**: the single committed definition of *how to sign* — the
  certificate identity (selecting a certificate installed on the build
  machine; the private key is never in the repository) and the timestamp
  authority to use.
- **Signable artifact set**: the set of shipped executable artifacts inside a
  release output tree (application, crash reporter, helper libraries, plugin
  modules, language modules) — discovered from the tree itself, never a
  hard-coded list, so newly added plugins or languages are covered
  automatically.
- **Signing run report**: the outcome of one signing run — per-file result
  (signed, skipped as already signed, failed) and an overall pass/fail.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: One command produces a release build in which 100 % of shipped
  executable artifacts verify successfully against the maintainer's
  certificate.
- **SC-002**: Installing from a signed installer on a clean Windows machine
  shows the maintainer as verified publisher in the elevation dialog — never
  "Unknown publisher".
- **SC-003**: Re-running a signing run over an already-signed output signs 0
  files and completes in under 1 minute.
- **SC-004**: Builds without a signing request show no measurable duration
  increase compared to before the feature (within normal build-time variance).
- **SC-005**: Certificate rotation requires editing exactly one committed
  entry; the next signing run converges the whole output to the new
  certificate without manual per-file work.
- **SC-006**: A signing run interrupted by a network outage can be re-run to
  completion with no manual cleanup, and the final tree passes full
  verification.
- **SC-007**: After a release build, the output tree contains zero
  non-distribution build artifacts, and the produced installer contains zero
  such files regardless of the output tree's state.
- **SC-008**: A complete signed release — signed build output plus signed
  installer — is producible with a single command invocation.

## Assumptions

- The release certificate is installed in the maintainer's Windows certificate
  store on the build machine and is referenced by its identity (thumbprint);
  no private key material ever enters the repository. The current certificate
  thumbprint is `a3d05ccf5ca13eaff49cc7f64d1832f0e6ef6733`.
- The timestamp authority is Certum (`http://time.certum.pl`), matching the
  maintainer's proven manual signing command (SHA-256 file digest and SHA-256
  timestamp digest).
- Signing runs happen on the maintainer's machine (interactive session with
  access to the certificate store); no CI/build-server signing is in scope.
- The certificate thumbprint is public information and safe to commit.
- Debug builds are for development only and are never signed or distributed.
- Scope is the x64 release output tree and the distributable installer.
  Artifacts not currently built (shell extension, legacy setup/remove, spawn
  and open helpers, self-extractor stubs) are covered by the same mechanism
  when they join the shipped output (FR-012), but wiring their projects into
  the default build is out of scope.
- The one existing published installer output is superseded; re-signing or
  re-publishing previously released artifacts is out of scope.
