# Research: Help Menu Simplification — Code Touch Points

Forensic mapping of every help affordance in the main application (and the Find
window), with exact files/lines, so the implementation is a precise, minimal
edit set. All line numbers are as of branch base (commit `f7606c3`).

## 1. Menus are template-driven (no `.rc` MENU resources)

Menus are built at runtime from `MENU_TEMPLATE_ITEM` arrays in `src/menu4.cpp`,
not from classic `MENU`/`MENUITEM` resources. Confirmed: no `MENUITEM ... Help`
exists in any `.rc`. Item text comes from string IDs in `src/lang/texts.rc2`.

`MNTT_PB` = popup begin, `MNTT_PE` = popup end/terminator, `MNTT_IT` = item,
`MNTT_SP` = separator.

### 1a. Main window Help menu — `src/menu4.cpp` (~lines 248–263)

```
{MNTT_PB, IDS_MENU_HELP,              ..., CML_HELP, ...},           // "Help" popup
{MNTT_IT, IDS_MENU_HELP_CONTENTS,     ..., CM_HELP_CONTENTS, IDX_TB_HELP, ...},
{MNTT_IT, IDS_MENU_HELP_INDEX,        ..., CM_HELP_INDEX, ...},
{MNTT_IT, IDS_MENU_HELP_SEARCH,       ..., CM_HELP_SEARCH, ...},
{MNTT_IT, IDS_MENU_HELP_KEYBOARD,     ..., CM_HELP_KEYBOARD, ...},
{MNTT_IT, IDS_MENU_HELP_CONTEXT,      ..., CM_HELP_CONTEXT, IDX_TB_CONTEXTHELP, ...},
// (commented) IDS_MENU_HELP_TIP ...
{MNTT_SP, ...},
{MNTT_IT, IDS_MENU_HELP_FORUM,        ..., CM_FORUM, ...},
{MNTT_IT, IDS_MENU_HELP_TASKLIST,     ..., CM_TASKLIST, ...},
{MNTT_SP, ...},
{MNTT_PB, IDS_MENU_HELP_ABOUTPLUGINS, ..., CML_HELP_ABOUTPLUGINS, ...},  // submenu
{MNTT_PE},
{MNTT_IT, IDS_MENU_HELP_ABOUT,        ..., CM_HELP_ABOUT, ...},          // KEEP
{MNTT_PE},                                                              // KEEP (Help popup end)
{MNTT_PE},  // terminator
```

**Edit**: Keep the `IDS_MENU_HELP` popup line, the `IDS_MENU_HELP_ABOUT` item,
and the two closing `MNTT_PE` lines. Delete everything between (Contents … About
Plugin submenu, including the two separators and the commented Tip line).

### 1b. Find files window Help menu — `src/menu4.cpp` (~lines 336–341)

```
// Help
{MNTT_PB, IDS_FFMENU_HELP,        ..., CML_FIND_HELP, ...},
{MNTT_IT, IDS_MENU_HELP_CONTENTS, ..., CM_HELP_CONTENTS, IDX_TB_HELP, ...},
{MNTT_IT, IDS_MENU_HELP_INDEX,    ..., CM_HELP_INDEX, ...},
{MNTT_IT, IDS_MENU_HELP_SEARCH,   ..., CM_HELP_SEARCH, ...},
{MNTT_PE},
{MNTT_PE},  // terminator
```

**Edit**: Delete the whole Help popup (the `MNTT_PB IDS_FFMENU_HELP` line through
its `MNTT_PE`). The preceding popup's `MNTT_PE` and the final terminator remain,
leaving a valid Find menu with no Help.

## 2. Toolbars — `src/toolbar4.cpp`

### 2a. Master button table is index-coupled (DO NOT reorder)

`#define TBBE_HELP_CONTENTS 52`, `#define TBBE_HELP_CONTEXT 53` (lines ~95–96)
index into `ToolBarButtons[]` (lines ~204–205):

```
/*TBBE_HELP_CONTENTS*/ {NIB1(IDX_TB_HELP),        0, IDS_TBTT_HELP,        CM_HELP_CONTENTS, ..., "HelpContents"},
/*TBBE_HELP_CONTEXT*/  {NIB1(IDX_TB_CONTEXTHELP), 0, IDS_TBTT_CONTEXTHELP, CM_HELP_CONTEXT,  ..., "WhatIsThis"},
```

**Keep these rows.** Removing them would shift every later `TBBE_*` index and
corrupt saved toolbar layouts. They stay as inert definitions.

### 2b. Default top toolbar layout — `TopToolBarButtons[]` (~lines 357–358)

```
NIB2(TBBE_HELP_CONTENTS)
    NIB2(TBBE_HELP_CONTEXT)
```

**Edit**: Delete these two entries so the default toolbar has no help buttons.
(The `TBBE_TERMINATOR` on the next line stays.)

