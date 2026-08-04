# Implementation Plan: On-Demand Release Code Signing

**Branch**: `050-code-signing` | **Date**: 2026-08-04 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/050-code-signing/spec.md`

## Summary

Sign every shipped PE artifact of the x64 release build (exe, dll, .spl
plugins, .slg language modules — currently 206 files) plus the Inno Setup
installer and its uninstaller with the maintainer's Certum certificate
(selected from the Windows certificate store by SHA-1 thumbprint,
timestamped by Certum, SHA-256 digests) — strictly on demand. The core is an
idempotent PowerShell 5.1 signing sweep (`tools/codesign/sign_release.ps1`)
driven by a one-line committed profile (`tools/codesign/codesign.cfg`),
triggered by a new `sign` argument of `build.cmd` and by a new
`setup\build_setup.cmd [sign]` installer script; a `setup` argument of
`build.cmd` chains the whole signed release into one command. The release
output tree is additionally freed of non-distribution linker byproducts
(`.pdb/.lib/.exp`) by redirecting them outside the tree (feature-023
pattern) with a cleanup + installer-exclude safety net.

## Technical Context

**Language/Version**: Windows Batch (.cmd) + Windows PowerShell 5.1 (no pwsh7
on the build machine); MSBuild project settings (VS2022); Inno Setup 7 script
**Primary Dependencies**: `signtool.exe` (Windows 10/11 SDK, already a build
prerequisite), Inno Setup 7 compiler `ISCC.exe`
(`C:\Program Files\Inno Setup 7\ISCC.exe`, not on PATH), Certum timestamp
authority `http://time.certum.pl`
**Storage**: `tools/codesign/codesign.cfg` (committed key=value signing
profile: certificate thumbprint + timestamp URL); certificate + private key
live only in the maintainer's Windows certificate store
**Testing**: manual/scripted end-to-end runs (`build.cmd release`,
`build.cmd full release sign`, `setup\build_setup.cmd [sign]`),
`Get-AuthenticodeSignature` / `signtool verify /pa` verification passes
**Target Platform**: Windows 11+, maintainer's build machine (interactive
session with access to the certificate store); no CI signing
**Project Type**: build/release tooling for a desktop application
**Performance Goals**: unsigned builds unchanged (0 added work); re-sweep of a
fully signed tree < 1 min (SC-003); initial sweep bounded by TSA round-trips —
batch ~15 files per signtool invocation (~14 batches for 206 files)
**Constraints**: signing strictly opt-in (FR-002); PowerShell 5.1 syntax only;
no absolute paths committed except documented tool locations resolved with
fallbacks; existing `*_release.props` post-build hooks must keep working
unchanged (FR-012)
**Scale/Scope**: 206 PE files today (1 exe + 2 utils + 19 .spl + 4 helper
binaries + 180 .slg), grows automatically with plugins/languages — the sweep
discovers files from the tree, never from a hard-coded list

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Assessment |
|---|---|
| I. Build Reproducibility | PASS — single-command flows (`build.cmd full release sign [setup]`), no manual steps; signing is a deterministic post-step; no hardcoded paths beyond resolved tool discovery with fallbacks; output stays under `OPENSAL_BUILD_DIR` |
| II. Backward Compatibility | PASS — no product-behavior change; binaries gain signatures only; unsigned builds byte-identical in behavior; no registry/identity changes |
| III. Incremental Modernization | PASS — small, independently revertable changes: one new ps1 + cfg, one hook fill-in, additive build.cmd args, additive .iss block, one new setup script, one Directory.Build.targets ItemDefinitionGroup |
| IV. Windows Platform Commitment | PASS — pure Windows tooling (signtool, MSBuild, Inno Setup) |
| V. Plugin Architecture Preservation | PASS — plugins untouched; .spl/.slg signed as opaque PE files |
| VI. UI Consistency | N/A — no UI |

Post-design re-check: PASS (no violations introduced by design; no
Complexity Tracking entries needed).

## Project Structure

### Documentation (this feature)

```text
specs/050-code-signing/
├── spec.md              # Feature specification (clarified)
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/
│   └── signing-cli.md   # CLI/config contracts for all four surfaces
└── tasks.md             # Phase 2 output (/speckit.tasks)
```

