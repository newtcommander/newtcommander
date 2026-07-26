# Research: Project Build and Architecture Analysis

**Date**: 2026-03-20
**Branch**: `001-project-build-analysis`

## R1: Solution Structure and Project Inventory

### Decision
Document all 90 projects in salamand.sln organized by category.

### Findings
The solution contains 90 C++ projects (platform toolset v143):
- **1 main application**: salamand.exe
- **1 main language resource**: lang (english.slg)
- **35 plugins** (.spl output): unfat, 7zip, tar, unarj, uncab, unchm,
  undelete, uniso, unlha, unmime, unole, unrar, zip, pictview, mmviewer,
  ieviewer, peviewer, filecomp, folders, diskmap, checksum, checkver,
  dbviewer, renamer, splitcbn, regedt, ftp, nethood, wmobile, demoplug,
  demoview, demomenu, pak, automation, portables
- **35 language modules** (.slg output): one per plugin
- **2 shell extensions**: salextx86, salextx64
- **6 utility executables**: salopen, salspawn, salmon, tserver,
  translator, zip2sfx
- **4 helper libraries/tools**: 7za, 7zwrapper, chmlib, exif, salpvenv,
  fcremote, sqlite
- **3 setup/install**: setup, remove, sfx7zip

### Build Configurations
- Debug|Win32, Debug|x64
- Release|Win32, Release|x64
- Utils (Release)|Win32, Utils (Release)|x64

### Property Sheet Hierarchy
Main app: x86/x64.props → sal_base.props → sal_debug/release.props
Plugins: x86/x64.props → plugin_base.props → plugin_debug/release.props
Language: x86/x64.props → lang_base.props → lang_debug/release.props

All props files are in `src/vcxproj/` (main) and
`src/plugins/shared/vcxproj/` (plugins).

---

## R2: Third-Party Dependencies

### Decision
Catalog all included and missing dependencies with license status.

### Included Libraries (GPLv2-compatible)
| Library | Location | License | Status |
|---------|----------|---------|--------|
| zlib | src/common/dep/zlib/ | zlib License | Available |
| bzip2 | src/common/dep/bzip2/ | BSD-like | Available |
| AES (Brian Gladman) | src/common/dep/crypt/ | BSD | Available |
| SQLite | src/common/dep/sqlite/ | Public Domain | Available |
| libexif | src/plugins/pictview/ | LGPL 2.1 | Available |
| CHMLIB | src/plugins/unchm/ | LGPL 2.1 | Available |
| Nano SVG | src/common/dep/nanosvg/ | zlib License | Available |
| cmark-gfm | src/plugins/ieviewer/ | BSD 2-Clause | Available |
| PNGLite | src/common/dep/pnglite/ | zlib License | Available |
| WIL | src/common/dep/wil/ | MIT | Available |
| fmt | src/common/dep/fmt/ | MIT | Available |

### Missing/External Dependencies
| Dependency | Plugin | Issue | Replacement Candidate |
|------------|--------|-------|----------------------|
| pvw32cnv.dll | pictview | Proprietary, not OSS | WIC (Windows Imaging) |
| unrar.dll | unrar | Not included (rarlab) | Downloadable from rarlab.com |
| OpenSSL | ftp | Not included | Build from source or vcpkg |
| Embarcadero RTL | winscp | Requires C++ Builder | libssh2 (SFTP replacement) |

### No External Package Managers
Zero NuGet packages. All dependencies are embedded source or pre-built
in `src/plugins/shared/libs/`.

---

## R3: MSVC-Specific Code Patterns Analysis

### Decision
Quantify all compiler-specific patterns to assess portability.

### Pattern Inventory

