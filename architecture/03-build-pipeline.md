# Build Pipeline

## Prerequisites

| Requirement | Details |
|-------------|---------|
| OS | Windows 11 or newer |
| IDE | [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) (Community, Professional, or Enterprise) |
| Workload | Desktop development with C++ |
| SDK | Windows 11 SDK (10.0.26100.4654) — install as VS optional component |
| Git | Optional — [git-scm.com](https://git-scm.com/downloads) |
| PowerShell | Optional — 7.4+ for normalize.ps1 script |
| HTML Help | Optional — [HTMLHelp Workshop 1.3](https://learn.microsoft.com/en-us/answers/questions/265752/htmlhelp-workshop-download-for-chm-compiler-instal) for help file compilation |

## Environment Variables

| Variable | Required | Description |
|----------|----------|-------------|
| `OPENSAL_BUILD_DIR` | Recommended | Build output directory. **Must have trailing backslash** (e.g., `D:\Build\OpenSal\`). If not set, build may fail or use a default path. |

## Quick Start

```batch
:: 1. Set build output directory
set OPENSAL_BUILD_DIR=D:\Build\OpenSal\

:: 2. Build from the repository root (recommended)
build.cmd                :: Debug x64 incremental build
build.cmd rebuild        :: Full clean + rebuild

:: 3. Or open solution in Visual Studio
start src\vcxproj\salamand.sln
```

## Build Entry Points

### 1. build.cmd — Repository Root (Recommended)

```batch
build.cmd              :: Incremental Debug x64 build
build.cmd rebuild      :: Full clean + rebuild Debug x64
build.cmd help         :: Show usage
```

Auto-detects Visual Studio 2022 via vswhere.exe, validates
prerequisites, and displays a build summary with timing.

### 2. Visual Studio IDE

Open `src\vcxproj\salamand.sln` and build from the IDE. Select
configuration (Debug/Release) and platform (x86/x64) from the toolbar.

### 3. src\vcxproj\build.cmd — Legacy Single Configuration

```batch
src\vcxproj\build.cmd [config] [arch]
```

- Default: `Debug x64`
- Examples: `build.cmd Release x86`, `build.cmd Debug x64`

### 4. src\vcxproj\rebuild.cmd — Legacy Full Rebuild with Menu

```batch
src\vcxproj\rebuild.cmd
```

Interactive menu with options:

| Option | Description |
|--------|-------------|
| 3 | Rebuild all targets |
| 5 | **Internal Build** (Utils + Debug x86/x64) — default |
| 6 | Developers Build (Utils + Debug + Release x86/x64) |
| 8 | Release/Beta Build (Utils + Release x86/x64) |
| 9 | Utils Only (Utils x86/x64) |

Uses MSBuild with parallel compilation (`/m:%NUMBER_OF_PROCESSORS%`).

## Build Configurations

| Configuration | Optimization | Debug Info | ASLR | Code Signing | Key Defines |
|---------------|-------------|------------|------|-------------|-------------|
| Debug\|Win32 | Disabled | Full | Off (fixed base) | No | `_DEBUG`, `TRACE_ENABLE` |
| Debug\|x64 | Disabled | Full | Off (fixed base) | No | `_DEBUG`, `TRACE_ENABLE`, `_WIN64` |
| Release\|Win32 | MaxSpeed + LTO | Yes | On | Yes | `NDEBUG` |
| Release\|x64 | MaxSpeed + LTO | Yes | On | Yes | `NDEBUG`, `_WIN64` |
| Utils\|Win32 | MaxSpeed | Yes | On | Yes | `NDEBUG` |
| Utils\|x64 | MaxSpeed | Yes | On | Yes | `NDEBUG`, `_WIN64` |

Debug builds use fixed base addresses from `baseaddr_x86.txt` /
`baseaddr_x64.txt` to help with debugging and memory leak detection.

## MSBuild Configuration Flow

```
salamand.sln
  ├── Per-project .vcxproj
  │     └── Imports property sheets (.props) in order:
  │           1. x86.props or x64.props (platform macros)
  │           2. sal_base.props / plugin_base.props (shared settings)
  │           3. sal_debug.props / sal_release.props (config-specific)
  │
  ├── Resolves macros:
  │     $(OPENSAL_BUILD_DIR) → build root
  │     $(ShortPlatform) → x86 or x64
  │     $(Configuration) → Debug, Release, or Utils (Release)
  │
  └── Builds in dependency order (parallel where possible)
```

## Key Compiler Settings

| Setting | Value | Flag |
|---------|-------|------|
| C++ Standard | C++20 | `/std:c++latest` |
| Char signedness | Unsigned | `/J` |
| Parallel compilation | Yes | `/MP` |
| Warning level | 3 | `/W3` |
| Precompiled header | precomp.h | `/Yu"precomp.h"` |
| Runtime (Debug) | MultiThreadedDebugDLL | `/MDd` |
| Runtime (Release) | MultiThreadedDLL | `/MD` |

## Output Directory Structure

```
%OPENSAL_BUILD_DIR%salamander\
├── Debug_x86\
│   ├── salamand.exe              Main application
│   ├── lang\english.slg          Main language resources
│   ├── plugins\
│   │   ├── 7zip\
│   │   │   ├── 7zip.spl          Plugin binary
│   │   │   └── lang\english.slg  Plugin language resources
│   │   ├── zip\zip.spl
│   │   ├── tar\tar.spl
│   │   └── [35 plugin directories...]
│   ├── Intermediate\             Object files, PCH cache
│   └── toolbars\                 Toolbar bitmaps
├── Debug_x64\                    Same structure as Debug_x86
├── Release_x86\                  Same structure (signed binaries)
├── Release_x64\                  Same structure (signed binaries)
├── sfx7zip\                      Self-extractor tools
├── translator\                   Translation utility
├── tserver\                      Trace server
├── setup\                        Installer
└── remove\                       Uninstaller
```

## Post-Build Steps

### Code Signing (Release only)

Release builds run `tools\codesign\sign_with_retry.cmd` as a
post-build event to sign executables, plugins, and DLLs.

### Populate Build Directory

After building, run:

```batch
src\vcxproj\!populate_build_dir.cmd
```

This copies runtime dependencies to the build directory:
- **MSVC Redistributables**: concrt140.dll, msvcp140.dll, vcruntime140.dll
- **Universal CRT**: ucrtbase.dll and api-ms-*.dll files
- **Conversion tables**: from `convert\` directory
- **Support files**: automation scripts, CSS files, ZIP2SFX readme

The script references specific redistributable paths under:
- `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\`
- `C:\Program Files (x86)\Windows Kits\10\Redist\`

These paths may need updating for different VS2022 editions or SDK
versions.

## Build Logs

rebuild.cmd generates detailed logs:

| Log | Content |
|-----|---------|
| `rebuild_debug_x86.log` | Full build output |
| `rebuild_debug_x86.log.err` | Errors only |
| `rebuild_debug_x86.log.wrn` | Warnings only |
| `rebuild_times.log` | Build timing |

Similar logs are generated for each configuration.

## Linker Dependencies

### Main Application (salamand.exe)

```
htmlhelp.lib   — HTML Help API
comctl32.lib   — Common controls
mpr.lib        — WAN networking
wsock32.lib    — Winsock
netapi32.lib   — Network APIs
msimg32.lib    — Imaging functions
shlwapi.lib    — Shell utilities
```

Stack reserve: 3 MB (3,145,728 bytes)

### Plugins

```
comctl32.lib   — Common controls (all plugins)
```

Each plugin has a `.def` file for explicit DLL exports.

## Cleaning Build Artifacts

| Script | Description |
|--------|-------------|
| `!clean_all_interm.cmd` | Clean intermediate files from source AND build directories |
| `!clean_src_interm.cmd` | Clean VS cache, .aps, .ncb, .sdf files from source only |
