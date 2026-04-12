# Compiler Comparison

This document evaluates compiler options for building Open Salamander,
analyzing compatibility with the existing codebase and practical
trade-offs for each option.

## Current Compiler: MSVC 2022 (v143)

Open Salamander is built with MSVC v143 (Visual Studio 2022). The
entire build system (MSBuild, .vcxproj, .props) and codebase are
designed for this compiler.

## MSVC-Specific Code Patterns in the Codebase

Before evaluating alternatives, here is an inventory of MSVC-specific
patterns that affect compiler compatibility:

| Pattern | Occurrences | Files | Portability Impact |
|---------|-------------|-------|--------------------|
| `#pragma once` | 609 | 609 | Low — widely supported |
| `#pragma warning(...)` | 218 | 81 | Medium — MSVC-only |
| `#pragma comment(lib, ...)` | 19 | 15 | Medium — linker directive |
| `__declspec(dllexport/import)` | 13+ | 13 | Medium — has alternatives |
| `__forceinline` | 76 | 14 | Low — replaceable |
| `__assume()` | 6 | 2 | Low — removable |
| `_MSC_VER` checks | 280 | 128 | Medium — need expansion |
| `__try/__except` (SEH) | — | 89 | **Very High** — critical |
| `__int64/__int32` types | — | 132 | Low — use stdint.h |
| Calling conventions | 5,301 | 135 | High — Win32 requirement |
| SAL annotations (`_In_`, `_Out_`) | 650 | 29 | Low — ignored by others |
| Windows API types (HWND, etc.) | 3,000+ | 364+ | Critical — by design |
| COM/ATL/WTL patterns | 76+ | 13+ | High — Windows-only |

### Critical Blockers

1. **SEH (Structured Exception Handling)**: Used in 89 files for crash
   handling, error recovery, and debugging. `__try/__except/__finally`
   is fundamental to the application's stability.

2. **Windows API**: The entire application is built on HWND-based
   window management, COM shell integration, and WinAPI. This is by
   design, not a limitation.

3. **COM/ATL/WTL**: Used by the automation plugin (IDispatch), the
   portables plugin (WPD), and IE viewer (WebBrowser control).

## Compiler Evaluation

### 1. MSVC 2022 Build Tools — RECOMMENDED PRIMARY

| Criterion | Assessment |
|-----------|-----------|
| **Installation** | `winget install Microsoft.VisualStudio.2022.BuildTools` + C++ workload |
| **Install size** | 4–12 GB (depends on workloads selected) |
| **Self-contained** | No — needs Windows SDK separately |
| **C++20 support** | Complete (`/std:c++latest`) |
| **SEH support** | Full native support |
| **MSBuild** | Native — .vcxproj files work as-is |
| **Windows SDK headers** | Native — designed for MSVC |
| **Effort to adopt** | Zero — current compiler |
| **License** | Free (Build Tools / Community edition) |
| **Verdict** | **COMPATIBLE — zero changes needed** |

**Pros**: Perfect compatibility, native MSBuild, full debugging, all
Windows SDK features, no migration effort.

**Cons**: Large installation, requires Microsoft account for Community
edition after 30 days, Windows-only.

### 2. Clang-cl (LLVM for Windows) — VIABLE SECONDARY

| Criterion | Assessment |
|-----------|-----------|
| **Installation** | `winget install LLVM.LLVM` (~1.5 GB) + MSVC Build Tools + Windows SDK |
| **Install size** | 6–8 GB total (LLVM itself is small but needs MSVC headers/libs) |
| **Self-contained** | No — requires MSVC headers, libraries, and Windows SDK |
| **C++20 support** | Complete |
| **SEH support** | Yes — supported in clang-cl mode (minor edge cases) |
| **MSBuild** | Yes — via `ClangCL` platform toolset in .vcxproj |
| **Windows SDK headers** | Via MSVC — fully compatible |
| **Effort to adopt** | Medium — change PlatformToolset, fix warnings |
| **License** | Apache 2.0 with LLVM exception (free, open source) |
| **Verdict** | **PARTIALLY COMPATIBLE — good as secondary CI compiler** |

**Pros**: Catches bugs MSVC misses (different warning set), superior
error messages, MSBuild integration via ClangCL toolset, supports most
MSVC extensions (`__declspec`, `#pragma comment`, SEH).

**Cons**: Requires MSVC headers/SDK anyway (not standalone), some
WIL/ATL headers may produce warnings, SEH behavior may differ in
edge cases, not all MSVC-specific pragmas are mapped.

**Integration steps**:
1. Install LLVM via winget
2. Set `PlatformToolset` to `ClangCL` in .vcxproj (or via MSBuild property)
3. Fix Clang-specific warnings (estimated: dozens to hundreds)
4. Verify SEH behavior in crash handling paths
5. Test all plugins (especially automation, portables with COM/WTL)

