# Research: MSVC x64 Build Script

**Date**: 2026-03-20
**Branch**: `002-msvc-x64-build-script`

## R1: Existing Build Scripts Analysis

### Decision
Create a new build script at the repository root that improves upon
the existing `src/vcxproj/build.cmd` while preserving backward
compatibility with the existing scripts.

### Findings

**Existing scripts**:
- `src/vcxproj/rebuild.cmd` — Interactive menu-driven full rebuild.
  Hardcodes MSBuild path to VS2022 Community edition. Uses file
  logging with windows-1250 encoding. Supports Debug/Release/Utils
  for both x86/x64.
- `src/vcxproj/build.cmd` — Simple single-config build. Checks PATH
  for msbuild, falls back to hardcoded Community path. Default is
  Debug x64. Uses `/t:build` (incremental).
- `src/vcxproj/!populate_build_dir.cmd` — Post-build script to copy
  MSVC/UCRT redistributables. Hardcodes specific redist versions.

**Problems with existing scripts**:
1. Hardcoded MSBuild path — only works with Community edition
2. No vswhere.exe detection — doesn't find Professional/Enterprise
3. No prerequisite validation — fails with cryptic errors if VS not installed
4. Located in `src/vcxproj/` — not discoverable from repo root
5. rebuild.cmd is interactive (menu) — blocks automation
6. No build summary with timing and error counts

### Alternatives Considered
- **Modify existing build.cmd**: Rejected — existing scripts should
  remain for backward compatibility
- **CMake**: Rejected — would require rewriting all 90 .vcxproj files
- **PowerShell script**: Considered but rejected — batch script is
  simpler, has no external dependencies, matches existing convention

---

## R2: MSBuild and VS Detection Best Practices

### Decision
Use `vswhere.exe` (ships with VS2022 installer) to dynamically locate
the Visual Studio installation and MSBuild path.

### Rationale
`vswhere.exe` is the Microsoft-recommended way to locate VS
installations. It handles Community, Professional, Enterprise, and
Build Tools editions. It is installed at a fixed path:
`%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe`

### Detection Strategy
1. Check if `vswhere.exe` exists at the standard installer path
2. Use vswhere to find VS2022+ with the C++ Desktop workload
3. Derive MSBuild path from the VS installation path
4. Fall back to PATH search if vswhere is not available

### MSBuild Command Line for Debug x64

```batch
MSBuild.exe salamand.sln /t:build /p:Configuration=Debug /p:Platform=x64 /m:%NUMBER_OF_PROCESSORS%
```

For full rebuild, change `/t:build` to `/t:rebuild`.

---

## R3: Prerequisite Validation

### Decision
Check all prerequisites before starting the build and report all
missing items at once (not one at a time).

### Prerequisites to Check
1. **OPENSAL_BUILD_DIR** — environment variable must be set with
   trailing backslash
2. **Visual Studio 2022** — detected via vswhere
3. **C++ Desktop workload** — detected via vswhere component check
4. **MSBuild.exe** — derived from VS installation path
5. **Windows SDK** — checked by attempting to locate SDK headers

### Error Reporting
Each missing prerequisite produces a specific, actionable message
telling the user exactly what to install and how.

---

## R4: Build Script Location and Interface

### Decision
Place the new script at `build.cmd` in the repository root.

### Rationale
- Discoverable: first place a developer looks
- Convention: many projects have build.cmd/build.sh at root
- CLAUDE.md already documents `src/vcxproj/build.cmd` — the new
  root-level script replaces this as the primary entry point

### Command-Line Interface

```
build.cmd [options]

Options:
  (no args)     Incremental Debug x64 build (default)
  rebuild       Full clean + rebuild
  help          Show usage information
```

### Exit Codes
- 0: Build succeeded
- 1: Prerequisites missing
- Non-zero (from MSBuild): Build failed
