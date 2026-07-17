# Surface Inventory: File Name / Path / Stored-String Display Audit (Feature 010)

**Protocol**: Manual walkthrough with the sample-name set (quickstart.md);
one row per display surface (data-model.md §3). Verdicts:
`candidate` → `verified-correct` | `defective` → `fixed` (re-verified) | `out-of-scope`.

**Summary** *(close-out, T028, 2026-07-17)*:
- Sample set: Czech diacritics (composed + decomposed), Cyrillic/Greek/CJK, emoji, plain ASCII
  (test tree: `%LOCALAPPDATA%\Temp\salamander-test\010\`)
- Build verified against: full clean rebuild Debug x64, 2026-07-17, `BUILD SUCCEEDED`,
  0 warnings in changed files (29 pre-existing legacy warnings in untouched plugin code);
  smoke start of `salamand.exe` OK
- Totals: **50 rows — 37 defective→fixed (code), 11 verified-correct, 2 out-of-scope**
- Interactive walkthrough (quickstart P1/P2/P3 + ASCII regression): **pending** —
  rows marked `fixed (code)` become `fixed (verified)` after the walkthrough
  (tasks T011, T016, T025, T026)
- Follow-up (out of this feature): drive-bar volume-label *acquisition* is ANSI
  end-to-end (`GetVolumeInformationA`, `src/drivelst.cpp`) — consistent today, no
  mojibake, but labels outside the active code page cannot render; widening it is
  a future feature (research R6/R9). FTP log-edit caret restore uses byte offsets
  (clamped by EM_SETSEL — cosmetic only).

## Area: Main-window chrome

| Id | Location | String source | API | Verdict | Resolution |
|----|----------|---------------|-----|---------|------------|
| A2-dirline-draw | `src/stswnd.cpp` `CStatusWindow::Paint` | Path | `ExtTextOutA` → `ExtTextOutW` via `TextW` cache | fixed (code) | wide mirror `TextW` + `DrawTextSeg`/`DrawEllipsis`; ANSI fallback kept |
| A2-dirline-measure | `src/stswnd.cpp` `BuildHotTrackItems` | Path | `GetTextExtentExPointW`, WCHAR `AlpDX` | fixed (code) | single wide measurement; byte→WCHAR offset remap post-pass |
| A2-dirline-ellipsis | `src/stswnd.cpp` `Paint` truncation math | Path | WCHAR-unit arithmetic | fixed (code) | `textLen` local follows wide/legacy mode; no dropped components |
| A2-dirline-hottrack | `src/stswnd.cpp` `GetItemText`, click/drag/clipboard | Path | `SalWToU8` round-trip, `CopyTextToClipboardW` | fixed (code) | FR-007: navigation/clipboard/drag carry the true path |
| A2-infoline | `src/stswnd.cpp` blBottom + `SetSubTexts` | Path + file info | wide measure/draw, byte→WCHAR subtext remap | fixed (code) | producers stay byte-based; boundary converts |
| A2-dragimage | `src/stswnd.cpp` `CreateDragImage` | Path | `DrawTextW`/`GetTextExtentPoint32W` | fixed (code) | drag image renders true glyphs |
| A2-sizetext | `src/stswnd.cpp` `SetSize`/`Paint` | localized units | `SizeW` mirror + `ExtTextOutW` | fixed (code) | non-ASCII units render correctly |
| A6-tooltips | `src/tooltip.cpp` `CToolTip` | Path | `TextW` mirror + `DrawTextW` | fixed (code) | covers dir-line, panel long-name, throbber/security tooltips |
| title-bar | `src/mainwnd1.cpp:1899-1904` | Path | `SetWindowTextW` | verified-correct (005 reference) | regression guard only |
| panel-list | `src/fileswn4.cpp` | Path | `SalU8ToW`+`ExtTextOutW` | verified-correct (004 reference) | regression guard only |
| panel-header | `src/filesbx2.cpp:230,270` | resource text | `TextOutA` | out-of-scope | localized captions, not names |
| drivebar-labels | `src/drivelst.cpp` | volume labels (ANSI acquisition) | ANSI end-to-end | out-of-scope (follow-up) | consistent today; widening = future feature (R6/R9) |

## Area: Pack family dialogs

| Id | Location | String source | API | Verdict | Resolution |
|----|----------|---------------|-----|---------|------------|
| P2-pack-combo | `src/dialogs3.cpp` `CPackDialog::Transfer` | Stored String | `SalComboAddStringU8` | fixed (code) | reported Alt+F5 defect |
| P2-pack-path | `src/dialogs3.cpp` (IDE_PATH combo) | Path | `SalComboAddStringU8` | fixed (code) | — |
| P2-unpack-combo | `src/dialogs3.cpp` `CUnpackDialog::Transfer` | Stored String + Path | `SalComboAddStringU8` | fixed (code) | Alt+F6 |
| B11-cfg-packers | `src/dialogsp.cpp` + `src/edtlbwnd.cpp` | Stored String | `CEditListBox` wide draw + `SalSet/GetWindowTextU8` edit round-trip | fixed (code) | systemic fix in the shared edit-list-box control |
| B11-cfg-unpackers | `src/dialogsp.cpp` | Stored String | dtto + `SalComboAddStringU8` type combos | fixed (code) | — |
| B11-cfg-archivers | `src/dialogsp.cpp` | Stored String | `SalListBoxAddStringU8`, `SalComboAddStringU8` | fixed (code) | incl. Archives Association page combos |
| cfg-reset-gate | `src/mainwnd2.cpp` | Stored String lifecycle | `THIS_CONFIG_VERSION` 104→105 gate | fixed (code) | ConfigVersion<105 ⇒ packer/unpacker/archiver/assoc sections rebuilt from defaults (clarified policy); preferred indexes not carried over |

## Area: Owner-draw menus (005 item A4)

| Id | Location | String source | API | Verdict | Resolution |
|----|----------|---------------|-----|---------|------------|
| A4-menu-measure | `src/menu1.cpp` `DecodeSubTextLenghtsAndWidths` | Path + resource | `DrawTextW(DT_CALCRECT)` per column | fixed (code) | columns split on '\t' first, converted separately |
| A4-menu-draw | `src/menu3.cpp` `CMenuPopup::DrawItem` (9 sites) | Path + resource | `DrawTextW` | fixed (code) | convert once per column per paint; ANSI fallback |
| A4-menubar | `src/menubar.cpp` `DrawItem` + `RefreshMinWidths` | resource | `DrawTextW` (draw + measure) | fixed (code) | measure/draw unit-consistent |
| A4-menu2 | `src/menu2.cpp` | — | no text rendering | verified-correct | layout/tracking only |

## Area: Toolbars (005 item A5)

| Id | Location | String source | API | Verdict | Resolution |
|----|----------|---------------|-----|---------|------------|
| A5-toolbar2 | `src/toolbar2.cpp` `Refresh` + `DrawItem` | Path (hot-path labels) | `DrawTextW` measure+draw | fixed (code) | measure-at-layout/draw-at-paint converted in lockstep |
| A5-toolbar3 | `src/toolbar3.cpp` Customize dialog listboxes | resource + item names | `DrawTextW` | fixed (code) | — |
| A5-toolbar4 | `src/toolbar4.cpp` `SetMaxItemWidths` | F1-F12 labels | `DrawTextW(DT_CALCRECT)` | fixed (code) | non-terminated 15-byte texts handled by explicit length |

## Area: Misc deferred (005 items B17 + label-edit + custom-draw)

| Id | Location | String source | API | Verdict | Resolution |
|----|----------|---------------|-----|---------|------------|
| B17-salamdr2 | `src/salamdr2.cpp` `ClearComboboxListbox` | Path | `SalGet/SetWindowTextU8` round-trip | fixed (code) | — |
| B17-salamdr3 | `src/salamdr3.cpp` timestamps listbox, `SetEditOrComboText`, browse read | Path | `SalListBoxAddStringU8`, `SalSetWindowTextU8`, `SalGetDlgItemTextU8` | fixed (code) | — |
| B17-codetbl | `src/codetbl.cpp:668` | table names (convert.cfg) | `SalInsertMenuItemU8` (new helper) | fixed (code) | legacy-encoded names take ANSI fallback |
| B17-shellsup | `src/shellsup.cpp` Paste/Paste Shortcuts/New menu items | resource (UTF-8) | `SalInsertMenuItemU8` | fixed (code) | panel background context menu |
| B17-dialogs6 | `src/dialogs6.cpp` icon-overlays listview | overlay names | `SalListViewSetItemTextU8` | fixed (code) | — |
| B17-convtbl | `src/dialogs3.cpp` conversion-tables listview | convert.cfg descriptions | `SalListViewSetItemTextU8` | fixed (code) | — |
| dialogs5-sites | `src/dialogs5.cpp` (23 sites) | plugin texts, paths, localized combos, task list | `Sal*U8` helpers | fixed (code) | plugins manager page, plugin keys dialog, drive-spec path, viewers combo, appearance combos, task list |
| dialogs4-labeledit | `src/dialogs4.cpp` views + hot-paths pages | user-entered names | wide read of label-edit control + `SalListViewSetItemTextU8` | fixed (code) | LVN_ENDLABELEDIT A-mangling bypassed |
| packac | `src/packac.cpp` | archiver names + found paths | `LVN_GETDISPINFOW` (Unicode format), wide columns, `SalSetDlgItemTextU8` | fixed (code) | — |
| editwnd-compact | `src/editwnd.cpp` `CInnerText` | Path | `PathCompactPathW`+`DrawTextW` | fixed (code) | finddlg1 pattern |

## Area: Plugins (18 enabled per plugins.cfg)

| Id | Plugin | Location | Verdict | Resolution |
|----|--------|----------|---------|------------|
| E1-ftp-bookmarks | ftp | `ftp3.cpp`, `dialogs1.cpp` bookmark listboxes | fixed (code) | `SendMessageW(LB_ADDSTRING/LB_INSERTSTRING)` via `SplU8ToWAlloc` |
| ftp-log-ops | ftp | `ctrlcon2.cpp` log text, `dialogs2.cpp` welcome/reply + wait windows, `dialogs4.cpp` parser-test LV + copy/move subjects, `dialogs5.cpp` title, `dialogs7.cpp` LV tooltip, `dialogs8.cpp` history combos/browse/rename | fixed (code) | ~25 sites converted to W APIs with ANSI fallback |
| ftp-virtual-lv | ftp | `dialogs6.cpp` Connections/Items virtual listviews | fixed (code) | `ListView_SetUnicodeFormat(TRUE)` + `LVN_GETDISPINFOW` handlers (packac pattern) |
| sftp-own-ui | sftp | `dialogs.cpp` (16 sites), `logs.cpp` (3 sites) | fixed (code) | new plugin-local `SetDlgItemTextU8`/`GetDlgItemTextU8`/`ListBoxAddStringU8` helpers + `GetOpenFileNameW` browse |
| core-waitwnd | core | `src/dialogs3.cpp` `CWaitWindow::PaintText`+measure | fixed (code) | found by sftp audit: wait-window texts carry paths; `DrawTextW` measure+draw |
| E5-regedt-paths | regedt | `dialogs.cpp` config command + export target, `utils.cpp` W common dialog | fixed (code) | `SplWToU8`/`SplU8ToW` + `GetOpenFileNameW`; ANSI fallback kept |
| zip | zip | own config dialogs (local `U8` helpers) | verified-correct (code review) | 004/005 pattern present |
| 7zip | 7zip | config dialogs | verified-correct (code review) | conversion present |
| tar / uncab / uniso | — | config only, listing via core | verified-correct (code review) | spot-check at walkthrough |
| checksum | checksum | file-list dialogs | verified-correct (code review) | U8 helpers used |
| pictview / renamer / undelete | — | via `winliblt`/history combos (005) | verified-correct (code review) | spot-check at walkthrough |
| folders / portables / peviewer / dbviewer | — | listing via core; content OoS | verified-correct (code review) | spot-check at walkthrough |
| diskmap | diskmap | own chrome (`ZTextToW`+`ExtTextOutW`) | verified-correct (code review 2026-07-17) | already on 004 pattern |
| filecomp | filecomp | headers `DrawTextW`, title `SetWindowTextW` | verified-correct (code review 2026-07-17) | already on 004 pattern |

## Already safe (regression references — do not touch)

`CMessageBox` (A1), `CStaticText` (A3), `CTruncatedString` (A8),
command-line combo (A9), dialog transfers B1–B10/B12–B16, shared
`winliblt::EditLine` (D1), history combos (D2), validation loops C1–C3,
viewer title (`viewer3.cpp:52-55`). Viewer *content* rendering: out of
scope (spec).

## New shared helpers added by this feature

- `SalListBoxAddStringU8` (`src/common/winlib.{h,cpp}`) — LB_ADDSTRING wide with ANSI fallback
- `SalInsertMenuItemU8` (`src/common/winlib.{h,cpp}`) — MFT_STRING menu insert wide with ANSI fallback
- `CStatusWindow::{TextW,TextLenW,SizeW}` model + `GetItemText`/`DrawTextSeg`/`DrawEllipsis` (`src/stswnd.{h,cpp}`)
- `CToolTip::{TextW,TextLenW}` mirror (`src/tooltip.{h,cpp}`)
