# Open Salamander — Project Context

## What Is This?

Open Salamander is a two-panel file manager for Windows, open-sourced
under GPLv2 in 2023. It is a pure WinAPI C++ application — no MFC,
no Qt, no cross-platform frameworks.

## Current Phase

**Phase 1: Build system analysis** — mapping the project architecture,
dependencies, and compiler options. No code changes yet.

Future phases: fix existing features, add new ones.

## Technology

- **Language**: C++ (C++20, `/std:c++latest`)
- **Compiler**: MSVC v143 (Visual Studio 2022)
- **Platform**: Windows 11+, pure WinAPI
- **Build system**: MSBuild (`.sln` / `.vcxproj` / `.props`)
- **Plugin format**: `.spl` (plugin DLL) + `.slg` (language resource)

## Repository Structure

```
src/                   All source code (~2,224 files)
  common/              Shared libraries and headers
    dep/               Third-party libs (zlib, bzip2, sqlite, fmt, wil...)
  plugins/             35 plugins (archive, viewer, utility, network)
    shared/            Shared plugin build infrastructure
  vcxproj/             VS solution (salamand.sln) and project files
  lang/                English resources for main app
  salmon/              Crash reporter
  shellext/            Shell extension (x86 + x64)
  setup/               Installer/uninstaller
architecture/          Architecture documentation (see below)
convert/               Character conversion tables
doc/                   License files, third-party notices
help/                  User manual source (HTML Help)
tools/                 Build utilities (code signing, timing)
translations/          UI translations
```

## Build Quick Start

```batch
set OPENSAL_BUILD_DIR=D:\Build\OpenSal\
build.cmd                       :: Debug x64 incremental build (from repo root)
build.cmd rebuild               :: Full clean + rebuild Debug x64
build.cmd full                  :: Complete build: also copies runtime data
                                ::   (convert tables, toolbars, scripts) and
                                ::   generates plugins\plugins.ver so all 35
                                ::   plugins auto-register in Plugin Manager
build.cmd full release          :: Complete Release x64 build
```

Alternative scripts in `src\vcxproj\`: `build.cmd` (simple), `rebuild.cmd` (interactive menu).

**Prerequisites**:
- Windows 11 or newer
- Visual Studio 2022 (Community, Professional, or Enterprise)
- "Desktop development with C++" workload installed in VS2022
- Windows 10/11 SDK (any version; projects use `10.0` = latest installed)
- Environment variable `OPENSAL_BUILD_DIR` (optional — defaults to `.\build\`)

## Key Facts

- **90 projects** in salamand.sln (1 main app, 35 plugins, 36 lang
  modules, 7 helper libs, 5 utilities, 2 shell exts, 3 setup, 1 other)
- **All dependencies are embedded** — zero NuGet packages
- **Missing deps**: pvw32cnv.dll (pictview), unrar.dll (unrar),
  OpenSSL (ftp), Embarcadero RTL (winscp — not in repo)
- **Encoding**: UTF-8-BOM, formatted with clang-format
- **Comments**: Legacy Czech OK, new comments in English
- **Debug builds** use fixed base addresses (no ASLR) for leak detection
- **Release builds** use LTO/WPO and code signing

## Architecture Documentation

Detailed analysis is in the `architecture/` directory:

| Document | What It Covers |
|----------|---------------|
| [01-project-overview.md](architecture/01-project-overview.md) | History, tech stack, repo layout |
| [02-solution-structure.md](architecture/02-solution-structure.md) | All 90 projects with categories |
| [03-build-pipeline.md](architecture/03-build-pipeline.md) | Build scripts, configs, output paths |
| [04-dependencies.md](architecture/04-dependencies.md) | Third-party libs, missing deps |
| [05-compiler-comparison.md](architecture/05-compiler-comparison.md) | MSVC vs Clang-cl vs MinGW vs Intel |
| [06-plugin-architecture.md](architecture/06-plugin-architecture.md) | Plugin API, .spl/.slg format |
| [07-preprocessor-defs.md](architecture/07-preprocessor-defs.md) | All #defines by configuration |
| [08-code-standards.md](architecture/08-code-standards.md) | Encoding, formatting, conventions |

## Compiler Recommendation

- **Primary**: MSVC 2022 (full compatibility, zero effort)
- **Secondary CI**: Clang-cl (catches extra bugs, MSBuild-compatible)
- **Not viable**: MinGW-w64 (no x86 SEH, no MSBuild)

## Plugin Build Pattern

Each plugin: `plugins/<name>/vcxproj/<name>.vcxproj` → outputs `<name>.spl`
Each language: `plugins/<name>/vcxproj/lang_<name>.vcxproj` → outputs `english.slg`
Property sheets: `plugins/shared/vcxproj/plugin_base.props` + debug/release variants

## Branching Strategy

- **`main`** — upstream/stable branch
- **`ai-main`** — main branch for AI-assisted development
- Feature branches (e.g., `003-speckit-review`) are created from and merged into `ai-main`

## Constitution

Project principles are in `.specify/memory/constitution.md`:
build reproducibility, backward compatibility, incremental
modernization, Windows platform commitment, plugin architecture
preservation.

<!-- MANUAL ADDITIONS START -->
<!-- MANUAL ADDITIONS END -->

## Active Technologies
- Windows Batch script (.cmd) + MSBuild (from VS2022), vswhere.exe (002-msvc-x64-build-script)
- C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022) + Pure WinAPI (no frameworks); internal shared libs (`src/common/`); no new external dependencies (004-long-paths-unicode)
- Windows Registry for configuration (`REG_SZ` string values); NTFS/exFAT/FAT/network file systems as managed objects (004-long-paths-unicode)

## Recent Changes
- 002-msvc-x64-build-script: Added Windows Batch script (.cmd) + MSBuild (from VS2022), vswhere.exe
