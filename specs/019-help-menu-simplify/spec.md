# Feature Specification: Simplify the Help Menu — Keep Only "About Open Salamander"

**Feature Branch**: `019-help-menu-simplify`
**Created**: 2026-07-18
**Status**: Draft
**Input**: User: The application menu bar's last top-level item is "Help". It
contains Contents, Index, Search, Official Support Forum and more. We will NOT
build the program help, so trim this menu to keep only one item — "About Open
Salamander".

## Problem Statement

Open Salamander's help content (the HTML Help / `.chm`-style manual) is not
being built for this fork. Every remaining help entry point therefore leads to
content that does not exist:

- the **Help** top-level menu (Contents, Index, Search, Keyboard, What is This?,
  Official Support Forum, Task List, About Plugin, About Open Salamander),
- the **Find files** window's own small **Help** menu (Contents, Index, Search),
- the **F1** key (opens help Contents) and **Shift+F1** ("What is This?"
  context-help mode),
- two **toolbar** buttons (Help Contents, What is This?) and the bottom
  function-key bar's **F1** slot.

The goal is a clean product with no dangling help affordances: the **Help** menu
keeps a single working item — **About Open Salamander** — and every other help
entry point is retired.

## Clarifications

### Session 2026-07-18

- Q: Besides the menu, should the other entry points into the (now non-existent)
  help be disabled — the F1 (Contents) key, Shift+F1 "What is This?", and the two
  toolbar Help buttons? → A: **Yes — full cleanup.** Remove the two toolbar Help
  buttons and disable F1/Shift+F1 help, so no element leads to non-existent help.
- Q: The Find files window has its own "Help" menu (Contents / Index / Search).
  What about it? → A: **Remove it too** (retire help consistently everywhere).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Help menu shows only "About Open Salamander" (Priority: P1)

Opening the top-level **Help** menu shows exactly one item — **About Open
Salamander** — which opens the About dialog. All other former items (Contents,
Index, Search, Keyboard, What is This?, Official Support Forum, Task List, the
About Plugin submenu) and their separators are gone.

**Why this priority**: This is the core, explicitly requested change.

**Independent Test**: Start the app, open the **Help** menu; verify a single
item **About Open Salamander** that opens the About dialog.

**Acceptance Scenarios**:

1. **Given** the app is running, **When** the user opens the **Help** menu,
   **Then** it contains exactly one enabled item, **About Open Salamander**, and
   no separators or submenus.
2. **Given** the **Help** menu is open, **When** the user clicks **About Open
   Salamander**, **Then** the About dialog opens (unchanged behavior).
3. **Given** the menu bar, **When** the user inspects the last top-level entry,
   **Then** it is still labeled **Help** (the menu itself is retained).

---

### User Story 2 - No dangling help entry points (Priority: P1)

No key, toolbar button, or secondary menu invokes the non-existent help.

**Why this priority**: The user explicitly chose the full cleanup so nothing
routes to missing help content.

**Acceptance Scenarios**:

1. **Given** the panel has focus, **When** the user presses **F1**, **Then**
   nothing happens (help Contents is NOT opened).
2. **Given** the app is running, **When** the user presses **Shift+F1**, **Then**
   "What is This?" context-help mode is NOT entered.
3. **Given** the default toolbar, **When** it is displayed, **Then** the **Help
   Contents** and **What is This?** buttons are absent.
4. **Given** the bottom function-key bar, **When** it is displayed, **Then** the
   **F1** slot is empty (no "Help" label/action) in every modifier state.
5. **Given** the **Find files** window, **When** the user opens its menu bar,
   **Then** there is no **Help** menu, and **F1** in that window does nothing.

---

### Edge Cases

- **Existing saved toolbar configuration**: A user whose Registry-saved toolbar
  layout still contains the Help buttons may keep them until they reset the
  toolbar. Default (fresh) toolbars have none. Accepted (documented) — the master
  toolbar-button table is retained to keep button indices stable; only the
  default layout drops the buttons.
- **`CM_HELP_*` command handlers remain in code** but become unreachable from the
  UI. Retained deliberately (minimal-change principle); they are harmless dead
  paths.
- **Bug-report confirmation text** (`IDS_BUGREPORTCNFRM_TEXT`) currently reads
  "please use menu Help > Official Support Forum." That menu path no longer
  exists; the stale reference is noted (see Assumptions) — a low-risk cosmetic
  string, addressed only if it can be reworded without translation churn.
- **Per-dialog context help** (F1/`?` inside individual dialogs, property sheets,
  and message boxes) is a separate, pervasive mechanism and is OUT OF SCOPE.

## Requirements *(mandatory)*

- **FR-001**: The top-level **Help** menu MUST contain exactly one item —
  **About Open Salamander** (`CM_HELP_ABOUT`) — which MUST open the About dialog
  with unchanged behavior. The **Help** popup itself MUST be retained.
- **FR-002**: The following MUST be removed from the **Help** menu: Contents,
  Index, Search, Keyboard, What is This?, the separators, Official Support Forum,
  Task List, and the About Plugin submenu.
- **FR-003**: The **Find files** window MUST NOT present a **Help** menu.
- **FR-004**: Pressing **F1** in the main panel MUST NOT open help. Pressing
  **F1** in the **Find files** window MUST NOT open help.
- **FR-005**: Pressing **Shift+F1** MUST NOT enter "What is This?" context-help
  mode (its accelerator is removed).
- **FR-006**: The default toolbar layout MUST NOT include the **Help Contents**
  or **What is This?** buttons.
- **FR-007**: The bottom function-key bar's **F1** slot MUST be empty in all
  modifier states (Normal, Shift, Menu) that currently show a help action.
- **FR-008**: No behavior of unrelated menus, keys, toolbar buttons, or the
  About dialog may regress (Backward Compatibility).
- **FR-009**: Debug and Release x64 builds MUST compile cleanly.

## Success Criteria *(mandatory)*

- **SC-001**: The **Help** menu shows a single item, **About Open Salamander**,
  that opens the About dialog; all other former items are absent.
- **SC-002**: F1 (panel and Find window) and Shift+F1 no longer open or arm any
  help; the default toolbar and the bottom F-key bar expose no help affordance.
- **SC-003**: The **Find files** window has no **Help** menu.
- **SC-004**: No unrelated command, accelerator, or dialog changes behavior.
- **SC-005**: Debug x64 (build verification) and Release x64 build succeed.

## Assumptions

- The help manual is intentionally not built; retiring its entry points is the
  desired end state (not a temporary disable).
- Scope is the main application window's help affordances plus the Find window's
  Help menu and F1, and the toolbar/function-key help buttons. Per-dialog
  context help (`WM_HELP` → `CSalamanderHelp::OnHelp` in dialogs/sheets/message
  boxes) is left untouched.
- The master toolbar-button definition table (`ToolBarButtons[]`, index-coupled
  to the `TBBE_*` enum) and the `CM_HELP_*` command handlers are retained to
  keep the change minimal and index-stable; only default layouts, menus,
  accelerators, and F1/WM_HELP routing are edited.
- `IDS_BUGREPORTCNFRM_TEXT`'s reference to "menu Help > Official Support Forum"
  is a known minor stale string; whether to reword it is decided during
  implementation (default: leave, to avoid translation churn).
- English is the build/verification language (`texts.rc2` → `english.slg`).
- Verification = clean Debug (and Release) x64 build plus a controlled launch to
  confirm the Help menu shows only About and F1/Shift+F1/toolbar expose no help.
