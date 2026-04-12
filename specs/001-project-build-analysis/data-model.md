# Data Model: Project Build and Architecture Analysis

**Date**: 2026-03-20
**Branch**: `001-project-build-analysis`

This feature produces documentation, not code. The "entities" below
define the information structures that each architecture document
captures.

## Entity: Solution Project

Represents one project entry in salamand.sln.

| Field | Description | Example |
|-------|-------------|---------|
| Name | Project name in solution | salamand |
| GUID | Unique project identifier | {FA82995C-...} |
| Type | Output type | Application, DynamicLibrary |
| Output Extension | .exe, .spl, .slg, .dll | .spl |
| Category | Functional grouping | Core, Plugin, Language, Utility |
| Path | Relative path to .vcxproj | ../plugins/zip/vcxproj/zip.vcxproj |
| Dependencies | Other projects it references | lang, fcremote |
| Configurations | Active build configs | Debug/Release × x86/x64 |

**Count**: 90 projects total

## Entity: Third-Party Dependency

Represents an external library used by the project.

| Field | Description | Example |
|-------|-------------|---------|
| Name | Library name | zlib |
| Location | Path in repo | src/common/dep/zlib/ |
| Version | Library version | 1.3 |
| License | License type | zlib License |
| GPLv2 Compatible | Yes/No | Yes |
| Status | Available/Missing | Available |
| Used By | Projects that depend on it | salamand, zip |
| Replacement | OSS alternative if missing | WIC for pvw32cnv |

**Count**: 11 included, 4 missing/external

## Entity: Build Configuration

Represents a build mode combination.

| Field | Description | Example |
|-------|-------------|---------|
| Name | Configuration label | Debug\|x64 |
| Build Type | Debug, Release, Utils | Debug |
| Platform | x86, x64 | x64 |
| Preprocessor Defines | Active #defines | _DEBUG, TRACE_ENABLE |
| Optimization | Compiler optimization level | Disabled / MaxSpeed |
| Runtime Library | CRT linkage | MultiThreadedDebugDLL |
| ASLR | Address randomization | Disabled (debug) |
| Code Signing | Post-build signing | No (debug) / Yes (release) |
| Output Dir | Build output path | $(OPENSAL_BUILD_DIR)salamander\Debug_x64\ |

**Count**: 6 configurations (3 types × 2 platforms)

## Entity: Compiler Option

Represents a compiler toolchain evaluated for the project.

| Field | Description | Example |
|-------|-------------|---------|
| Name | Compiler name | Clang-cl |
| Version | Current version | 18.x |
| Install Method | How to install | winget install LLVM.LLVM |
| Install Size | Disk space needed | 6–8 GB |
| Self-Contained | Needs MSVC/SDK? | No (needs MSVC) |
| C++20 Support | Standard compliance | Full |
| SEH Support | __try/__except | Yes |
| MSBuild Support | .vcxproj compatible | Yes (ClangCL toolset) |
| Compatibility | Full/Partial/Incompatible | Partial |
| Effort | Migration work estimate | Medium |
| Recommendation | Primary/Secondary/Not viable | Secondary CI |

**Count**: 4 compilers evaluated + 1 niche (Embarcadero)

## Entity: Preprocessor Definition

Represents a #define used in the build system.

| Field | Description | Example |
|-------|-------------|---------|
| Name | Macro name | TRACE_ENABLE |
| Value | Macro value (if any) | (none) |
| Scope | Where defined | sal_debug.props |
| Configurations | Active in which configs | Debug only |
| Purpose | What it controls | Enables trace server output |

**Count**: ~30 unique definitions across all .props files
