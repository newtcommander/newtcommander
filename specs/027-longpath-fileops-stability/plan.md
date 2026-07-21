# Implementation Plan: Long-Path & Unicode File-Operation Stability Revision

**Branch**: `027-longpath-fileops-stability` | **Date**: 2026-07-21 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/027-longpath-fileops-stability/spec.md`

## Summary

Close the long-path/Unicode file-operation class for good. Four work fronts,
all grounded in dump forensics and code audit (see [research.md](research.md)):

1. **Crashes (US2)** — two fresh dump-confirmed Release crashes still on
   main: `CDrivesList::CurrentPath[MAX_PATH]` overrun on Alt+F1
   (`drivelst.cpp:1069`) and `CExecuteExpData::Buffer[MAX_PATH]` fast-fail
   on F4 (`execute.cpp:794/878`). Fix by widening to `SAL_MAX_PATH_UTF8` +
   bounded copies (R1).
2. **Clipboard (US1)** — copy-out ≥MAX_PATH builds Salamander's own wide
   CF_HDROP IDataObject instead of the shell verb (R2); paste-in of foreign
   CF_HDROP with long source/target is taken over by the own engine
   (route B) instead of shell `InvokeCommand("paste")` (R3). Both changes
   length-gated: sub-260 behavior byte-for-byte unchanged.
3. **F5/F6/F7 + drag gaps (US3)** — stale `BuildScriptDir` source (~258) and
   target (248) gates raised to `SAL_MAX_PATH_UTF8`; ANSI
   `CreateDirectoryA`/security/encrypt call sites → W/Sal wrappers; F7 and
   `DoConvert` widened; drag-onto-subdir gate and `IsSimpleSelection`
   prefix fixed (R4, R5).
4. **Provable closure (US5) + speed (US4)** — execute feature 014's never-run
   8-pass audit to completion (`audit/A.md`–`H.md` → `INVENTORY.md`), fix
   every CRASH verdict, script a static exhaustion check (R7);
   canonicalization pre-scan skip + timing harness for the ≤10% bar (R6).

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI; internal shared libs
(`src/common/salpath`, `salunicode`, `salfileio`); OLE clipboard/drag
(IDataObject, CF_HDROP/DROPFILES); no new external dependencies
**Storage**: N/A (file system is the managed object)
**Testing**: `saltests` project (existing, 403 tests) + scripted
file-system-level verification + Debug/Release build gates; final GUI
walkthrough by user
**Target Platform**: Windows 11+ x64, ANSI build (UTF-8 internal `char*`,
W-APIs at OS boundary via `\\?\`)
**Project Type**: Desktop application (two-panel file manager)
**Performance Goals**: long-path copy ≤ 1.10 × ordinary-path copy for an
identical file set (SC-005); zero slowdown for sub-260 operations
**Constraints**: plugin ABI frozen (`SalPathAppend`/`PATH_MAX_PATH`/…
signatures untouched); `salextx64.dll` shared-memory IPC struct frozen;
sub-260 behavior byte-for-byte unchanged (Constitution II)
**Scale/Scope**: ~10 core source files with targeted fixes + 8-subsystem
audit over ~30 files / ~764 fixed-size buffers; 2 dump-confirmed crash
sites; 1 new data-object builder

## Constitution Check

*GATE: evaluated pre-Phase 0 and re-checked post-design — PASS.*

| Principle | Assessment |
|---|---|
| I. Build Reproducibility | No build-system changes; all fixes compile within existing projects. PASS |
| II. Backward Compatibility | Every behavior change is length-gated: sub-260 paths keep today's code paths byte-for-byte (shell verb copy, shell paste, existing gates). Plugin ABI and shellext IPC ABI explicitly frozen (R4 #2, R5). PASS |
| III. Incremental Modernization | Point fixes follow the established 004/011–014 pattern (`SAL_MAX_PATH_UTF8` widen / heap / bounded+message); no adjacent refactoring; audit documents rather than rewrites. PASS |
| IV. Windows Platform Commitment | Pure WinAPI (OLE, W-APIs); no new dependencies. PASS |
| V. Plugin Architecture Preservation | Plugin interfaces untouched; `PATH_MAX_PATH` constant (plugin ABI) not modified — only core call sites stop using it as a gate. PASS |
| VI. UI Consistency | No new dialogs; existing error messages reused (`IDS_TOOLONGNAME` family) with corrected gating. PASS |

## Project Structure

### Documentation (this feature)

```text
specs/027-longpath-fileops-stability/
├── plan.md              # This file
├── research.md          # Phase 0 — consolidated 4-agent investigation
├── data-model.md        # Phase 1 — entities (audit inventory, matrices, routes)
├── quickstart.md        # Phase 1 — build/verify/walkthrough instructions
├── contracts/
│   └── clipboard-dataobject.md  # own IDataObject / CF_HDROP contract
├── audit/               # execution of the 014 methodology to completion
│   ├── A.md … H.md      # 8 subsystem passes (written during implementation)
│   └── INVENTORY.md     # consolidated verdicts + external-limit list
└── tasks.md             # Phase 2 (/speckit-tasks)
```

### Source Code (repository root)

```text
src/
├── drivelst.h / drivelst.cpp     # D1: CDrivesList::CurrentPath widen (R1)
├── execute.cpp                   # D2: CExecuteExpData::Buffer widen + writers (R1)
├── fileswn9.cpp                  # paste routing: length-gated route-C takeover (R3)
├── shellsup.cpp                  # copy-out gate + own-dataobject hook; subdir gate :477 (R2, R5)
├── shellib.cpp / shellib.h       # own wide CF_HDROP builder; IsSimpleSelection prefix; IPC refusal (R2, R5)
├── fileswn6.cpp                  # BuildScriptDir gates :1587/:1655 (R4)
├── fileswn5.cpp                  # F7 CreateDir gate+buffers (R4)
├── salamdr5.cpp                  # ANSI CreateDirectory :968; SalGetFileSize2 (R4)
├── worker.cpp                    # security/encrypt W-APIs; DoConvert tmp name (R4)
├── common/salpath.cpp            # canonicalization pre-scan skip (R6)
└── (audit fixes)                 # any additional CRASH-verdict sites from audit/

