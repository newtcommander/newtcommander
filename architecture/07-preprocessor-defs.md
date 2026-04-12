# Preprocessor Definitions Reference

This document catalogs every preprocessor definition and preprocessor-like compiler flag used across the Salamander build system, extracted from the `.props` files in `src/vcxproj/` and `src/plugins/shared/vcxproj/`.

---

## 1. Salamander Base (all configurations)

**Source:** `src/vcxproj/sal_base.props`

### Compiler Preprocessor Definitions

| Definition | Value | Purpose |
|---|---|---|
| `SAFE_ALLOC` | *(none)* | Enables safe memory allocation wrappers/checks |
| `_MT` | *(none)* | Indicates multithreaded runtime; required for thread-safe CRT |
| `WIN32` | *(none)* | Standard Windows platform identifier |
| `_WINDOWS` | *(none)* | Targets the Windows subsystem (GUI application) |
| `INSIDE_SALAMANDER` | *(none)* | Marks code as part of the core Salamander executable (as opposed to plugins) |
| `WINVER` | `0x0601` | Minimum Windows version target: Windows 7 |
| `_WIN32_WINNT` | `0x0601` | Minimum Windows NT version target: Windows 7 |
| `_WIN32_IE` | `0x0800` | Minimum Internet Explorer feature level: IE 8.0 (for common controls) |
| `_CRT_SECURE_NO_WARNINGS` | *(none)* | Suppresses MSVC deprecation warnings for classic CRT functions (e.g., `strcpy`) |
| `_SCL_SECURE_NO_WARNINGS` | *(none)* | Suppresses MSVC deprecation warnings for classic STL algorithms |
| `_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES` | *(none)* | Enables automatic secure overloads of standard CRT names (e.g., `strcpy` to `strcpy_s`) |
| `_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT` | *(none)* | Enables count-based secure overloads of standard CRT names |

### Resource Compiler Preprocessor Definitions

| Definition | Value | Purpose |
|---|---|---|
| `WINVER` | `0x0601` | Minimum Windows version for resource-level conditionals |

### Compiler Flags Acting as Preprocessor Settings

| Flag | Effect |
|---|---|
| `/MP` | Enable parallel compilation (multi-process build) |
| `/J` | Treat `char` as `unsigned char` by default; changes the semantics of all plain `char` types project-wide |

---

## 2. Salamander Debug Only

**Source:** `src/vcxproj/sal_debug.props`

### Compiler Preprocessor Definitions

| Definition | Value | Purpose |
|---|---|---|
| `_DEBUG` | *(none)* | Standard MSVC debug mode indicator; enables CRT debug heap and assertions |
| `__DEBUG_WINLIB` | *(none)* | Enables debug instrumentation in the internal WinLib UI library |
| `TRACE_ENABLE` | *(none)* | Activates the trace/logging subsystem for diagnostic output |
| `HANDLES_ENABLE` | *(none)* | Activates handle-tracking subsystem to detect leaks |
| `MESSAGES_DEBUG` | *(none)* | Enables debug-level message logging (window messages, IPC, etc.) |
| `MULTITHREADED_TRACE_ENABLE` | *(none)* | Enables trace output from multiple threads simultaneously |
| `MULTITHREADED_MESSAGES_ENABLE` | *(none)* | Enables message logging from multiple threads simultaneously |
| `MULTITHREADED_HANDLES_ENABLE` | *(none)* | Enables handle tracking from multiple threads simultaneously |
| `_CRTDBG_MAP_ALLOC` | *(none)* | Maps `malloc`/`free` to CRT debug equivalents for memory leak detection |
| `_ALLOW_RTCc_IN_STL` | *(none)* | Permits Runtime Check (smaller type check) in STL headers without errors |

### Resource Compiler Preprocessor Definitions

| Definition | Value | Purpose |
|---|---|---|
| `_DEBUG` | *(none)* | Enables debug-conditional resources (e.g., debug version strings) |

### Compiler Flags Acting as Preprocessor Settings

| Flag / Setting | Effect |
|---|---|
| `Optimization: Disabled` | No optimization; preserves debuggability |
| `RuntimeLibrary: MultiThreadedDebugDLL` (`/MDd`) | Links against the debug multithreaded DLL CRT |
| `SmallerTypeCheck: true` (`/RTCc`) | Runtime check for data loss on assignment to smaller types |

---

## 3. Salamander Release Only

**Source:** `src/vcxproj/sal_release.props`

### Compiler Preprocessor Definitions

| Definition | Value | Purpose |
|---|---|---|
| `NDEBUG` | *(none)* | Standard release indicator; disables `assert()` macro |
| `MESSAGES_DISABLE` | *(none)* | Completely disables the message logging subsystem for performance |

### Resource Compiler Preprocessor Definitions

