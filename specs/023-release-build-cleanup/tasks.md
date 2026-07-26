# Tasks: Clean Release Build Output (No Intermediate / saltests Directories)

**Input**: Design documents from `/specs/023-release-build-cleanup/`
**Prerequisites**: plan.md ✅, spec.md ✅, research.md ✅, data-model.md ✅, quickstart.md ✅

**Tests**: No automated test tasks — this is a build-system change; verification is
via `build.cmd release` + PowerShell directory checks and `MSBuild -getProperty`
(see quickstart.md). No unit-test framework applies.

**Organization**: Tasks grouped by user story. US1 and US2 are both P1 (the two
halves of a clean release tree); US3 is a P2 guardrail.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- File paths are repo-relative.

---

## Phase 1: Setup

**Purpose**: Baseline for before/after verification.

- [x] T001 Set `OPENSAL_BUILD_DIR`, locate VS2022 MSBuild via `vswhere`, and record a baseline listing of all `Intermediate` directories and the `saltests` directory currently present under `build\newtcommander\Release_x64\` (for before/after comparison).

---

## Phase 2: User Story 1 - Release output free of intermediate build artifacts (Priority: P1) 🎯 MVP

**Goal**: No `Intermediate` directory anywhere in the Release output tree, incremental Release builds still fast.

**Independent Test**: After a Release build, `Get-ChildItem $OUT -Directory -Recurse -Filter Intermediate` returns nothing; a second incremental Release build does not full-recompile.

- [x] T002 [US1] Create `src/Directory.Build.targets` that, for `Configuration=Release` only, relocates `IntDir` outside the output tree by rewriting the root segment: `$(IntDir.Replace('newtcommander\$(Configuration)_$(ShortPlatform)\', 'obj\$(Configuration)_$(ShortPlatform)\'))`. Include an explanatory comment. (Debug untouched; no-op for non-salamander-tree projects.)
- [x] T003 [US1] Add a Release-only post-build sweep to `build.cmd` that, after a successful build, removes any residual `Intermediate` directories (recursively) **and** the `saltests` directory from `%OUT_DIR%` (the output tree). Gate on `%BUILD_CONFIG%`=="Release" so Debug keeps its `Intermediate`. (This task also satisfies US2's residual-`saltests` cleanup — single shared edit to `build.cmd`.)
- [x] T004 [US1] Verify with `MSBuild <proj> -getProperty:IntDir -p:Configuration=Release -p:Platform=x64` for `salamand`, a plugin, a plugin `lang_*`, and `salmon` that IntDir now resolves under `obj\Release_x64\…`; and with `-p:Configuration=Debug` that `salamand`/plugin IntDir is unchanged (`newtcommander\Debug_x64\…\Intermediate\`).

**Checkpoint**: Release build produces no `Intermediate` dirs; Debug IntDir unchanged.

---

## Phase 3: User Story 2 - Release output free of test binaries (Priority: P1)

**Goal**: No `saltests` directory in the Release output tree; tests still build/run in Debug.

**Independent Test**: After a Release build, `Test-Path $OUT\saltests` is `False`; a Debug build still produces `saltests.exe`.

- [x] T005 [US2] In `src/vcxproj/salamand.sln`, remove the two Release `Build.0` mappings for the `saltests` project GUID `{D4A7C5B1-9E2F-4F63-A1C8-5B0E7D2F1A44}` (`.Release|x64.Build.0` and `.Release|Win32.Build.0`), keeping the `ActiveCfg` lines and all Debug mappings, so `saltests` is not built in Release but stays buildable in Debug.
- [x] T006 [US2] (Covered by T003.) Confirm the `build.cmd` sweep removes any pre-existing `saltests\` directory from the Release output tree.

**Checkpoint**: Release build no longer builds or leaves a `saltests\` directory; Debug still builds it.

---

## Phase 4: User Story 3 - Debug build behaviour unchanged (Priority: P2)

**Goal**: Debug output retains `Intermediate` directories and the `saltests` binary.

**Independent Test**: After a Debug build, `Test-Path $DBG\Intermediate` and `Test-Path $DBG\saltests` are both `True`.

- [x] T007 [US3] Verify a Debug build (`build.cmd`) is structurally unchanged: `newtcommander\Debug_x64\Intermediate\` and `newtcommander\Debug_x64\saltests\` still present, and `saltests.exe` runs. (Guardrail — satisfied by the Release-only conditions in T002/T003/T005; no code change.)

---

## Phase 5: Polish & Validation

**Purpose**: Full acceptance run against Success Criteria.

- [x] T008 Clean Release build (`build.cmd rebuild release`, or `build.cmd full release`); then run quickstart checks SC-001 (no `Intermediate`) and SC-002 (no `saltests`) against `build\newtcommander\Release_x64\`.
- [x] T009 [P] Verify SC-003: all runtime deliverables still present (`salamand.exe`, `salamand.pdb`, `lang\english.slg`, `plugins\*.spl`, `plugins\plugins.ver`, `convert\`, `toolbars\`, `utils\`).
- [x] T010 Verify SC-004: run `build.cmd release` twice; the second run is a fast incremental build (no full recompilation) — confirming the relocated `obj\` cache is reused.
- [x] T011 [P] Verify SC-005/SC-006: Debug output unchanged and `saltests.exe` (Debug) builds/runs.

---

## Dependencies & Execution Order

- **T001 (Setup)** first.
- **T002, T003** (US1) are the core changes; **T005** (US2) is independent of them (different file: `.sln`). T002/T003/T005 can be authored in parallel (different files) but T003 also removes `saltests` residue relied on by US2.
- **T004** verification depends on T002.
- **T007** (US3) depends on T002/T003/T005 being in place (verifies they didn't leak into Debug).
- **Phase 5** validation depends on all of T002–T005.

## Parallel Opportunities

- T002 (`Directory.Build.targets`), T003 (`build.cmd`), T005 (`salamand.sln`) touch three different files → can be edited in parallel.
- T009 and T011 are independent read-only verifications → [P].

## Implementation Strategy

MVP = US1 + US2 (both P1) — together they deliver the clean release tree. US3 is a
verification guardrail. Recommended order: T001 → T002 → T003 → T005 → T004 →
clean build (T008) → remaining verifications (T007, T009–T011).

## Notes

- Relocation (not deletion) of intermediates is what keeps incremental Release
  builds fast (FR-005); the `build.cmd` sweep only ever removes stale/empty
  scaffolding because the live cache now lives under `obj\`, outside the tree.
- No `.cpp`/`.h`, per-project `.vcxproj`/`.props`, or `plugins.cfg` changes.
