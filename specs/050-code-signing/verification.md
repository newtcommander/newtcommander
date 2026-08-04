# Verification Log: On-Demand Release Code Signing

**Date**: 2026-08-04 · **Machine**: maintainer build machine (Windows 11,
VS2022, Windows SDK 10.0.26100, Inno Setup 7, Certum certificate
`A3D05CCF5CA13EAFF49CC7F64D1832F0E6EF6733` in `Cert:\CurrentUser\My`,
valid to 2027-08-03)

## Foundational (T002–T004)

| Check | Result |
|---|---|
| `sign_release.ps1` PSParser syntax check | ✅ 0 errors |
| New files pure ASCII (`codesign.cfg`, `sign_release.ps1`, `sign_with_retry.cmd`, `build_setup.cmd`) | ✅ 0 non-ASCII bytes |
| `build.cmd full release` after Directory.Build.targets redirect | ✅ BUILD SUCCEEDED (40 s incremental) |
| FR-013: `*.pdb/*.lib/*.exp` in Release_x64 after build | ✅ 0 files (was 72); 52 PDBs present under `build\obj\Release_x64\` for symbolication |
| PE artifact population | ✅ 206 files (1+2 exe/dll in utils, 19 .spl, 4 plugin helpers, 180 .slg) |

## User Story 1 — signed release build (T007)

| Scenario | Result |
|---|---|
| (a) `build.cmd full release` without `sign` | ✅ no signing, no TSA contact, 0 of 206 artifacts signed afterwards; build output identical in shape to pre-feature builds |
| (f) `build.cmd sign` without `release` | ✅ rejected before any work: "ERROR: 'sign' requires 'release'", exit 1 |
| (d) standalone sweep over existing unsigned tree (FR-009) | ✅ `Signed: 206  Skipped: 0  Failed: 0 (of 206)`, `Verified: 206 of 206`, exit 0, 169 s (14 batches × ≤15 files, Certum TSA) |
| (b) spot checks (`tandemcommander.exe`, `zip.spl`, `czech.slg`, `sqlite.dll`, `7za.dll`, `fcremote.exe`) | ✅ all `Valid`, thumbprint `A3D05CCF…`, timestamp `CN=Certum Timestamp 2026` |
| (c) idempotent re-run (SC-003) | ✅ `Signed: 0  Skipped: 206  Failed: 0`, exit 0, **3 s** |
| (e) `-VerifyOnly` on signed tree | ✅ "206 of 206 artifacts signed by the configured certificate", exit 0, 3 s |
| per-target hook, `TC_CODESIGN` unset | ✅ no-op, exit 0, file untouched (`NotSigned`) |
| per-target hook, `TC_CODESIGN=1` | ✅ target signed + verified via `sign_release.ps1 -File`, exit 0 |

## User Story 3 — certificate lifecycle (T012)

| Scenario | Result |
|---|---|
| Bogus thumbprint in profile (cert-missing edge case) | ✅ pre-flight fails with "certificate … not found in Cert:\CurrentUser\My or Cert:\LocalMachine\My", exit 1, **no file modified** |
| File signed by a different certificate | ✅ classified "NOT SIGNED BY CURRENT CERT" (`-VerifyOnly` exit 1); sweep re-signs it — signtool replaces the primary signature; after re-sign it verifies with the configured thumbprint |
| OS-catalog-signed file dropped into the tree (found during testing) | ✅ handled: `Get-AuthenticodeSignature` reports the *catalog* signer even after embedding our signature (Authenticode hash excludes the security directory, so the OS catalog stays valid); `Test-SignedByCurrent` now inspects the embedded signer for `SignatureType -eq 'Catalog'`, preventing an endless re-sign loop and false verification failures |
| Rotation procedure (quickstart.md) matches behavior | ✅ single `thumbprint` edit in `codesign.cfg`; sweep converges old-cert artifacts (verified via the different-certificate case above) |

## User Story 2 — signed installer (T011)

| Scenario | Result |
|---|---|
| (a) `setup\build_setup.cmd` (unsigned) | ✅ compiles with zero signing dependencies (no `/DSIGN`, no signtool); produced installer `NotSigned` as expected; 6.5 MB, 3.6 s compile |
| (b) `setup\build_setup.cmd sign` over a partially unsigned tree (3 signatures stripped via `signtool remove`) | ✅ sweep ran first: `Signed: 3  Skipped: 203`, `Verified: 206 of 206` — the installer can never package unsigned binaries |
| (c) installer signature | ✅ ISCC log: `Successfully signed: …tandemcommander-0.1.0-x64-setup.exe`; post-compile verification: `Valid`, thumbprint `A3D05CCF…`, exit 0 |
| uninstaller stub signed at compile time | ✅ ISCC log: `Running Sign Tool tcsign: … uninst.e32.tmp` → `Successfully signed` (`SignedUninstaller=yes`) |
| (d) silent install (`/VERYSILENT /CURRENTUSER`) → `unins000.exe` | ✅ install exit 0; `unins000.exe`: `Valid`, thumbprint `A3D05CCF…`; installed `tandemcommander.exe` also `Valid` |
| (e) installed content hygiene (SC-007) | ✅ 0 × `*.pdb/*.lib/*.exp` installed; 207 PE files (206 artifacts + unins000.exe) |
| silent uninstall | ✅ `/VERYSILENT` uninstall removed everything (0 files left) |
| (f) `build.cmd full release sign setup` (SC-008) | ✅ see Final acceptance below |

## Defects found and fixed during verification

**D1 — language modules seeded from a signed english.slg (root cause).**
The first chained `build.cmd full release sign setup` failed: 40 `.slg`
files (czech + dutch × 20 modules) came out of `build_langs` unsignable
(`signtool` 0x800700C1, and `signtool remove` fails too with 0x57).
`build_langs.ps1` seeds every language module as a **copy of english.slg**
and lets the translator patch it in place; once english.slg is signed, the
patched copy keeps a stale, malformed certificate table. Exactly czech +
dutch were affected because the sweep's alphabetical order stamped their
signatures *before* english.slg, making them look older than their seed to
the incremental check. **Fixes**:
- `build_langs.ps1`: `Remove-PeSignature` strips the Authenticode signature
  from the seed copy right after `Copy-Item` — modules are again clean,
  signable PEs regardless of the seed's signing state (verified:
  regenerated module has zeroed certificate directory, sections intact,
  `NotSigned`).
- `sign_release.ps1`: files classified for signing get any existing
  certificate table stripped first (self-heal for stale/broken signatures
  that signtool itself cannot remove), and `english.slg` files are signed
  **first** so sibling modules never look stale after a sweep.

**D2 — first repair heuristic could truncate live data.** The initial
`Remove-PeSignature` truncated at the certificate offset whenever the table
reached EOF; for *stale* directory entries pointing into live data this cut
real section bytes (26 modules corrupted, confirmed by section-table
forensics; all were regenerable build artifacts). Final version truncates
only when a plausible `WIN_CERTIFICATE` header sits at the offset
(`dwLength` == directory size, known `wRevision`) **and** no section's raw
data extends past the cut; otherwise it only zeroes the directory entry.
The 40 modules were deleted, regenerated by `build_langs.cmd release`
(exercising the D1 fix against the signed seed), and signed:
`Signed: 40  Skipped: 166  Failed: 0`, `Verified: 206 of 206`, exit 0.

**D3 — pre-existing cosmetic defect in `build.cmd` duration math** (surfaced
by these runs, not caused by feature 050): `%time%` tokens `08`/`09` parse
as invalid octal in `set /a`, breaking the displayed duration for builds
started between 8–9 o'clock. Fixed with the `1%%a %% 100` idiom in both
time-parse blocks.

## Final acceptance (T015)

`build.cmd full release sign setup` (single command, SC-008):

| Check | Result |
|---|---|
| Build | ✅ BUILD SUCCEEDED, incremental, duration displayed correctly (D3 fix — run started 08:xx) |
| Tree signing step | ✅ `Signed: 0  Skipped: 206  Failed: 0 (of 206)` — tree already converged, idempotent inside the chain |
| Installer step | ✅ compiled, uninstaller stub + setup exe signed by Inno (`Number of errors: 0`), post-compile verification passed |
| Summary block | ✅ `Code signing : OK`, `Installer : OK (setup\output\)` |
| Whole-tree audit afterwards | ✅ `-VerifyOnly`: 206 of 206, exit 0 |
| Installer signature | ✅ `Valid`, thumbprint `A3D05CCF…`, `CN=Certum Timestamp 2026`; `signtool verify /pa` → `Successfully verified`, exit 0 |
| Encoding conventions | ✅ new `.cmd/.ps1/.cfg` files pure ASCII; edited files keep their original encoding (no BOM changes) |

**Success criteria**: SC-001 ✅ (206/206 verify) · SC-002 ✅ (verified
publisher via valid Authenticode signature; SmartScreen reputation builds
over time by design) · SC-003 ✅ (re-run 3 s, 0 signed) · SC-004 ✅ (unsigned
builds: no signing code path executes; arg-gated) · SC-005 ✅ (single
`thumbprint` entry; different-cert artifacts converge) · SC-006 ✅ (Certum
outage mid-run exercised for real — retries + per-file isolation + resumable
re-run) · SC-007 ✅ (0 byproducts in tree and in installed dir) · SC-008 ✅
(one command produced signed build + signed installer).
