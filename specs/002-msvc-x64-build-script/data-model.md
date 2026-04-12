# Data Model: MSVC x64 Build Script

**Date**: 2026-03-20
**Branch**: `002-msvc-x64-build-script`

This feature produces a batch script. The "entities" below define
the script's interface and behavior.

## Entity: Build Script (build.cmd)

| Field | Description | Value |
|-------|-------------|-------|
| Location | Path in repo | Repository root (`build.cmd`) |
| Language | Script type | Windows Batch (.cmd) |
| Default action | What happens with no args | Incremental Debug x64 build |
| Rebuild flag | How to force full rebuild | `build.cmd rebuild` |
| Help flag | Show usage | `build.cmd help` |
| Exit code (success) | When build succeeds | 0 |
| Exit code (prereq fail) | When prerequisites missing | 1 |
| Exit code (build fail) | When compilation fails | Non-zero (from MSBuild) |

## Entity: Prerequisites

| Check | How Detected | Error Message |
|-------|-------------|---------------|
| OPENSAL_BUILD_DIR | Environment variable check | "OPENSAL_BUILD_DIR not set. Set it to your build directory with trailing backslash." |
| Visual Studio 2022 | vswhere.exe query | "Visual Studio 2022 not found. Install from visualstudio.microsoft.com" |
| C++ Desktop workload | vswhere component filter | "C++ Desktop workload not installed. Run VS Installer and add it." |
| MSBuild.exe | Derived from VS path | "MSBuild.exe not found at expected location." |

## Entity: Build Output

| Artifact | Path | Description |
|----------|------|-------------|
| Main executable | `%OPENSAL_BUILD_DIR%salamander\Debug_x64\salamand.exe` | Open Salamander application |
| Main language | `%OPENSAL_BUILD_DIR%salamander\Debug_x64\lang\english.slg` | English UI resources |
| Plugins | `%OPENSAL_BUILD_DIR%salamander\Debug_x64\plugins\*\*.spl` | 35 plugin DLLs |
| Plugin languages | `%OPENSAL_BUILD_DIR%salamander\Debug_x64\plugins\*\lang\english.slg` | Plugin UI resources |
| Utilities | `%OPENSAL_BUILD_DIR%salamander\...\` | salopen, salspawn, salmon, tserver, etc. |

## Entity: Build Summary

Displayed at script completion:

| Field | Example |
|-------|---------|
| Status | "BUILD SUCCEEDED" or "BUILD FAILED" |
| Duration | "Build time: 3 minutes 42 seconds" |
| Configuration | "Configuration: Debug x64" |
| Output directory | "Output: D:\Build\OpenSal\salamander\Debug_x64\" |