### 2c. Bottom function-key bar — `BottomTBData[btbsCount][12]` (~lines 1214–1321)

Each row is one keyboard-modifier state; each of 12 columns is F1..F12. Three F1
slots reference help:

- `btbdNormal`, F1 (line ~1218): `{NIB3(TBBE_HELP_CONTENTS)}` → `{TBBE_TERMINATOR}`
- `btbdShift`,  F1 (line ~1263): `{NIB3(TBBE_HELP_CONTEXT)}`  → `{TBBE_TERMINATOR}`
- `btbsMenu`,   F1 (line ~1308): `{NIB3(TBBE_HELP_CONTENTS)}` → `{TBBE_TERMINATOR}`

**Edit**: Replace each with `{TBBE_TERMINATOR}` (the same empty marker already
used for other blank F-key slots, e.g. `btbdAlt` F8). Array shape (12 columns)
is preserved.

## 3. Accelerator — `src/salamand.rc` (line ~65)

```
VK_F1,  CM_HELP_CONTEXT,  VIRTKEY, SHIFT
```

**Edit**: Delete this line → Shift+F1 no longer arms "What is This?" mode.
(Alt+F1 = Change Drive and Ctrl+F1 = Drive Info are unrelated and stay. There is
no plain-F1 accelerator; plain F1 is handled via `WM_HELP`, see §4.)

## 4. Plain F1 → Contents is routed via `WM_HELP` (not an accelerator)

The main window has **no** `WM_HELP` handler and no plain-F1 accelerator. Plain
F1 reaches help through the focused child window:

### 4a. Panel — `src/filesbx1.cpp` (~lines 1276–1288)

```
case WM_HELP:
{
    if (MainWindow->HasLockedUI()) break;
    if (MainWindow->HelpMode == HELP_INACTIVE &&
        no Ctrl/Shift/Alt held)
    {
        PostMessage(MainWindow->HWindow, WM_COMMAND, CM_HELP_CONTENTS, 0); // plain F1
        return TRUE;
    }
    break;
}
```

**Edit**: Remove the plain-F1 → `CM_HELP_CONTENTS` posting so F1 in the panel is
a no-op. Simplest: drop the `PostMessage`/`return TRUE` (let the case `break`),
or delete the `WM_HELP` case entirely.

### 4b. Find window — `src/finddlg1.cpp` (~lines 3247–3251)

```
case WM_HELP:
{
    PostMessage(HWindow, WM_COMMAND, CM_HELP_CONTENTS, 0);
    return TRUE;
}
```

**Edit**: Neutralize (F1 in Find does nothing).

## 5. Command handlers stay (dormant) — `src/mainwnd3.cpp` (~lines 2535–2601)

`CM_HELP_CONTEXT` (2535), `CM_FORUM` (2548), `CM_HELP_CONTENTS/INDEX/SEARCH/
KEYBOARD` (2554–2593), `CM_HELP_ABOUT` (2596) — all retained. After the edits
above, only `CM_HELP_ABOUT` remains reachable (via the surviving menu item). The
`HelpMode` machinery (`mainwnd4.cpp`, `OnContextHelp`) is left intact but never
armed. This honors the minimal-change / don't-refactor-untouched-code principle.

## 6. Strings and dependent text

- Orphaned string IDs after the edits (`IDS_MENU_HELP_CONTENTS/INDEX/SEARCH/
  KEYBOARD/CONTEXT/FORUM/TASKLIST/ABOUTPLUGINS`, `IDS_FFMENU_HELP`) are left in
  `texts.rc2`. Unused string resources are harmless; removing them risks
  churning the language-module build and translations for zero user benefit.
- Still-used strings: `IDS_MENU_HELP` ("&Help" popup, kept), `IDS_MENU_HELP_ABOUT`
  (kept item), `IDS_TBTT_HELP` / `IDS_TBTT_CONTEXTHELP` (referenced by the
  retained master button rows).
- `IDS_BUGREPORTCNFRM_TEXT` references the removed "menu Help > Official Support
  Forum" path (stale). Cosmetic; default is to leave it (translation churn),
  reword only if trivially safe.

## 7. Out of scope (confirmed)

- Per-dialog context help: `WM_HELP` handlers in `common/winlib.cpp`,
  `common/sheets.cpp`, `msgbox.cpp`, and plugin dialogs route to
  `CSalamanderHelp::OnHelp` → `OpenHtmlHelp`. Pervasive and unrelated to the menu
  request; untouched.
- `OpenHtmlHelp` / HTML Help plumbing (`mainwnd3.cpp:147`) is unchanged.

## Build & verification method

- Build: `build.cmd` (Debug x64) validates compilation of `.rc` + `.cpp` edits.
  Release x64 (`build.cmd full release`) for the shipping config.
- Runtime check: launch the built binary, open **Help** (only About), click it
  (About dialog), press F1/Shift+F1 (no help), inspect toolbar and F-key bar.
