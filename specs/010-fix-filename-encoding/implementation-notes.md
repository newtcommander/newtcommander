# Implementation Notes: Feature 010 — Display Encoding + Long-Path Follow-ups

**Feature**: `010-fix-filename-encoding` | **Branch/commits**: `main`
(`f950681`, `1a17a04`, `ef18626`, `6050f91`) | **Written**: 2026-07-18

This document is the complete working context of feature 010 for future
follow-up work: what was wrong, what was changed and why, which patterns to
follow, and what remains. Companion artifacts in this directory:
[spec.md](spec.md) (requirements + clarifications),
[research.md](research.md) (root causes R1–R9),
[contracts/display-conversion-contract.md](contracts/display-conversion-contract.md)
(rules C1–C7 every fix follows), [surface-inventory.md](surface-inventory.md)
(per-surface audit verdicts), [quickstart.md](quickstart.md) (verification
walkthroughs), [tasks.md](tasks.md) (task state).

---

## 1. Architecture primer (read this first)

Established by feature 004, unchanged by 010 — every fix builds on it:

- **Internal strings**: file names, paths, and app/localized display strings
  are **UTF-8 in `char*`**. The build is **ANSI** (no `UNICODE`/`_UNICODE`;
  `salamand.vcxproj` has no `<CharacterSet>`), so unsuffixed Win32 macros
  resolve to the `-A` variants. Passing UTF-8 bytes to an `-A` API = mojibake.
