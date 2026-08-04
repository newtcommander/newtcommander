# Phase 0 Research: On-Demand Release Code Signing

All Technical Context unknowns resolved. Each decision below records what was
chosen, why, and what was rejected.

## R1. Primary signing mechanism: post-build hooks vs. output-tree sweep

- **Decision**: An idempotent sweep over the finished release output tree
  (`tools/codesign/sign_release.ps1 -Root <tree>`) is the primary mechanism.
  The existing per-target post-build hook (`sign_with_retry.cmd`, called by
  all 19 `*_release.props`) becomes a thin, `TC_CODESIGN`-env-gated wrapper
  around the same core (single-file mode), default no-op.
- **Rationale**:
  - `.slg` language modules are produced by `build_langs.cmd` (translator
    quiet modes), not by MSBuild post-build events — hooks alone can never
    cover 180 of the 206 files. Upstream acknowledged this with the separate
    `src/vcxproj/signslgs.cmd` sweep.
  - FR-009 requires signing an existing tree without rebuilding — only a
    sweep can do that.
  - FR-002 requires zero behavior change by default — a gated hook keeps
    every `*_release.props` untouched (FR-012).
  - Batching TSA requests is only possible in a sweep.
- **Alternatives considered**:
  - *Hooks as primary, sweep only for .slg*: rejected — two mechanisms to
    keep consistent, incremental builds leave unsigned stale binaries, and
    a plain rebuild would contact the TSA even when signing wasn't wanted
    unless additionally gated.
  - *Signing inside MSBuild via `<SignTask>`/custom targets*: rejected —
    invasive per-project changes, violates "props hooks unchanged".

## R2. Idempotence / re-sign semantics

- **Decision**: For every candidate file run
  `Get-AuthenticodeSignature`; skip iff `Status -eq 'Valid'` AND
  `SignerCertificate.Thumbprint` equals the configured thumbprint
  (case-insensitive). Everything else — unsigned, invalid, or signed by a
  different certificate — is (re-)signed. `signtool sign` without `/as`
  replaces the primary signature, so re-signing old-cert files needs no
  special handling.
- **Rationale**: satisfies FR-004 and Story 3 (cert rotation converges the
  tree); makes re-runs after a network outage resumable (SC-006) and the
  no-op re-run fast (SC-003).
- **Alternatives considered**: `signtool verify /pa /q` per file — slower
  (process spawn per file ~200 ms × 206) and cannot distinguish "valid but
  old cert" without extra parsing. `Get-AuthenticodeSignature` is in-process
  and returns the signer certificate directly.

## R3. Keeping `.pdb/.lib/.exp` out of the release tree (FR-013)

- **Decision**: Release-only `<ItemDefinitionGroup>` in
  `src/Directory.Build.targets`:
  `<ImportLibrary>$(IntDir)$(TargetName).lib</ImportLibrary>` and
  `<ProgramDatabaseFile>$(IntDir)$(TargetName).pdb</ProgramDatabaseFile>`.
  Feature 023 already relocates `$(IntDir)` outside the shipped tree
  (`…\obj\Release_x64\…`), so the linker byproducts follow: `.lib` (and the
  `.exp`, which the linker always emits next to the import library) and
  `.pdb` never enter the tree in the first place. As a safety net for legacy
  trees and anything that escapes, `build.cmd :clean_release_tree` also
  deletes `*.pdb;*.lib;*.exp` under the output root (excluding nothing —
  verified no shipped file uses these extensions).
- **Rationale**:
  - *Why not delete-only*: `.lib/.pdb` are declared MSBuild link outputs
    (tracked in `link.write.*.tlog`); deleting them marks every DLL project
    out-of-date, and Release links with LTCG (`/LTCG` + WPO) — every
    incremental Release build would pay a full code-gen relink of all ~30
    binaries. Redirecting keeps incremental builds intact (same reasoning
    that made feature 023 relocate instead of delete intermediates).
  - PDBs remain archived under `obj\Release_x64\…` for crash-dump
    symbolication (salmon minidumps) instead of being destroyed.
- **Verified safety**: grep across `src/**/*.{vcxproj,props,targets}` found
  zero references to any produced import library (`sqlite.lib`, `7za.lib`,
  `7zwrapper.lib`, `exif.lib`, plugin libs); sqlite.dll/7za.dll are loaded
  dynamically. `Directory.Build.targets` is evaluated after project bodies,
  so the metadata override wins uniformly.