### Source Code (repository root)

```text
tools/codesign/
├── codesign.cfg             # NEW: committed signing profile (thumbprint, TSA)
├── sign_release.ps1         # NEW: signing core (sweep + single-file modes)
└── sign_with_retry.cmd      # REWRITE: placeholder → TC_CODESIGN-gated hook

build.cmd                    # MODIFY: `sign` + `setup` args, guard, sweep call,
                             #         release-tree byproduct cleanup, help/summary

src/Directory.Build.targets  # MODIFY: Release-only redirect of ImportLibrary
                             #         (.lib/.exp) and ProgramDatabaseFile (.pdb)
                             #         out of the shipped tree (feature-023 pattern)

src/vcxproj/build_langs.ps1  # MODIFY (found during verification): strip the
                             #         Authenticode signature from the english.slg
                             #         seed copy before the translator patches it -
                             #         a signed seed otherwise yields an unsignable
                             #         module (see research addendum in
                             #         verification.md, defect D1)

setup/
├── tandemcommander.iss      # MODIFY: #ifdef SIGN block, Excludes on [Files]
└── build_setup.cmd          # NEW: installer compile, optional sign chain

README.md                    # MODIFY: release + signing process documentation
```

**Structure Decision**: extend the existing build-tooling surfaces in place —
`tools/codesign/` is the upstream-designated home for signing (every
`*_release.props` already calls into it), `build.cmd` is the single build
entry point, `setup/` owns the installer. No new directories except files
inside existing ones.

## Design Decisions (Phase 0 summary — details in research.md)

1. **Sweep over per-target hooks as the primary mechanism**: one idempotent
   pass over the finished tree covers `.slg` files (which have no post-build
   hooks), works without rebuilding (FR-009), and batches TSA traffic. The
   existing per-target hook becomes a `TC_CODESIGN`-gated opt-in (FR-012),
   default no-op, preserving FR-002.
2. **Idempotence**: skip files whose Authenticode status is `Valid` AND whose
   signer thumbprint equals the configured one; anything else is (re-)signed —
   signtool without `/as` replaces the primary signature (FR-004, Story 3).
3. **FR-013 via redirect + sweep**: a Release-only `<ItemDefinitionGroup>` in
   `src/Directory.Build.targets` moves `ImportLibrary` (which also carries the
   `.exp`) and `ProgramDatabaseFile` into `$(IntDir)` (already relocated to
   `…\obj\` outside the tree by feature 023). Verified: nothing links against
   those import libraries by path (sqlite/7za are loaded dynamically), so the
   redirect is safe; PDBs stay archived under `obj\` for crash symbolication.
   Deleting instead of redirecting would invalidate MSBuild link outputs and
   force a full LTCG relink on every incremental Release build — rejected.
   `build.cmd`'s existing `:clean_release_tree` additionally deletes stray
   `*.pdb/*.lib/*.exp` from the tree (legacy trees, safety net).
4. **Installer signing via ISCC command line**: `.iss` gains a
   `#ifdef SIGN … SignTool=tcsign / SignedUninstaller=yes … #endif` block;
   `build_setup.cmd sign` passes
   `/DSIGN=1 "/Stcsign=$q<signtool>$q sign /sha1 <thumb> /tr <tsa> /td sha256 /fd sha256 /v $f"`.
   Unsigned compiles remain possible with zero signing dependencies (FR-006,
   Story 2 scenario 4). FR-014: `Excludes: "*.pdb,*.lib,*.exp"` on the
   recursive `[Files]` entry.
5. **Robustness**: batches of ~15 files per signtool call, 3 attempts with 5 s
   backoff per batch, per-file fallback isolation after a failed batch, final
   whole-tree verification, non-zero exit + named files on any failure
   (FR-007, FR-008).
6. **Tool discovery**: signtool = newest
   `%ProgramFiles(x86)%\Windows Kits\10\bin\10.*\x64\signtool.exe`, fallback
   `where signtool`; ISCC = Inno Setup 7 path, fallback `where iscc`, then
   Inno Setup 6 path. Certificate store lookup replicates the maintainer's
   proven command verbatim (`/sha1 <thumb>`, no store overrides).

## Complexity Tracking

No constitution violations — table not needed.