| Definition | Value | Purpose |
|---|---|---|
| `NDEBUG` | *(none)* | Marks resources as release build |

### Compiler Flags Acting as Preprocessor Settings

| Flag / Setting | Effect |
|---|---|
| `Optimization: MaxSpeed` (`/O2`) | Full speed optimization |
| `IntrinsicFunctions: true` (`/Oi`) | Replace some function calls with intrinsic (inline) equivalents |
| `FunctionLevelLinking: true` (`/Gy`) | Places each function in its own COMDAT section for dead-code elimination |
| `RuntimeLibrary: MultiThreadedDLL` (`/MD`) | Links against the release multithreaded DLL CRT |
| `WholeProgramOptimization: true` (`/GL`) | Enables link-time code generation for cross-module optimization |

---

## 4. Plugin Base (all configurations)

**Source:** `src/plugins/shared/vcxproj/plugin_base.props`

### Compiler Preprocessor Definitions

| Definition | Value | Purpose |
|---|---|---|
| `_MT` | *(none)* | Indicates multithreaded runtime |
| `WIN32` | *(none)* | Standard Windows platform identifier |
| `_WINDOWS` | *(none)* | Targets the Windows subsystem |
| `_USRDLL` | *(none)* | Indicates this is a user DLL; affects DLL-related CRT initialization |

### Compiler Flags Acting as Preprocessor Settings

| Flag | Effect |
|---|---|
| `/MP` | Enable parallel compilation |
| `/J` | Treat `char` as `unsigned char` by default |

### Notable Differences from Salamander Base

Plugins do **not** inherit the following Salamander-base definitions: `SAFE_ALLOC`, `INSIDE_SALAMANDER`, `WINVER`, `_WIN32_WINNT`, `_WIN32_IE`, `_CRT_SECURE_NO_WARNINGS`, `_SCL_SECURE_NO_WARNINGS`, `_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES`, `_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT`. Plugins add `_USRDLL` to denote DLL builds (output extension: `.spl`).

---

## 5. Plugin Debug Only

**Source:** `src/plugins/shared/vcxproj/plugin_debug.props`

### Compiler Preprocessor Definitions

| Definition | Value | Purpose |
|---|---|---|
| `_DEBUG` | *(none)* | Standard MSVC debug mode indicator |
| `TRACE_ENABLE` | *(none)* | Activates the trace/logging subsystem |
| `MHANDLES_ENABLE` | *(none)* | Activates module-level handle tracking for plugins (note: `MHANDLES_ENABLE`, not `HANDLES_ENABLE` as in core Salamander) |
| `_CRTDBG_MAP_ALLOC` | *(none)* | Maps allocations to CRT debug heap for leak detection |
| `_ALLOW_RTCc_IN_STL` | *(none)* | Permits Runtime Check (smaller type check) in STL headers |

### Resource Compiler Preprocessor Definitions

| Definition | Value | Purpose |
|---|---|---|
| `_DEBUG` | *(none)* | Enables debug-conditional resources |

### Compiler Flags Acting as Preprocessor Settings

| Flag / Setting | Effect |
|---|---|
| `Optimization: Disabled` | No optimization |
| `RuntimeLibrary: MultiThreadedDebugDLL` (`/MDd`) | Debug multithreaded DLL CRT |
| `SmallerTypeCheck: true` (`/RTCc`) | Runtime check for data loss on smaller type assignment |

### Notable Differences from Salamander Debug

Plugin debug builds omit: `__DEBUG_WINLIB`, `MESSAGES_DEBUG`, `MULTITHREADED_TRACE_ENABLE`, `MULTITHREADED_MESSAGES_ENABLE`, `MULTITHREADED_HANDLES_ENABLE`. They use `MHANDLES_ENABLE` instead of `HANDLES_ENABLE`.

---

## 6. Plugin Release Only

**Source:** `src/plugins/shared/vcxproj/plugin_release.props`

### Compiler Preprocessor Definitions

| Definition | Value | Purpose |
|---|---|---|
| `NDEBUG` | *(none)* | Standard release indicator; disables `assert()` |

### Resource Compiler Preprocessor Definitions

| Definition | Value | Purpose |
|---|---|---|
| `NDEBUG` | *(none)* | Marks resources as release build |

### Compiler Flags Acting as Preprocessor Settings

| Flag / Setting | Effect |
|---|---|
| `Optimization: MaxSpeed` (`/O2`) | Full speed optimization |
| `IntrinsicFunctions: true` (`/Oi`) | Intrinsic function replacement |
| `FunctionLevelLinking: true` (`/Gy`) | Per-function COMDAT sections |
| `RuntimeLibrary: MultiThreadedDLL` (`/MD`) | Release multithreaded DLL CRT |
| `WholeProgramOptimization: true` (`/GL`) | Link-time code generation |

