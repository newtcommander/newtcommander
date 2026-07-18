# Tasks: Simplify the Help Menu — Keep Only "About Open Salamander"

**Branch**: `019-help-menu-simplify` | **Spec**: [spec.md](./spec.md) | **Plan**: [plan.md](./plan.md)

Ordered, mostly independent edits. `[P]` = parallelizable with siblings (touches a
distinct file/region). Line numbers per [research.md](./research.md) (base `f7606c3`).

## Phase 1 — Menus (US1, US2)

- [x] **T001** Main Help menu — `src/menu4.cpp` (~248–263): keep the
  `IDS_MENU_HELP` popup, the `IDS_MENU_HELP_ABOUT` (`CM_HELP_ABOUT`) item, and the
  closing `MNTT_PE`s; delete Contents, Index, Search, Keyboard, What-is-This,
  both separators, Forum, Task List, the commented Tip line, and the About-Plugin
  submenu. (FR-001, FR-002)
- [x] **T002** Find window Help menu — `src/menu4.cpp` (~336–341): delete the
  entire `IDS_FFMENU_HELP` popup (begin → its `MNTT_PE`). (FR-003)

## Phase 2 — Toolbars (US2)

- [x] **T003 [P]** Default top toolbar — `src/toolbar4.cpp` (~357–358): remove
  `NIB2(TBBE_HELP_CONTENTS)` and `NIB2(TBBE_HELP_CONTEXT)` from
  `TopToolBarButtons[]`. Do NOT touch the master `ToolBarButtons[]` rows
  (~204–205) or the `TBBE_*` defines. (FR-006)
- [x] **T004 [P]** Bottom F-key bar — `src/toolbar4.cpp` (~1218, 1263, 1308): set
  the three help F1 slots (`btbdNormal`, `btbdShift`, `btbsMenu`) to
  `{TBBE_TERMINATOR}`. (FR-007)

## Phase 3 — Keyboard / F1 (US2)

- [x] **T005 [P]** Accelerator — `src/salamand.rc` (~65): delete
  `VK_F1, CM_HELP_CONTEXT, VIRTKEY, SHIFT`. Leave Alt+F1 / Ctrl+F1. (FR-005)
- [x] **T006 [P]** Panel plain F1 — `src/filesbx1.cpp` (~1276–1288): remove the
  plain-F1 → `CM_HELP_CONTENTS` posting so `WM_HELP` is a no-op in the panel.
  (FR-004)
- [x] **T007 [P]** Find window F1 — `src/finddlg1.cpp` (~3247–3251): neutralize
  the `WM_HELP` → `CM_HELP_CONTENTS` posting. (FR-004)

## Phase 4 — Build & Verify

- [x] **T008** Build Debug x64 (`build.cmd`); resolve any compile issues.
  (FR-009, SC-005)
- [~] **T009** Build Release x64 (`build.cmd release`). Compile PASSED (all
  objects incl. toolbar4/menu4/filesbx1/finddlg1; 0 warnings on our files). Final
  link BLOCKED by a running `Release_x64\salamand.exe` (`LNK1104`); re-run once the
  running instance is closed. (FR-009, SC-005)
- [ ] **T010** Runtime check: launch; open **Help** → only **About Open
  Salamander**, which opens the About dialog; press F1 (panel) and Shift+F1 → no
  help; confirm default toolbar and bottom F-key bar show no help affordance;
  open the Find window → no Help menu, F1 does nothing. (SC-001…SC-004)

## Notes / Out of scope

- `CM_HELP_*` handlers (`mainwnd3.cpp`) and `HelpMode` machinery stay (dormant).
- Orphaned string IDs in `texts.rc2` are left in place (harmless).
- `IDS_BUGREPORTCNFRM_TEXT` stale "menu Help > Forum" wording: leave unless a
  trivially safe reword (translation-churn caution).
- Per-dialog context help (`WM_HELP` → `OnHelp` in dialogs/sheets/msgbox) is not
  touched.