- **Alternatives considered**: per-project props edits (30+ files touched —
  rejected, feature-023 precedent solves it centrally); Inno-side excludes
  only (rejected as sole measure by clarification Q1 — the tree itself must
  be clean; kept as independent layer per FR-014).

## R4. Installer signing (FR-006)

- **Decision**: `.iss` gains a conditional block —
  `#ifdef SIGN` → `SignTool=tcsign`, `SignedUninstaller=yes` → `#endif`.
  `setup/build_setup.cmd sign` (new) first runs the tree sweep, then compiles
  with `ISCC /DSIGN=1 "/Stcsign=$q<signtool>$q sign /sha1 <thumb> /tr <tsa> /td sha256 /fd sha256 /v $f" tandemcommander.iss`,
  then verifies the produced installer's signature. Without `sign` the
  compile is exactly today's manual ISCC run.
- **Rationale**: the `/S` command-line definition keeps the signing command
  out of the committed `.iss` (single source stays `codesign.cfg`); the
  `#ifdef` keeps unsigned compiles free of any sign-tool dependency (Story 2
  scenario 4). Inno signs both `setup.exe` and the uninstaller stub when
  `SignedUninstaller=yes`. Running the sweep first enforces "a signed
  installer never packages unsigned binaries" (Story 2 scenario 2).
- **Alternatives considered**: configuring the sign tool in the Inno IDE
  (per-machine, not committed, not automatable — rejected); Inno
  `Flags: signonce` per file (redundant — sweep already signed the tree).

## R5. Timestamp-authority robustness (FR-007)

- **Decision**: batches of 15 files per `signtool sign` invocation; up to 3
  attempts per batch with 5 s pause; after 3 failed batch attempts, fall back
  to per-file signing inside that batch to isolate and report the exact
  failing file(s). Certum (`http://time.certum.pl`) is the only TSA, per the
  maintainer's proven command.
- **Rationale**: signtool timestamps every file in an invocation but a batch
  amortizes process startup and groups failures; Certum occasionally
  throttles/hiccups — bounded retries with backoff absorb transient faults
  while persistent failure still fails the run with named files (FR-007).
- **Alternatives considered**: per-file invocations only (206 process spawns
  + 206 individually retried TSA calls — slow); fallback TSA rotation
  (rejected — user specified Certum; silent TSA switching changes the
  timestamp chain unexpectedly).

## R6. Tool discovery

- **Decision**:
  - `signtool.exe`: newest version under
    `%ProgramFiles(x86)%\Windows Kits\10\bin\10.*\x64\signtool.exe`
    (sorted by version, descending), fallback `where signtool`.
  - `ISCC.exe`: `C:\Program Files\Inno Setup 7\ISCC.exe` (present on the
    build machine, not on PATH), fallback `where iscc`, then
    `C:\Program Files (x86)\Inno Setup 6\ISCC.exe`.
- **Rationale**: Windows SDK is already a mandatory build prerequisite, so
  signtool is guaranteed present; explicit paths keep the scripts runnable
  from a plain cmd prompt (Build Reproducibility principle: single command,
  no manual PATH setup).

## R7. Certificate selection

- **Decision**: replicate the maintainer's proven invocation verbatim:
  `/sha1 <thumbprint> /tr http://time.certum.pl /td sha256 /fd sha256 /v`
  (no `/s`/`/sm` store overrides — default current-user `My` store search).
  Thumbprint + TSA URL live in `tools/codesign/codesign.cfg` (committed;
  thumbprints are public). Missing certificate is detected up front
  (`Get-ChildItem Cert:\CurrentUser\My`/`Cert:\LocalMachine\My` by
  thumbprint) so the run fails before touching any file (edge case:
  cert-not-installed).
- **Alternatives considered**: PFX file + password (secret handling in repo
  or env — rejected); Azure Trusted Signing/HSM (not what the maintainer
  has).

## R8. PowerShell 5.1 constraints (build machine has no pwsh 7)

- **Decision**: `sign_release.ps1` targets Windows PowerShell 5.1: no
  ternary/null-coalescing, no `ForEach-Object -Parallel`, ASCII-only source
  (the machine's 5.1 parses UTF-8-no-BOM as ANSI — known repo quirk), explicit
  `-ErrorAction Stop` + try/catch around cmdlet failures, `exit 1` on any
  failure so `build.cmd` can gate on `%errorlevel%`.
- **Rationale**: `normalize.ps1` requiring pwsh 7.4 is already unusable on
  this machine; repeating that mistake would make signing unrunnable.