### 3. MinGW-w64 (GCC for Windows) — NOT VIABLE

| Criterion | Assessment |
|-----------|-----------|
| **Installation** | `winget install MSYS2.MSYS2` then `pacman -S mingw-w64-x86_64-gcc` |
| **Install size** | 1.5–2.5 GB (self-contained, smallest option) |
| **Self-contained** | **Yes** — does not need MSVC or Windows SDK |
| **C++20 support** | Complete |
| **SEH support** | **x64 only** — NO x86 SEH (uses SJLJ or DWARF) |
| **MSBuild** | **Not supported** — requires CMake or Makefile |
| **Windows SDK headers** | Own reimplementation (mingw-w64) — gaps for COM/ATL/WTL |
| **Effort to adopt** | **Extreme** — full build system rewrite + code changes |
| **License** | GPL (free, open source) |
| **Verdict** | **INCOMPATIBLE for this project** |

**Pros**: Smallest install size, completely self-contained, no Microsoft
dependencies, good for simple C/C++ projects.

**Cons** (critical for this project):
- **No x86 SEH**: Cannot build the 32-bit configuration at all (89 files use SEH)
- **No MSBuild**: Would require rewriting the entire build system (90 projects, 16+ .props files) to CMake or Makefiles
- **Incomplete WinAPI headers**: mingw-w64 reimplements Windows headers but has gaps for newer APIs, COM interfaces, WTL, ATL
- **Different ABI**: C++ name mangling and vtable layout differ from MSVC — binary incompatibility with Windows system DLLs that expect MSVC ABI
- **No `#pragma comment(lib)`**: All 19 linker directives would need to be moved to build system configuration

### 4. Intel oneAPI DPC++/C++ (ICX) — NOT RECOMMENDED

| Criterion | Assessment |
|-----------|-----------|
| **Installation** | `winget install Intel.oneAPI.DPC++Compiler` + MSVC Build Tools |
| **Install size** | 7–11 GB (Intel + MSVC + Windows SDK) |
| **Self-contained** | No — requires MSVC headers/libs |
| **C++20 support** | Complete (LLVM-based) |
| **SEH support** | Same as clang-cl (LLVM-based) |
| **MSBuild** | Yes — Intel compiler toolset |
| **Effort to adopt** | Medium (same as clang-cl) |
| **License** | Free (no-cost, proprietary) |
| **Verdict** | **PARTIALLY COMPATIBLE but no advantage over clang-cl** |

**Pros**: LLVM-based (same benefits as clang-cl), Intel-specific
optimizations (SIMD, vectorization).

**Cons**: Largest installation, requires MSVC anyway, Intel optimizations
irrelevant for a file manager UI, same compatibility issues as clang-cl
with extra weight.

### 5. Embarcadero C++ Builder — NOT APPLICABLE

Only relevant for the WinSCP plugin dependency (not in the repository).
Expensive ($1,500+), uses its own VCL/RTL (different ABI), cannot
compile the main codebase. **Recommendation**: Replace the WinSCP
dependency with libssh2 for SFTP support.

## Comparison Matrix

| Criterion | MSVC 2022 | Clang-cl | MinGW-w64 | Intel ICX |
|-----------|-----------|----------|-----------|-----------|
| Single command install | Mostly | Yes | Yes | Yes |
| Self-contained | No | No | **Yes** | No |
| Install size | 4–12 GB | 6–8 GB | **1.5–2.5 GB** | 7–11 GB |
| C++20 | Full | Full | Full | Full |
| SEH (x86 + x64) | **Full** | **Yes** | x64 only | Yes |
| MSBuild / .vcxproj | **Native** | **Yes** | No | Yes |
| Windows SDK headers | **Native** | Via MSVC | Own (gaps) | Via MSVC |
| Effort to adopt | **Zero** | Medium | Extreme | Medium |
| Extra bug detection | Baseline | **Yes** | Yes | Yes |
| Compatibility | **Full** | Partial | Incompatible | Partial |
| License | Free | Free/OSS | Free/OSS | Free |

## Recommendation

1. **Primary compiler: MSVC 2022 Build Tools**
   Zero migration effort, full compatibility, proven with this codebase.
   Use the free Build Tools edition for CI/CD pipelines.

2. **Optional secondary: Clang-cl for CI**
   Worth adding as a CI build step to catch latent bugs with different
   warnings. The ClangCL MSBuild toolset makes integration feasible
   without rewriting build files. Requires thorough SEH testing first.

3. **Not viable: MinGW-w64**
   Despite the smallest install size and self-contained nature, the
   lack of x86 SEH, MSBuild support, and complete WinAPI headers makes
   this impractical for a project of this nature.

4. **Not needed: Intel ICX**
   Offers no meaningful benefit over clang-cl for a file manager.
   The optimization advantages are irrelevant for UI-bound applications.