### Notable Differences from Salamander Release

Plugin release builds omit `MESSAGES_DISABLE` (plugins do not define this).

---

## 7. Language Modules

**Source:** `src/vcxproj/lang_base.props`, `lang_debug.props`, `lang_release.props`

Language modules (`.slg` files) are resource-only DLLs with no entry point. They have no C/C++ compilation step, so preprocessor definitions apply only to the resource compiler (`rc.exe`).

### lang_base.props -- Resource Compiler Definitions

| Definition | Value | Purpose |
|---|---|---|
| `WINVER` | `0x0601` | Minimum Windows version for resource conditionals |
| `_LANG` | *(none)* | Marks the build as a language module |
| `_LANG_SALAMANDER` | *(none)* | Marks the language module as belonging to core Salamander (as opposed to a plugin language module) |

### lang_debug.props -- Resource Compiler Definitions

| Definition | Value | Purpose |
|---|---|---|
| `_DEBUG` | *(none)* | Debug-conditional resources |

### lang_release.props -- Resource Compiler Definitions

| Definition | Value | Purpose |
|---|---|---|
| `NDEBUG` | *(none)* | Release-conditional resources |

---

## 8. Platform-Specific (x86 / x64)

**Source:** `src/plugins/shared/vcxproj/x86.props`, `x64.props`

### x86.props

| Setting | Value | Purpose |
|---|---|---|
| `ShortPlatform` (MSBuild macro) | `x86` | Used in output paths and base-address file selection |
| `EnableEnhancedInstructionSet` | `StreamingSIMDExtensions2` (`/arch:SSE2`) | Sets SSE2 as baseline instruction set for 32-bit builds |

No preprocessor definitions are added for x86.

### x64.props

| Setting | Value | Purpose |
|---|---|---|
| `ShortPlatform` (MSBuild macro) | `x64` | Used in output paths and base-address file selection |

#### Compiler Preprocessor Definitions

| Definition | Value | Purpose |
|---|---|---|
| `_WIN64` | *(none)* | Identifies a 64-bit Windows build; used for pointer-size conditionals and type selection |

#### Resource Compiler Preprocessor Definitions

| Definition | Value | Purpose |
|---|---|---|
| `_WIN64` | *(none)* | Identifies 64-bit build in resource scripts |

---

## Summary: Effective Definitions per Build Configuration

The tables below show the combined preprocessor definitions that a translation unit will see after all property sheets are merged (using `%(PreprocessorDefinitions)` inheritance).

### Salamander Core (Debug, x86)

```
SAFE_ALLOC, _MT, WIN32, _WINDOWS, INSIDE_SALAMANDER,
WINVER=0x0601, _WIN32_WINNT=0x0601, _WIN32_IE=0x0800,
_CRT_SECURE_NO_WARNINGS, _SCL_SECURE_NO_WARNINGS,
_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES,
_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT,
_DEBUG, __DEBUG_WINLIB, TRACE_ENABLE, HANDLES_ENABLE,
MESSAGES_DEBUG, MULTITHREADED_TRACE_ENABLE,
MULTITHREADED_MESSAGES_ENABLE, MULTITHREADED_HANDLES_ENABLE,
_CRTDBG_MAP_ALLOC, _ALLOW_RTCc_IN_STL
```

### Salamander Core (Debug, x64)

All of the above, plus: `_WIN64`

### Salamander Core (Release, x86)

```
SAFE_ALLOC, _MT, WIN32, _WINDOWS, INSIDE_SALAMANDER,
WINVER=0x0601, _WIN32_WINNT=0x0601, _WIN32_IE=0x0800,
_CRT_SECURE_NO_WARNINGS, _SCL_SECURE_NO_WARNINGS,
_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES,
_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT,
NDEBUG, MESSAGES_DISABLE
```

### Salamander Core (Release, x64)

All of the above, plus: `_WIN64`

### Plugin (Debug, x86)

```
_MT, WIN32, _WINDOWS, _USRDLL,
_DEBUG, TRACE_ENABLE, MHANDLES_ENABLE,
_CRTDBG_MAP_ALLOC, _ALLOW_RTCc_IN_STL
```

### Plugin (Debug, x64)

All of the above, plus: `_WIN64`

### Plugin (Release, x86)

```
_MT, WIN32, _WINDOWS, _USRDLL, NDEBUG
```

### Plugin (Release, x64)

All of the above, plus: `_WIN64`

### Language Module (Debug)

Resource compiler only: `WINVER=0x0601, _LANG, _LANG_SALAMANDER, _DEBUG`

### Language Module (Release)

Resource compiler only: `WINVER=0x0601, _LANG, _LANG_SALAMANDER, NDEBUG`