| Pattern | Occurrences | Files | Portability Impact |
|---------|-------------|-------|--------------------|
| #pragma once | 609 | 609 | Low (widely supported) |
| #pragma warning | 218 | 81 | Medium (MSVC-only) |
| #pragma comment(lib) | 19 | 15 | Medium (linker directive) |
| __declspec(dllexport/import) | 13+ | 13 | Medium (has alternatives) |
| __forceinline | 76 | 14 | Low (replaceable) |
| __assume() | 6 | 2 | Low (removable) |
| _MSC_VER checks | 280 | 128 | Medium (need expansion) |
| __try/__except (SEH) | 89+ files | 89 | **Very High** |
| __int64/__int32 types | 132 files | 132 | Low (use stdint.h) |
| Calling conventions | 5,301 | 135 | High (Win32 requirement) |
| SAL annotations | 650 | 29 | Low (ignored by others) |
| Windows API types | 3,000+ | 364+ | **Critical** (by design) |
| COM/ATL patterns | 76+ | 13+ | High (Windows-only) |

### Key Blockers for Alternative Compilers
1. **SEH** (89 files): __try/__except is critical for crash handling.
   Clang-cl supports it; MinGW supports it on x64 only, NOT x86.
2. **Calling conventions**: Required by Windows API — all compilers
   targeting Windows support these, but syntax may differ.
3. **#pragma comment(lib)**: MSVC/clang-cl only — MinGW requires
   separate linker configuration.
4. **WTL/ATL**: Used by portables and automation plugins — MSVC-only.

---

## R4: Compiler Comparison

### Decision
Evaluate MSVC, Clang-cl, MinGW-w64, and Intel ICX for this project.

### MSVC 2022 (v143) — Current Compiler

- **Installation**: VS2022 Community (free) or Build Tools via winget
  `winget install Microsoft.VisualStudio.2022.BuildTools`
- **Size**: 4–12 GB depending on workloads
- **C++20**: Complete support (/std:c++latest)
- **SEH**: Full native support
- **MSBuild**: Native integration
- **Verdict**: **COMPATIBLE** — zero changes needed

### Clang-cl (LLVM for Windows)

- **Installation**: `winget install LLVM.LLVM` (~1.5 GB) but still
  requires MSVC headers/libs and Windows SDK
- **Total size**: ~6–8 GB (LLVM + MSVC Build Tools + Windows SDK)
- **C++20**: Complete support
- **SEH**: Supported (minor edge-case differences)
- **MSBuild**: Yes — via ClangCL platform toolset in .vcxproj
- **MSVC pragma support**: Good — #pragma once, #pragma comment(lib),
  most #pragma warning mapped
- **__declspec**: Fully supported in clang-cl mode
- **Known issues**: Some WIL/ATL headers may produce warnings; SEH
  in deeply nested scenarios may behave differently
- **Effort**: Medium — change PlatformToolset to ClangCL, fix warnings
- **Verdict**: **PARTIALLY COMPATIBLE** — viable as secondary CI
  compiler for catching bugs; not recommended as primary without
  thorough SEH testing

### MinGW-w64 (GCC for Windows)

- **Installation**: `winget install MSYS2.MSYS2` then
  `pacman -S mingw-w64-x86_64-gcc` (~1.5–2.5 GB)
- **Total size**: 1.5–2.5 GB (self-contained, no MSVC needed)
- **C++20**: Complete support
- **SEH**: x64 only — **NO x86 SEH support** (uses SJLJ or DWARF)
- **MSBuild**: Not supported — requires CMake or Makefile migration
- **Windows SDK**: Uses own reimplemented headers (mingw-w64) — gaps
  exist for newer APIs, COM interfaces, WTL
- **#pragma comment(lib)**: Not supported
- **__declspec**: Supported via __attribute__ equivalents
- **Known blockers**:
  - No x86 SEH → cannot build 32-bit configuration
  - No MSBuild → requires complete build system rewrite
  - Incomplete WinAPI headers for COM/ATL/WTL
  - Different ABI from MSVC (C++ name mangling, vtable layout)
- **Effort**: Extreme — full build system rewrite + code changes
- **Verdict**: **INCOMPATIBLE** for this project in current state

### Intel oneAPI DPC++/C++ (ICX)

- **Installation**: `winget install Intel.oneAPI.DPC++Compiler` (~2 GB)
  but requires MSVC headers/libs
