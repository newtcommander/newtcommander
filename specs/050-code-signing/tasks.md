# Tasks: On-Demand Release Code Signing

**Input**: Design documents from `/specs/050-code-signing/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md,
contracts/signing-cli.md, quickstart.md

**Tests**: No automated test framework exists for .cmd/.ps1 tooling in this
repo; each story instead carries an explicit scripted verification task that
executes its acceptance scenarios against the real build tree.

**Organization**: Tasks are grouped by user story so each story is an
independently deliverable increment.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 = signed release build, US2 = signed installer,
  US3 = certificate lifecycle

## Phase 1: Setup

**Purpose**: Create the committed signing profile every other piece reads.

- [X] T001 Create `tools/codesign/codesign.cfg` with `thumbprint =
      a3d05ccf5ca13eaff49cc7f64d1832f0e6ef6733` and `timestamp_url =
      http://time.certum.pl` per contracts/signing-cli.md §2 (ASCII, `#`
      comment header explaining rotation = edit thumbprint)

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The signing core used by every story, and the FR-013 tree
hygiene that must hold for every release build.

**⚠️ CRITICAL**: T002 blocks all user stories.

- [X] T002 Implement `tools/codesign/sign_release.ps1` (Windows PowerShell
      5.1, ASCII-only source) per contracts/signing-cli.md §1: parameter set
      `-Root | -File`, `[-Config]`, `[-VerifyOnly]`; codesign.cfg parser +
      validation (40-hex thumbprint, URL); pre-flight certificate presence
      check in `Cert:\CurrentUser\My` + `Cert:\LocalMachine\My`; signtool
      discovery (newest `%ProgramFiles(x86)%\Windows Kits\10\bin\10.*\x64\signtool.exe`,
      fallback `where signtool`); candidate enumeration (`*.exe,*.dll,*.spl,*.slg`,
      exclude `\Intermediate\`); per-file state classification via
      `Get-AuthenticodeSignature` (skip only Valid + matching thumbprint);
      batches of 15, 3 attempts per batch with 5 s pause, per-file fallback
      after failed batch; final verification pass; summary
      `Signed/Skipped/Failed (of T)` + `FAILED: <path>` lines; exit 0 iff
      whole target set verifies
- [X] T003 [P] Add Release-only `<ItemDefinitionGroup>` to
      `src/Directory.Build.targets` redirecting `<ImportLibrary>` and
      `<ProgramDatabaseFile>` to `$(IntDir)` so `.lib/.exp/.pdb` land in the
      relocated `obj\` tree, never in the shipped tree (FR-013, research R3;
      keep the feature-023 comment style and document the KEEP IN SYNC nature)
- [X] T004 [P] Extend `:clean_release_tree` in `build.cmd` to also delete
      `*.pdb`, `*.lib`, `*.exp` under `%OUT_DIR%` (legacy-tree cleanup +
      safety net for FR-013; keep existing Intermediate/saltests sweep)

**Checkpoint**: `sign_release.ps1 -VerifyOnly` runs against an existing tree;
a fresh `build.cmd full release` leaves zero `.pdb/.lib/.exp` in the tree.

---

## Phase 3: User Story 1 — Fully signed release build on demand (P1) 🎯 MVP

**Goal**: `build.cmd full release sign` produces a fully signed, verified
tree; every other build stays byte-identical in behavior.

**Independent Test**: acceptance scenarios 1–5 of Story 1 (spec.md) against
the real Release_x64 tree.

- [X] T005 [US1] `build.cmd`: parse new `sign` argument (order-independent
      loop at line ~30); early guard `sign` without `release` → error + exit 1
      before any build step; after successful build + `:populate_runtime` +
      `:clean_release_tree`, invoke
      `powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\codesign\sign_release.ps1" -Root "%OUT_DIR%"`;
      non-zero → `BUILD_EXIT=1`; add `sign` to help text and a signing result
      line to the summary block
- [X] T006 [P] [US1] Rewrite `tools/codesign/sign_with_retry.cmd`: exit 0
      immediately unless `TC_CODESIGN` is set; otherwise delegate to
      `sign_release.ps1 -File %1` (script-relative path via `%~dp0`) and
      propagate the exit code (FR-012, contract §3; must handle quoted paths
      with spaces)
- [X] T007 [US1] Verify Story 1 end-to-end and record results in
      `specs/050-code-signing/verification.md`: (a) `build.cmd release`
      without `sign` — no signing, no TSA contact, exit as before;
      (b) `build.cmd full release sign` — summary shows 206/0/0, spot-check
      `tandemcommander.exe`, `plugins\zip\zip.spl`, `lang\czech.slg`,
      `utils\sqlite.dll`, `plugins\7zip\7za.dll` via
      `Get-AuthenticodeSignature` (Valid + thumbprint `A3D05CCF…`);
      (c) immediate re-run — Signed 0, Skipped 206, < 1 min (SC-003);
      (d) standalone sweep over existing tree (FR-009); (e) `-VerifyOnly`
      exit 0 on signed tree; (f) `build.cmd sign` (no release) rejected

**Checkpoint**: US1 fully functional — this is the MVP.

---

## Phase 4: User Story 2 — Signed installer and uninstaller (P2)

**Goal**: `setup\build_setup.cmd [sign]` compiles the installer, signed on
demand and never packaging unsigned/shipped-irrelevant files;
`build.cmd … setup` chains the whole release (FR-015).

**Independent Test**: acceptance scenarios 1–5 of Story 2 (spec.md).

- [X] T008 [P] [US2] `setup/tandemcommander.iss`: add `#ifdef SIGN` →
      `SignTool=tcsign` + `SignedUninstaller=yes` → `#endif` in `[Setup]`;
      add `Excludes: "*.pdb,*.lib,*.exp"` to the recursive `[Files]` entry
      (FR-006, FR-014, contract §6)
- [X] T009 [US2] Create `setup/build_setup.cmd` per contract §5: resolve
      repo-relative paths from `%~dp0`; resolve `OPENSAL_BUILD_DIR` default
      `<repo>\build\`; require `Release_x64\tandemcommander.exe`; locate ISCC
      (Inno Setup 7 path → `where iscc` → Inno Setup 6 path); without `sign`
      plain compile; with `sign` run sweep first (abort on failure), read
      codesign.cfg, resolve signtool, compile with
      `/DSIGN=1 "/Stcsign=$q<signtool>$q sign /sha1 <thumb> /tr <tsa> /td sha256 /fd sha256 /v $f"`,
      then verify the produced installer's signature; non-zero exit on any
      failure
- [X] T010 [US2] `build.cmd`: parse `setup` argument (requires `release`,
      same guard pattern as `sign`); after successful build (+ signing when
      requested), call `setup\build_setup.cmd` passing `sign` iff signing was
      requested; propagate failure to `BUILD_EXIT`; extend help + summary
      (FR-015)
- [X] T011 [US2] Verify Story 2 end-to-end and append to
      `specs/050-code-signing/verification.md`: (a) `setup\build_setup.cmd`
      unsigned compile succeeds with no cert dependencies; (b) `setup\build_setup.cmd sign`
      over a *deliberately unsigned* tree signs it before packaging;
      (c) installer exe signature Valid + thumbprint; (d) installed
      `unins000.exe` signed (install to sandbox dir or inspect via
      innounp/administrative install if available — else install+uninstall
      round-trip); (e) installer contains zero `*.pdb/*.lib/*.exp`
      (SC-007); (f) `build.cmd full release sign setup` chains end-to-end
      (SC-008, Story 2 scenario 5)

**Checkpoint**: complete signed release producible with one command.

---

## Phase 5: User Story 3 — Certificate lifecycle in one place (P3)

**Goal**: rotation = one edit in codesign.cfg; old-cert artifacts converge.

**Independent Test**: acceptance scenarios of Story 3 (spec.md).

- [X] T012 [US3] Verify rotation semantics and append to verification.md:
      temporarily point `thumbprint` at a bogus value → pre-flight fails
      before touching files (cert-missing edge case); restore real value;
      confirm classification treats a file signed by a *different* cert as
      re-sign candidate (unit-style: run classification against a
      Microsoft-signed system DLL copied into a temp root with `-VerifyOnly`
      and confirm it reports it as not-ours); confirm quickstart.md rotation
      steps match actual behavior (FR-005, SC-005)

---

## Phase 6: Polish & Cross-Cutting

- [X] T013 [P] Update `README.md`: new "Release & code signing" section —
      prerequisites (cert in store, Inno Setup 7), everyday vs. release
      commands, one-command signed release, standalone sweep, certificate
      rotation, troubleshooting (from quickstart.md), FR-013 note that
      release trees no longer contain `.pdb/.lib/.exp`
- [X] T014 [P] Update `CLAUDE.md`: add feature 050 to Recent Changes (sign /
      setup build.cmd args, tools/codesign/, build_setup.cmd, byproduct
      redirect); keep Active Technologies entries added by the agent-context
      script
- [X] T015 Final acceptance run: `build.cmd full release sign setup` from a
      clean prompt; confirm SC-001…SC-008 evidence recorded in
      verification.md; confirm new/modified text files use the repo encoding
      conventions (new .cmd/.ps1/.cfg ASCII; edited files keep UTF-8-BOM)

---

## Dependencies

```text
T001 ─┬─> T002 ─┬─> T005 ─> T007 ─┐
      │         ├─> T006 ─────────┤ (US1 complete)
      │         ├─> T009 ─> T011  │
      │         └─> T012          │
T003 ─┤ (independent)             ├─> T013/T014 ─> T015
T004 ─┤ (independent)             │
T008 ─┴─> T009/T011               │
T005 ─> T010 ─> T011 ─────────────┘
```

- Story order: US1 → US2 (T010 chains onto T005's arg parsing; T009 needs
  T002) → US3 (validation only, needs T002).
- FR-013 tasks (T003, T004) are story-independent and can land any time
  before T011's SC-007 check.

## Parallel Execution Examples

- After T002: **T003 + T004 + T006 + T008** touch four different files — all
  parallelizable while T005 edits build.cmd.
- Polish: **T013 + T014** in parallel; T015 last.

## Implementation Strategy

MVP = Phase 1 + 2 + 3 (signed build). Then US2 (installer), US3 (rotation
validation), polish. Each checkpoint leaves the repo shippable: default
builds are untouched at every step (FR-002 holds from T005 onward because
the sweep only runs behind the new argument).
