# Quickstart: MSVC x64 Build Script

**Date**: 2026-03-20
**Branch**: `002-msvc-x64-build-script`

## How to Use

### First-time build

```batch
:: 1. Set the build output directory (trailing backslash required)
set OPENSAL_BUILD_DIR=D:\Build\OpenSal\

:: 2. Run from repository root
build.cmd
```

The script will:
1. Check all prerequisites (VS2022, C++ workload, SDK, OPENSAL_BUILD_DIR)
2. Build all 90 projects in Debug x64 configuration
3. Display a summary with build time and status

### Full rebuild (clean + rebuild)

```batch
build.cmd rebuild
```

### Show help

```batch
build.cmd help
```

## How to Verify

### 1. Build completes successfully

Run `build.cmd` and verify:
- Exit code is 0 (`echo %ERRORLEVEL%`)
- Summary shows "BUILD SUCCEEDED"
- Build time is displayed

### 2. Output files exist

Check that these files exist in the build directory:
- `%OPENSAL_BUILD_DIR%salamander\Debug_x64\salamand.exe`
- `%OPENSAL_BUILD_DIR%salamander\Debug_x64\lang\english.slg`
- At least one plugin: `%OPENSAL_BUILD_DIR%salamander\Debug_x64\plugins\zip\zip.spl`

### 3. Application launches

Run salamand.exe from the build directory and verify the file
manager UI appears.

### 4. Prerequisite detection works

Temporarily unset OPENSAL_BUILD_DIR and run build.cmd:
```batch
set OPENSAL_BUILD_DIR=
build.cmd
```
Verify the script reports the missing variable and exits with code 1.

### 5. Incremental build is faster

1. Run a full build: `build.cmd rebuild`
2. Note the build time
3. Modify one source file
4. Run incremental build: `build.cmd`
5. Verify the build time is significantly shorter