src/saltests/ (existing test project)
└── new tests: DROPFILES round-trip, canonicalization skip, gate arithmetic
```

**Structure Decision**: single existing solution; all changes land in the
core app project + saltests. No new projects, no build changes.

## Phase 0: Research — COMPLETE

See [research.md](research.md). All unknowns resolved; no NEEDS
CLARIFICATION remain. Key inputs: 4 parallel investigation agents (clipboard
pipeline, F5 engine, 004–015 history, crash-dump forensics with
commit-matched PDB rebuilds).

## Phase 1: Design

### Fix design (per front)

**F1 — Crash fixes (R1)**: mechanical widen + bound, established pattern.
`CDrivesList` grows to ~96 KB — verified stack-safe (two non-reentrant UI
frames, 1 MB stack). All `CExecuteExpData::Buffer` writers in `execute.cpp`
audited and bounded in the same pass.

**F2 — Own clipboard data object (R2)**: new function in `shellib.cpp` —
build `HGLOBAL` DROPFILES with `fWide=1` and double-NUL-terminated wide
full-path list (via `SalU8ToWAlloc`, display form, no `\\?\`), wrap in a
minimal IDataObject also advertising `CFSTR_PREFERREDDROPEFFECT`; place via
`OleSetClipboard`; then `SetClipCutCopyInfo` marks it `SALCF_IDATAOBJECT`
exactly as today. Selected in `ShellAction(saCopy/CutToClipboard)` only when
panel path + any name ≥ MAX_PATH. Contract: `contracts/clipboard-dataobject.md`.

**F3 — Paste takeover (R3)**: in `ClipboardPaste`, route C entry point gains
a length gate: extract CF_HDROP paths (existing `ProcessClipboardData`
parser), if target dir or any source ≥ MAX_PATH → feed route B machinery
(`DoCopyMove` with records + preferred-drop-effect move/copy); else
unchanged shell `InvokeCommand("paste")`.

**F4 — Engine gates & ANSI sites (R4, R5)**: raise the two `BuildScriptDir`
gates; F7 widen; `SalCreateDirectory` at `salamdr5.cpp:968`;
`GetNamedSecurityInfoW`/`SetNamedSecurityInfoW`/`EncryptFileW`/`DecryptFileW`
wrappers with `\\?\` (verify EncryptFileW accepts prefix; degrade with clear
message if not); `DoConvert` widen; drag subdir gate; `IsSimpleSelection`
heap prefix; IPC write-site refusal messages.

**F5 — Audit completion (R7)**: 8 subsystem passes A–H executed by parallel
subagents during implementation using `_BRIEF.md` methodology; consolidated
`INVENTORY.md`; every CRASH verdict fixed; `tools`-style static check script
(PowerShell) re-verifying the inventory.

**F6 — Performance (R6)**: pre-scan in `SalPathToWExtAlloc` skips
`SalCanonicalizePathW` for clean paths; timing harness compares matrices.

### Data model

See [data-model.md](data-model.md) — audit inventory schema, transfer/
operation matrices, external-limit list.

### Contracts

`contracts/clipboard-dataobject.md` — the only externally visible interface
touched (clipboard data format + marker semantics).

## Phase 2: Task generation approach

Tasks will be grouped by user story (US2 crashes first — smallest, highest
severity; then US1 clipboard, US3 engine gates, US4 perf, US5 audit), each
independently buildable and testable; audit passes fan out in parallel;
build + saltests + static check gate the finish. Generated by
`/speckit-tasks`.

## Complexity Tracking

No constitution violations — table not needed.