- **OS boundary**: convert to UTF-16 at the call site. File APIs go through
  long-path wrappers (`SalGetFileAttributes`, `SalFindFirstFile`, … in
  `src/common/salfileio.cpp`) which use `SalPathToWExtAlloc`
  (`src/common/salpath.cpp`) to prepend `\\?\` — max path is
  `SAL_MAX_PATH_W = 32767` WCHARs; the UTF-8 worst case is
  `SAL_MAX_PATH_UTF8 = 3*32767+1` bytes (`src/common/salpath.h`).
- **Conversion primitives** (`src/common/salunicode.h`): `SalU8ToW`,
  `SalWToU8`, `SalU8ToWAlloc`, `SalWToU8Alloc` — strict (invalid input →
  0/NULL, never silent replacement).
- **Window-text helpers** (`src/common/winlib.h`, feature 005 + 010):
  `SalSetWindowTextU8`, `SalGetWindowTextU8`, `SalSetDlgItemTextU8`,
  `SalGetDlgItemTextU8`, `SalComboAddStringU8`, `SalListBoxAddStringU8` (new
  in 010), `SalListViewSetItemTextU8`, `SalInsertMenuItemU8` (new in 010).
  Plugin-side equivalents: `SplU8ToW*` in `src/plugins/shared/splunicode.h`.
- **The mandatory pattern** (contract C1–C4): convert → `-W` API → `free()`,
  and ALWAYS keep the legacy `-A` call as the fallback branch when conversion
  fails (invalid UTF-8 = transitional data; never drop text). Measure/index
  text in the SAME units as the drawn form (WCHAR when wide) — never mix
  byte offsets with wide rendering.

## 2. What the feature set out to fix (and did)

Two reported defects, both display-time conversion gaps left after 004/005:

### 2.1 Directory Line mojibake (`G:\Můj disk\AI` → `G:\MĹŻj disk`)

`CStatusWindow` (`src/stswnd.{h,cpp}`) — one class renders both the
Directory Line (`blTop`) and the bottom Info Line (`blBottom`). It drew raw
UTF-8 via `ExtTextOutA` and did ALL measurement/ellipsis/hot-track math in
**bytes** (`AlpDX` per byte, `HotTrackItems.Offset/Chars` byte offsets) —
this both garbled accents and dropped trailing path components (byte-vs-glyph
width mismatch made the truncation math fire early and land mid-sequence).
This was audit item **A2, knowingly deferred by feature 005**.

**Fix (commit `f950681`)**: UTF-16 rebase of the class. New members `TextW`/
`TextLenW`/`SizeW` — converted ONCE in `SetText`/`SetSize`; `AlpDX` and all
`HotTrackItems` offsets are WCHAR-indexed when the wide mirror exists
(`BuildHotTrackItems` measures with `GetTextExtentExPointW`, then a post-pass
remaps the byte offsets produced by the `'\\'`-scanning to WCHAR units via a
local `U8BytesToWChars`); drawing goes through new private helpers
`DrawTextSeg`/`DrawEllipsis` (`ExtTextOutW`, ANSI fallback when
`TextW == NULL`); consumers get the true path back via `GetItemText`
(`SalWToU8`); clipboard copy uses `CopyTextToClipboardW`; the drag image and
free-space text render wide. Producers (`DirectoryLineSetText`,
`ItemFocused`/`SetSubTexts` byte offsets) are unchanged — the class converts
at its boundary. `CToolTip` (`src/tooltip.{h,cpp}`) got the same mirror
(`TextW[TOOLTIP_TEXT_MAX]`), covering the dir-line/panel-long-name/throbber
tooltips.

### 2.2 Alt+F5 Pack dialog packer combo mojibake

Packer titles are UTF-8 (from localized `.slg` via `LoadStr`, or from the
registry via `SalRegQueryValueExW8` — the registry round-trip was verified
NOT lossy). The dialog is an ANSI window (`CDialog` default
`unicodeWnd=FALSE`) and filled the combo with raw `CB_ADDSTRING` →
CP_ACP mis-decode. Feature 005 missed it because the fill bypasses
`CTransferInfo::EditLine`.

**Fix (commit `f950681`)**: `SalComboAddStringU8` in `CPackDialog`/
`CUnpackDialog::Transfer` (`src/dialogs3.cpp`) incl. the IDE_PATH combos;
config pages (`src/dialogsp.cpp`) incl. the External Archivers listbox
(new `SalListBoxAddStringU8`); **systemic fix of the shared `CEditListBox`**
(`src/edtlbwnd.cpp`): owner-draw via `DrawTextExW`, edit round-trip via
`SalSet/GetWindowTextU8` — this fixed all edit-list-box config pages at once.

**Legacy-config policy (spec clarification 2026-07-17)**: entries stored by
pre-UTF-8 versions are NOT converted — `THIS_CONFIG_VERSION` bumped 104→105
(`src/mainwnd2.cpp`) and the four packer sections (Custom Packers/Unpackers,
Predefined Archivers, Archive Associations) are **rebuilt from defaults**
when `Configuration.ConfigVersion < 105` (skip `Load()`, `DeleteAll*()` +
`AddDefault(0)`; preferred indexes not carried over). Note: 004 never bumped
the version, so pre/post-004 configs are indistinguishable → the reset is a
blanket one-time reset for anything older than 105.

### 2.3 The rest of the audit (US3, same commit)

- Owner-draw menus (A4): `menu1/3.cpp`, `menubar.cpp` — wide draw+measure,
  columns split on `'\t'` first then converted separately. `menu2.cpp` has
  no text rendering.
- Toolbars (A5): `toolbar2/3/4.cpp` — measure-at-layout and draw-at-paint
  converted in lockstep (both re-convert from the same UTF-8 → deterministic
  same-form). Beware `CBottomTBData::Text[15]` is stored WITHOUT terminator
  → explicit-length conversion.
- `PathCompactPath` remnant: `src/editwnd.cpp` `CInnerText` →
  `PathCompactPathW` (pattern source: `src/finddlg1.cpp:4050`).
- Misc (B17 + deferred): panel-background context menu items
  (`shellsup.cpp`, new `SalInsertMenuItemU8`), code-page menu
  (`codetbl.cpp`), icon-overlay + conversion-table listviews
  (`dialogs6.cpp`, `dialogs3.cpp`), `dialogs5.cpp` (23 sites: plugins
  manager page, plugin keys dialog, drive-spec path, viewers/appearance
  combos, task list), label-edit read-backs (`dialogs4.cpp` — read the
  edit control wide via `ListView_GetEditControl`+`SalGetWindowTextU8`
  because the A-notification delivers ACP-mangled text), `packac.cpp`
  (switched the listview to Unicode notification format:
  `ListView_SetUnicodeFormat(TRUE)` + `LVN_GETDISPINFOW` handler — THE
  pattern for owner-data listviews), timestamps listbox + browse fields
  (`salamdr2/3.cpp`), `CWaitWindow` measure+draw (`dialogs3.cpp`).
- Plugins: **ftp** (~30 sites: bookmark listboxes, log window, welcome/reply
  edit, wait windows, parser-test LV, copy/move subjects, history combos,
  operations-dialog virtual listviews via the `LVN_GETDISPINFOW` pattern),
  **sftp** (20 sites; plugin-local `SetDlgItemTextU8`/`GetDlgItemTextU8`/
  `ListBoxAddStringU8` helpers added in `dialogs.cpp/.h`; `GetOpenFileNameW`
  browse), **regedt** (config command + export target fields; plugin-local
  wide `GetOpenFileName` wrapper in `utils.cpp`). Remaining 15 enabled
  plugins verified-correct by code review (listing goes through the core
  panel; diskmap/filecomp already wide).

## 3. The long-path saga (verification uncovered feature-004 gaps)

User testing with the generated long-path tree exposed that **feature 004's
implementation was incomplete** despite its tasks being checked off. Three
rounds:

### Round 1 — "Cannot finish operation because of too long name"
(commit `ef18626`)

Enter/double-click navigation never reached the long-path machinery:

- `CFilesWindow::Execute` (`src/fileswn2.cpp`) composed the new path in
  stack buffers `char path[MAX_PATH]` + `SalPathAppend(..., MAX_PATH)` and
  errored out. **Fix**: heap-backed `SAL_MAX_PATH_UTF8` buffers via a local
  RAII holder (`CPathBufs` — the frame is reentrant through
  `ExecuteAssociation` message loops, so ~400KB of stack arrays was not
  acceptable); all append limits raised (disk subdir, archive entry,
  association launch, ZIP top-index bookkeeping).
- `CFilesWindow::ReadDirectory` (`src/fileswn3.cpp`) copied the current path
  into `char fileName[MAX_PATH+4]` with an **unbounded loop** — a latent
  stack overflow once long paths flow. **Fix**: `SAL_MAX_PATH_UTF8+4` buffer;
  wait-window text via `_snprintf_s(_TRUNCATE)`.
- Directory Line hot-track click/drag buffers `MAX_PATH` → heap
  `SAL_MAX_PATH_UTF8`; tooltip copy guarded by `TOOLTIP_TEXT_MAX` (5000).

### Round 2 — "The system cannot find the file specified" + mojibake popup
(commit `6050f91`)

Two independent causes, found by parallel read-only investigations:

1. **`SalCheckPath` truncation** (`src/salamdr5.cpp`): the single
   path-accessibility engine (used by `ChangePathToDisk` via
   `SalCheckAndRestorePathWithCut`) handed the path to its check worker
   thread through global `char ThreadPath[MAX_PATH]` (+ worker-local
   `threadPath[MAX_PATH+5]`). The long-path-capable `SalGetFileAttributes`
   then tested a truncated, nonexistent 259-byte prefix →
   `ERROR_PATH_NOT_FOUND` → the WithCut loop (`IsDirError` treats
   FILE/PATH_NOT_FOUND as "cut a component and retry") chopped the path
   below 260 and landed on an existing ancestor + showed the shorter-path
   warning. **Fix**: buffers widened to `SAL_MAX_PATH_UTF8` — safe because
   the handoff is serialized (single writer under `CheckPathCS`; workers
   copy before setting `ctsCanTerminate`; workers have default 1MB stacks).
   The ACCESS_DENIED fixed-disk probe switched from ANSI `FindFirstFile` to
   `SalFindFirstFile`. Path-bearing `sprintf`s in the flow hardened with
   `_snprintf_s(_TRUNCATE)`.
2. **`GetErrorText` emitted ACP** (`src/salamdr2.cpp:143`, `FormatMessageA`).
   Messages composed as `sprintf(LoadStr-format [UTF-8], path [UTF-8],
   GetErrorText [ACP])` are **invalid UTF-8 as a whole** → the 005-fixed
   `CMessageBox` falls back to ANSI rendering → the ACP error text renders
   fine but the UTF-8 path renders as mojibake. This affected ~250 core call
   sites (all of `worker.cpp`'s file-operation errors). **Fix**:
   `FormatMessageW` + `SalWToU8` into the existing cyclic buffer — result
   stays ≤ `MAX_PATH+20` incl. null (the documented plugin-SDK contract,
   `spl_gen.h:1539`, so no caller-buffer audit was needed), truncation only
   at a UTF-8 character boundary, the original `FormatMessageA` code kept
   verbatim as fallback. Same treatment for the plugin-facing
   `CSalamanderGeneral::GetErrorText` buffered variant (`src/zip.cpp:1569`;
   the `buf==NULL` branch already delegates to the core). **Companion
   fixes** for ANSI sinks that now receive UTF-8: `IDS_ERROR` statics in
   `CFileErrorDlg`/`CHiddenOrSystemDlg`/`CCannotMoveDlg` (`dialogs.cpp`) and
   ADS/viewer error dialogs (`dialogs6.cpp`) → `SalSetWindowTextU8`; Find
   log Text column (`finddlg2.cpp`) → wide `LVM_SETITEMTEXTW`;
   pre-language-DLL startup config errors (`regwork.cpp`) → `MessageBoxW` +
   existing `GetErrorTextW`.

**LESSON for future work**: feature 004's checked-off tasks did NOT equal
complete coverage. When touching a flow, verify the ENTIRE chain end to end
(compose → check → list → display → error-report); assume unconverted
`MAX_PATH` buffers and `-A` calls remain until proven otherwise.

## 4. Key rules distilled (for anyone continuing this work)

1. **Never compose mixed-encoding strings.** Everything `sprintf`-ed into a
   user-visible message must be UTF-8 (LoadStr is UTF-8, paths are UTF-8,
   `GetErrorText` is UTF-8 since `6050f91`). One ACP byte poisons the whole
   message into the ANSI fallback.
2. **Units must match the rendered form.** Byte offsets with `-A` drawing,
   WCHAR offsets with `-W` drawing. Mixed bookkeeping caused the dropped
   `\AI` component (`CStatusWindow`) — model: convert once on change, keep
   offsets in WCHAR, convert back at consumer boundaries.
3. **Always keep the `-A` fallback branch** for invalid UTF-8 (transitional
   data, e.g. legacy-encoded config values). `SalU8ToWAlloc` returning NULL
   IS the signal.
4. **Owner-data listviews**: use `ListView_SetUnicodeFormat(TRUE)` +
   `LVN_GETDISPINFOW` (see `packac.cpp`, `ftp/dialogs6.cpp`), not per-fill
   conversion — pull-model A-notifications cannot be fixed at the fill site.
5. **Label edits**: `LVN_ENDLABELEDIT` (A) delivers ACP-mangled text; read
   the edit control directly (`ListView_GetEditControl` +
   `SalGetWindowTextU8`) — see `dialogs4.cpp`.
6. **Long-path buffers**: `SAL_MAX_PATH_UTF8` is ~96KB — stack arrays only
   in non-reentrant frames with known 1MB stacks (worker threads, one per
   frame); otherwise heap with RAII or single-exit free. Never `strcpy` a
   panel path (`GetPath()`) into a `MAX_PATH` buffer.
7. **Path-bearing `sprintf` into fixed buffers** → `_snprintf_s(_TRUNCATE)`.

## 5. Test data

- `%LOCALAPPDATA%\Temp\salamander-test\010\` — Unicode sample set:
  `Můj disk\AI\` (reported defect path shape), composed+decomposed `č-dir`
  pair, `Тест-Ελλάδα-测试`, `emoji-🙂-dir`, `plain-ascii` (regression).
- `%LOCALAPPDATA%\Temp\salamander-test\010\long-paths\` — long-path set:
  540-char Unicode deep path (3 levels), 255-char single component,
  265-char edge case, 327-char ASCII-only. Created via `\\?\`-prefixed .NET
  calls; Explorer/old Salamander cannot open them — deleting requires
  `\\?\`-aware tooling.

## 6. Commit map (all on `main`)

| Commit | Content |
|--------|---------|
| `f950681` | All display-encoding fixes (37 surfaces, ~30 files), new helpers, config reset gate, spec-kit artifacts |
| `1a17a04` | Long-path verification walkthrough added to quickstart.md |
| `ef18626` | Long-path navigation round 1: `Execute`, `ReadDirectory`, hot-track buffers |
| `6050f91` | Long-path round 2: `SalCheckPath` truncation, `GetErrorText` → UTF-8, companion sinks |

## 7. Remaining work

### Interactive verification (blocked on a human at the screen)
Tasks T011/T016/T025/T026 in [tasks.md](tasks.md) = the quickstart.md
walkthroughs (P1 chrome, P2 packer lists incl. the legacy-reset case —
NOTE: first start after upgrade intentionally rebuilds packer sections from
defaults, custom entries are lost per the clarified policy — P3 inventory
sweep, long-path walkthrough, ASCII regression). After the pass, flip
`fixed (code)` verdicts in [surface-inventory.md](surface-inventory.md) to
`fixed (verified)`. **Status of the long-path walkthrough: two rounds of
fixes landed; the third user retest is pending.**

### Known accepted limitations (documented, deliberate)
- `.lnk` shortcut resolution stays MAX_PATH-bound (IShellLink API) and its
  name conversion still uses CP_ACP (`fileswn2.cpp` lnk branch) — deep/
  Unicode link targets degrade gracefully with an error box.
- Drive-bar volume labels: ANSI end-to-end (acquisition via
  `GetVolumeInformationA`, `drivelst.cpp`) — consistent, no mojibake, but
  labels outside the ACP cannot render; widening = separate feature.
- Drive-not-ready/reparse retry anchors (`CheckPathRootWithRetryMsgBox`)
  stay root-capped at MAX_PATH — internal graceful fallback.
- `SetCurrentDirectory(GetPath())` in `ReadDirectory` is ANSI and fails
  silently for long paths (result ignored; "so that it works better" only).
- FTP log-edit caret restore uses byte offsets (EM_SETSEL clamps — cosmetic).
- FTP/winliblt dialog titles: winliblt creates ANSI dialogs (`UnicodeWnd`
  not exposed to plugins); W-set titles get down-converted by USER32 —
  strictly better than raw UTF-8 bytes, not perfect.

### Candidate follow-ups (not started)
- Plugin-internal raw ANSI sink sweep (only core sinks were audited for the
  `GetErrorText` UTF-8 switch; plugins consuming `SalamanderGeneral->
  GetErrorText` into their own A-sinks may show mojibake for Czech system
  messages).
- Feature-004 completeness audit: systematically grep the remaining
  `MAX_PATH` buffers / `-A` file APIs on user-driven flows (rename, copy
  worker, viewers, Find) and test each with the long-path tree — the
  Execute/ReadDirectory/SalCheckPath chain proved the 004 task list
  unreliable as a coverage guarantee.
- `winliblt` `UnicodeWnd` support so plugin dialogs can be Unicode windows.