- **Total size**: 7–11 GB (Intel + MSVC + Windows SDK)
- **C++20**: Complete (based on LLVM/Clang)
- **SEH**: Same as clang-cl (LLVM-based)
- **MSBuild**: Yes — Intel toolset integration
- **Effort**: Same as clang-cl (it IS clang-cl + Intel optimizations)
- **Verdict**: **PARTIALLY COMPATIBLE** — no advantage over clang-cl
  for a file manager; adds unnecessary weight

### Embarcadero C++ Builder

- **Relevance**: Only for WinSCP plugin dependency
- **License**: Paid ($1,500+); Community edition restricted
- **Verdict**: **NOT APPLICABLE** — replace WinSCP dependency with
  libssh2 instead

### Comparison Matrix

| Criterion | MSVC | Clang-cl | MinGW-w64 | Intel ICX |
|-----------|------|----------|-----------|-----------|
| Install command | winget | winget | winget+pacman | winget |
| Self-contained | No (needs SDK) | No (needs MSVC) | **Yes** | No (needs MSVC) |
| Install size | 4–12 GB | 6–8 GB | **1.5–2.5 GB** | 7–11 GB |
| C++20 | Full | Full | Full | Full |
| SEH (x86+x64) | **Full** | **Yes** | x64 only | Yes |
| MSBuild/.vcxproj | **Native** | **Yes** | No | Yes |
| Windows SDK headers | **Native** | Via MSVC | Own (gaps) | Via MSVC |
| Effort to adopt | **Zero** | Medium | Extreme | Medium |
| Extra bug detection | Baseline | **Yes** | Yes | Yes |
| Compatibility | **Full** | Partial | Incompatible | Partial |

### Recommendation
1. **Primary**: MSVC 2022 Build Tools (zero-effort, full compatibility)
2. **Secondary CI**: Clang-cl (catches additional bugs, MSBuild-native)
3. **Not viable**: MinGW-w64 (no x86 SEH, no MSBuild, ABI mismatch)
4. **Not needed**: Intel ICX (clang-cl with extra weight)

---

## R5: Build Pipeline Analysis

### Decision
Document the complete build-to-executable flow.

### Prerequisites
- Windows 11
- Visual Studio 2022 (Community or Build Tools)
- Desktop development with C++ workload
- Windows 11 SDK (10.0.26100)
- Environment variable: `OPENSAL_BUILD_DIR` (trailing backslash)

### Build Entry Points
1. **rebuild.cmd** — Full rebuild with menu (Internal/Developer/Release)
2. **build.cmd** — Single-configuration build (params: config, arch)
3. **Visual Studio** — Open salamand.sln, build from IDE

### Build Flow
1. MSBuild loads salamand.sln
2. Property sheets resolve paths (OPENSAL_BUILD_DIR, ShortPlatform)
3. Projects build in dependency order (parallel /m flag)
4. Output: salamand.exe + english.slg + plugins/*.spl + plugins/*/lang/english.slg
5. Post-build: code signing (Release only)
6. !populate_build_dir.cmd copies MSVC/UCRT redistributables

### Output Structure
```
%OPENSAL_BUILD_DIR%newtcommander/
├── Debug_x86/           (or Release_x86, Debug_x64, Release_x64)
│   ├── salamand.exe     (main application)
│   ├── lang/english.slg (main language)
│   ├── plugins/
│   │   ├── 7zip/7zip.spl
│   │   ├── 7zip/lang/english.slg
│   │   ├── zip/zip.spl
│   │   └── [35+ plugin directories]
│   └── Intermediate/    (obj files, PCH)
├── sfx7zip/
├── translator/
├── tserver/
├── setup/
└── remove/
```

---

## R6: Code Standards

### Decision
Document encoding and formatting requirements.

### Findings
- **Encoding**: UTF-8-BOM for all source files
- **Formatting**: clang-format (config in repo root)
- **Normalization**: normalize.ps1 with normalize_config.json
- **Comments**: Legacy Czech comments acceptable; new comments in English
- **C++ Standard**: C++20 (/std:c++latest)
- **Char type**: unsigned char default (/J compiler flag)
