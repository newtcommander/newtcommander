# Implementation Plan: Simplify the Help Menu — Keep Only "About Open Salamander"

**Branch**: `019-help-menu-simplify` | **Date**: 2026-07-18 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `specs/019-help-menu-simplify/spec.md`

## Summary

Trim the main application's **Help** menu to a single working item — **About
Open Salamander** — and retire every other help entry point (Find window's Help
menu, F1/Shift+F1, two toolbar buttons, and the bottom F-key bar's F1 slot),
because the program help manual is not being built. The change is a small,
surgical edit set across four source files plus one resource file; the master
toolbar-button table and the `CM_HELP_*` command handlers are retained inert to
keep the change minimal and index-stable (see [research.md](./research.md)).

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI; no new dependencies
**Storage**: N/A (Registry-persisted toolbar layout is read-only for this change)
**Testing**: Build verification (Debug + Release x64) + manual runtime check
**Target Platform**: Windows 11+
**Project Type**: Single desktop application (`src/`)
**Performance Goals**: N/A (UI wiring only)
**Constraints**: Backward compatibility; minimal, index-stable edits
**Scale/Scope**: 5 files, ~7 small edit sites; no new files

## Constitution Check

*GATE: Must pass before implementation. Re-check after.*

- **I. Build Reproducibility** — ✅ No build-system change; standard `build.cmd`.
- **II. Backward Compatibility** — ✅ Deliberate, user-requested removal of help
  UI (help content is discontinued). No plugin API change; no binary-compat
  impact. Saved toolbar layouts stay valid (master button table unchanged).
- **III. Incremental Modernization** — ✅ Small, reviewable, revertible edits;
  no refactoring of adjacent code. `CM_HELP_*` handlers and `HelpMode` machinery
  left intact but unreached.
- **IV. Windows Platform Commitment** — ✅ Pure WinAPI, no new abstractions.
- **V. Plugin Architecture Preservation** — ✅ Not applicable (core UI only); the
  About-Plugin submenu is removed from the Help menu per the explicit request,
  but plugin registration/About plumbing is untouched.
- **VI. UI Consistency** — ✅ No new dialogs/controls; no process-wide visual
  behavior change. Removing menu items and buttons does not alter control styling.

**Result**: PASS. No violations; Complexity Tracking not required.

## Project Structure

### Documentation (this feature)

```text
specs/019-help-menu-simplify/
├── spec.md        # Feature spec (with Clarifications)
├── research.md    # Exact code touch points (Phase 0)
├── plan.md        # This file
└── tasks.md       # Ordered task list (Phase 2)
```

### Source Code (repository root)

```text
src/
├── menu4.cpp       # [edit] main Help menu (keep About); remove Find window Help menu
├── toolbar4.cpp    # [edit] drop 2 help buttons from default top toolbar;
│                   #        blank 3 F1 slots in the bottom F-key bar
├── salamand.rc     # [edit] remove Shift+F1 (CM_HELP_CONTEXT) accelerator
├── filesbx1.cpp    # [edit] neutralize plain-F1 WM_HELP → CM_HELP_CONTENTS (panel)
├── finddlg1.cpp    # [edit] neutralize F1 WM_HELP → CM_HELP_CONTENTS (Find window)
├── mainwnd3.cpp    # [no change] CM_HELP_* handlers retained (dormant)
└── lang/texts.rc2  # [no change] orphaned strings left in place
```

**Structure Decision**: Single desktop app; edits are confined to the main-app
UI wiring files listed above. No new modules, no new resources.

## Implementation Approach (ordered)

1. **Main Help menu** (`menu4.cpp` ~248–263): keep the `IDS_MENU_HELP` popup,
   the `IDS_MENU_HELP_ABOUT`/`CM_HELP_ABOUT` item, and the closing `MNTT_PE`s;
   delete all intermediate items, separators, and the About-Plugin submenu.
2. **Find window Help menu** (`menu4.cpp` ~336–341): delete the entire
   `IDS_FFMENU_HELP` popup (begin → its `MNTT_PE`).
3. **Default top toolbar** (`toolbar4.cpp` ~357–358): delete the
   `TBBE_HELP_CONTENTS` and `TBBE_HELP_CONTEXT` entries from `TopToolBarButtons[]`.
4. **Bottom F-key bar** (`toolbar4.cpp` ~1218/1263/1308): set the three help F1
   slots to `{TBBE_TERMINATOR}`.
5. **Accelerator** (`salamand.rc` ~65): delete the `VK_F1 … CM_HELP_CONTEXT …
   SHIFT` line.
6. **Plain F1 routing** (`filesbx1.cpp` ~1276; `finddlg1.cpp` ~3247): remove the
   `CM_HELP_CONTENTS` posting so F1 is a no-op in the panel and the Find window.
7. **Build** Debug x64 (`build.cmd`); fix any compile issues; then Release x64.
8. **Runtime verify**: launch, open Help (only About → About dialog opens),
   press F1/Shift+F1 (no help), confirm toolbar/F-key bar expose no help.

## Risks & Mitigations

- **Index coupling of `TBBE_*`** → Mitigation: never delete master
  `ToolBarButtons[]` rows; only edit default-layout and F-key-bar references.
- **Menu-template array shape** (`MNTT_PB`/`MNTT_PE` balance) → Mitigation:
  preserve popup begin/end pairing; verify balanced terminators after edits.
- **Leaving strings/handlers orphaned** → Accepted; harmless dead resources/code,
  documented in spec Assumptions.

## Complexity Tracking

No constitution violations — table not required.
