# Coding Standards

This document describes the coding standards and conventions used in the Open Salamander codebase.

## Source File Encoding

All source files use **UTF-8 with BOM** (UTF-8-BOM) encoding with platform-appropriate line endings (CRLF on Windows).

The `normalize.ps1` script enforces this automatically. It processes a broad set of file types defined in `normalize_config.json` under the `textfiles` section, including `.cpp`, `.h`, `.c`, `.rc`, `.vcxproj`, `.props`, `.sln`, `.ps1`, and many others. Third-party vendored directories (e.g., `src/common/dep/`, `src/sfx7zip/7zip/`, `src/plugins/winscp/`) are excluded from normalization.

## Code Formatting

### clang-format

The repository root contains a `.clang-format` configuration applied to all `*.cpp`, `*.h`, and `*.c` files (excluding `.git/` and generated code). The key settings are:

| Setting | Value | Meaning |
|---|---|---|
| `BasedOnStyle` | LLVM | Base style |
| `IndentWidth` | 4 | Four-space indentation |
| `BreakBeforeBraces` | Allman | Opening brace on its own line |
| `NamespaceIndentation` | All | Indent namespace contents |
| `ColumnLimit` | 0 | No line length limit |
| `SortIncludes` | false | Preserve existing include order |
| `ReflowComments` | false | Do not rewrap comments |
| `CommentPragmas` | `'^'` | Treat all comments as pragmas (prevents reformatting) |
| `DerivePointerAlignment` | false | Use explicit pointer alignment |
| `PointerAlignment` | Left | `int* ptr` style |
| `AccessModifierOffset` | -4 | `public:`/`private:` outdented to class level |
| `AlignEscapedNewlines` | DontAlign | No alignment of backslash continuations |

Third-party directories contain their own `.clang-format` files with `DisableFormat: true` to prevent formatting vendored code.

### normalize.ps1

The `normalize.ps1` PowerShell script is the single entry point for all source normalization. It performs two jobs:

1. **clang-format** -- formats C/C++ files according to the root `.clang-format`.
2. **Text normalization** -- converts files to UTF-8 with BOM and platform EOLs.

Common invocations:

```powershell
# Format and normalize everything in-place
pwsh -File .\normalize.ps1

# Dry-run check (exit code 2 if changes needed; useful in CI)
pwsh -File .\normalize.ps1 -DryRun

# Check only staged files (for pre-commit hooks)
pwsh -File .\normalize.ps1 -Staged

# Parallel workers with log output
pwsh -File .\normalize.ps1 -ThrottleLimit 4 -LogPath artifacts/normalize.log
```

Include/exclude rules live in `normalize_config.json`, which has separate sections for `clangformat` and `textfiles`.

## C++ Standard and Compiler Settings

Settings are defined in `src/vcxproj/sal_base.props` and individual `.vcxproj` files.

| Flag / Setting | Value | Notes |
|---|---|---|
| C++ standard | `/std:c++latest` (C++20) | Set via `<LanguageStandard>stdcpplatest</LanguageStandard>` in vcxproj files. A few auxiliary tools (setup, tserver, translator) use C++17 instead. |
| Unsigned char | `/J` | Makes plain `char` unsigned by default. Applied globally in `sal_base.props`. |
| Multiprocessor compilation | `/MP` | Parallel compilation of translation units. Applied globally in `sal_base.props`. |
| Warning level | Level 3 (`/W3`) | Set in `sal_base.props`. Additionally, warning C4706 (assignment in conditional) is promoted to level 3 in `precomp.h`. |
| Precompiled header | `precomp.h` | Every project uses `<PrecompiledHeader>Use</PrecompiledHeader>` with `precomp.h`. |
| Target platform | Windows 7+ | `WINVER=0x0601`, `_WIN32_WINNT=0x0601`, `_WIN32_IE=0x0800` |
| CRT warnings | Suppressed | `_CRT_SECURE_NO_WARNINGS`, `_SCL_SECURE_NO_WARNINGS` with secure overload names enabled |

## Comment Language Policy

The codebase has a Czech heritage. Many source files contain comments originally written in Czech.

- **Legacy Czech comments** are acceptable and should not be gratuitously changed. Files that have been machine-translated carry the marker `// CommentsTranslationProject: TRANSLATED` near the top.
- **New comments** should be written in **English**.
- Do not spend effort manually translating old Czech comments; AI-assisted translation has already covered the most important files.

## Naming Conventions

The codebase follows traditional Win32/MFC-era C++ conventions:

| Element | Convention | Examples |
|---|---|---|
| Classes | `C` prefix + PascalCase | `CDiskCache`, `CDeleteManager`, `CCfgPageGeneral` |
| Template classes | `T` prefix + PascalCase | `TDirectArray`, `TIndirectArray` |
| Free functions | PascalCase | `InitializeDiskCache()`, `ValidatePathIsNotEmpty()` |
| Member variables | camelCase or Hungarian notation | `hParent`, `hDC`, `pwHue`, `clrRGB` |
| Local variables | camelCase | `errorCode`, `maxTextWidth` |
| Boolean parameters | descriptive camelCase | `canBlock`, `onlyAdd`, `selectedDirectory` |
| Win32 handles | Hungarian `h` prefix | `hIcon`, `hParent`, `hTemplateFile` |
| Pointers | Hungarian `p`/`lp` prefix | `lpTargetHandle`, `pluginFS` |
| String pointers | `lpsz`/`sz` prefix when following Win32 style | mixed with plain names |
| Constants / macros | `UPPER_SNAKE_CASE` | `SAFE_ALLOC`, `INSIDE_SALAMANDER` |
| BOOL parameters | Uppercase TRUE/FALSE (Win32 BOOL) | `BOOL getGrayIcons = FALSE` |

The style is not rigidly enforced beyond what clang-format handles. Follow the conventions visible in the file you are editing.

## Precompiled Header Convention

Every project includes a `precomp.h` file used as the precompiled header (configured via `sal_base.props`). Every `.cpp` file in a project must `#include "precomp.h"` as its first include.

A typical `precomp.h` (e.g., the main Salamander shell) includes:

1. **Windows SDK headers** -- `<windows.h>`, `<tchar.h>`, `<shlobj.h>`, `<commctrl.h>`, `<winioctl.h>`, etc.
2. **C runtime headers** -- `<stdio.h>`, `<math.h>`, `<limits.h>`, `<crtdbg.h>`, `<process.h>`
3. **Debug support** -- a `new` operator override in debug builds that records `__FILE__` and `__LINE__` for memory leak tracking.
4. **Common project headers** -- `trace.h`, `messages.h`, `handles.h`, `heap.h`, `array.h`, `winlib.h`, `multimon.h`, `sheets.h`, and other shared infrastructure.

Plugin projects have their own `precomp.h` that typically includes the plugin SDK headers (`spl_com.h`, `spl_base.h`, etc.) plus whichever common headers the plugin needs.
