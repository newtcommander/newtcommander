# Implementation Plan: MSVC x64 Build Script

**Branch**: `002-msvc-x64-build-script` | **Date**: 2026-03-20 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/002-msvc-x64-build-script/spec.md`

## Summary

Create a new `build.cmd` at the repository root that builds the
complete 64-bit Debug version of Open Salamander using MSVC 2022.
The script uses `vswhere.exe` for robust VS detection (supporting
Community, Professional, Enterprise, and Build Tools editions),
validates all prerequisites, supports both incremental and full
rebuild modes, and provides clear error reporting with build summary.

## Technical Context

**Language/Version**: Windows Batch script (.cmd)
**Primary Dependencies**: MSBuild (from VS2022), vswhere.exe
**Storage**: N/A
**Testing**: Manual — run the script and verify build output
**Target Platform**: Windows 11+
**Project Type**: Build automation script
**Performance Goals**: Parallel compilation via /m flag
**Constraints**: Must use batch (.cmd) to match existing convention;
  no PowerShell or external tools beyond what ships with VS2022
**Scale/Scope**: Single script file, ~100–150 lines

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Build Reproducibility | **PASS** | This feature directly implements single-command automated builds |
| II. Backward Compatibility | **PASS** | Existing scripts in src/vcxproj/ are preserved unchanged |
| III. Incremental Modernization | **PASS** | New script improves build experience without touching source code |
| IV. Windows Platform Commitment | **PASS** | Uses MSVC 2022 on Windows as required |
| V. Plugin Architecture Preservation | **PASS** | Builds all 35 plugins as part of the solution |

All gates pass. No violations.

## Project Structure

### Documentation (this feature)

```text
specs/002-msvc-x64-build-script/
├── plan.md              # This file
├── research.md          # Build system research
├── data-model.md        # Script interface definition
└── quickstart.md        # How to use the build script
```

### Source Code (repository root)

```text
build.cmd                       # NEW — primary build entry point
CLAUDE.md                       # UPDATE — add build.cmd reference
architecture/03-build-pipeline.md  # UPDATE — document new script
```

**Structure Decision**: Single batch script at repository root.
Existing `src/vcxproj/build.cmd` and `src/vcxproj/rebuild.cmd` are
preserved unchanged for backward compatibility.

## Complexity Tracking

No violations — no complexity tracking needed.
