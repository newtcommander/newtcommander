# Research: Complete Revision of File Name and Path Display Encoding

**Feature**: `010-fix-filename-encoding` | **Date**: 2026-07-17
**Method**: Four parallel read-only code investigations (encoding
architecture, Directory Line trace, Pack dialog trace, app-wide
surface sweep). All file:line references verified against the current
`main`-based branch.

## R1: Text-encoding architecture to build on (established by 004/005)

**Decision**: Reuse the feature 004/005 boundary-conversion
architecture unchanged. Internal file names/paths and app-stored
display strings are **UTF-8 in `char*`**; the build stays **ANSI**
(no `UNICODE`/`_UNICODE` — `salamand.vcxproj` has no `<CharacterSet>`,
so unsuffixed Win32 macros resolve to `-A` variants); conversion to
UTF-16 happens only at OS boundaries via the central helpers.

**Helper inventory (the toolbox for every fix in this feature)**:

| Helper | Where | Purpose |
|--------|-------|---------|
| `SalU8ToW` / `SalWToU8` | `src/common/salunicode.h:42-43` | fixed-buffer UTF-8↔UTF-16 |
| `SalU8ToWAlloc` / `SalWToU8Alloc` | `src/common/salunicode.h:46-47` | allocating variants (caller `free()`s) |
| `SalSetWindowTextU8` / `SalGetWindowTextU8` | `src/common/winlib.h:327,330`, `winlib.cpp:1093,1107` | window text |
| `SalSetDlgItemTextU8` / `SalGetDlgItemTextU8` | `src/common/winlib.h:331-332` | dialog items |
| `SalComboAddStringU8` | `src/common/winlib.h:333`, `winlib.cpp:1146` | `CB_ADDSTRING` via `SendMessageW` |
| `SalListViewSetItemTextU8` | `src/common/winlib.h:336` | ListView cells |
| `SplU8ToW` / `SplWToU8` etc. | `src/plugins/shared/splunicode.h` | plugin-side equivalents |

Conversions are strict (invalid UTF-8 → 0/NULL, `salunicode.h:29-31`);
the canonical pattern (see `CTransferInfo::EditLine`,
`src/common/winlib.cpp:1033-1087`) is: convert → wide API → `free()`,
with a **legacy-ANSI fallback** when the string is not valid UTF-8.

**Rationale**: The pattern is proven across the panel drawing, the
window title, and all 005-fixed dialogs; deviating (e.g. a `UNICODE`
build) was explicitly rejected in `specs/004-long-paths-unicode/research.md:33-41`.

**Alternatives considered**: Full `UNICODE` build (rejected by 004);
per-process UTF-8 ACP manifest (rejected by 004 — GDI does not honor
it).

## R2: Directory Line / Info Line root cause and fix approach

**Root cause (confirmed)**: The Directory Line and the bottom info
line are both `CStatusWindow` (`src/stswnd.h:60-207`,
`src/stswnd.cpp`), audit item **A2 explicitly deferred by feature
005** (`specs/005-fix-unicode-display/validation-results.md:99`). The
UTF-8 path from `CFilesWindow::DirectoryLineSetText`
(`src/fileswn1.cpp:1662-1739`) is stored verbatim
(`CStatusWindow::SetText`, `stswnd.cpp:119-165`) and then:

- drawn with **`ExtTextOutA`** on raw UTF-8 bytes —
  `stswnd.cpp:1004,1015,1019-1020,1029,1045,1053,1086-1087,1095,1099-1104`
  (info line also `DrawTextA` at `stswnd.cpp:2225`) → mojibake;
- measured with **`GetTextExtentExPointA`** into the per-**byte**
  advance array `AlpDX` (`stswnd.cpp:188,274`; decl `stswnd.h:93`);
- shortened and hot-tracked with **byte-indexed** arithmetic
  (`Paint` truncation math `stswnd.cpp:881-954`; segment map
  `BuildHotTrackItems` `stswnd.cpp:190-233`; consumers
  `FindHotTrackItem` `:619-638`, `GetHotText` `:605-617`, drag/click
  handlers `:1802,2082,2098`). The byte-vs-glyph mismatch makes the
  overflow test fire early and the offset math land mid-sequence —
  this is why the trailing `\AI` component disappears in the reported
  example, not just the accents.

**Decision**: Rebase `CStatusWindow` text handling on UTF-16:
convert once in `SetText`/`SetSubTexts` (cache a `WCHAR*` alongside
the existing UTF-8 `Text`), build `AlpDX` per WCHAR with
`GetTextExtentExPointW`, keep all `HotTrackItems` offsets/`Chars`,
`EllipsedChars`, `SubTexts` offsets in **WCHAR units**, draw with
`ExtTextOutW`/`DrawTextW`, and convert hot-track text back with
`SalWToU8` where consumers need UTF-8 (`GetHotText`, drag/copy
handlers). Invalid-UTF-8 input falls back to the current ANSI route
(mirror of the panel pattern in `src/fileswn4.cpp:689,773,781` — the
reference implementation).

**Alternatives considered**: (a) Convert at each draw call only —
rejected: leaves the byte-based measurement/hot-tracking broken (the
dropped-component bug would remain). (b) Keep byte offsets and map
byte↔WCHAR at every use — rejected: error-prone double bookkeeping;
WCHAR-only bookkeeping with UTF-8 conversion at the producer/consumer
edges is simpler.

## R3: Pack dialog packer list root cause and fix approach

**Root cause (confirmed)**: `CPackDialog` is an **ANSI** dialog
(`CDialog` defaults `unicodeWnd = FALSE`, `src/common/winlib.h:350-365`;
`DialogBoxParamA` in `CDialog::Execute`, `winlib.cpp:606`). Its
`Transfer` fills the packer combo with raw ANSI `CB_ADDSTRING` on
UTF-8 titles: `src/dialogs3.cpp:1845`
(`SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)PackerConfig->GetPackerTitle(i))`);
same for `CUnpackDialog` at `:2091` and the `IDE_PATH` combo entries
at `:1875,1878`. The titles are UTF-8 both when built from defaults
(`CPackerConfig::AddDefault`, `src/packers.cpp:240-271`, via
`LoadStr(IDS_DP_*)` — localized `.slg` strings contain diacritics,
which is why even a fresh configuration shows mojibake) and when
loaded from the registry (see R4). The same `DialogProc` already
converts the subject label and the path edit correctly
(`dialogs3.cpp:1965-1972,1995-2035`) — the combos were simply missed
because the fill branch bypasses `CTransferInfo::EditLine`.

**Decision**: Replace the raw `CB_ADDSTRING` calls with
`SalComboAddStringU8` (drop-in, precedent `src/editwnd.cpp:1711`), in
both Pack (`dialogs3.cpp:1845,1875,1878`) and Unpack
(`dialogs3.cpp:2091` + its path combo). Fix the sibling surfaces that
render the same strings: Config → Pack/Unpack custom-packer pages
(`src/dialogsp.cpp:238,377,666,781` — `EDTLBN_GETDISPINFO`
edit-list-box path needs a wide disp-info route or conversion at
fill), Config → External Archivers listbox (`dialogsp.cpp:891`
`LB_ADDSTRING`, `:1116`), adding a `SalListBoxAddStringU8` helper next
to the combo helper if needed.

**Alternatives considered**: Making the whole dialog a Unicode window
(`unicodeWnd = TRUE`) — rejected: changes every control's text
semantics in that dialog at once (regression risk for already-working
transfers), against incremental-modernization principle.

## R4: Registry round-trip and the legacy-config reset (clarified 2026-07-17)

**Finding**: The registry layer already speaks UTF-8 correctly.
`SetValue`/`GetValue` route through `SalRegSetValueExW8`/
`SalRegQueryValueExW8` (`src/salamdr6.cpp:2379,2306`), which store
REG_SZ as native UTF-16 and read it back as UTF-8
(`MultiByteToWideChar(CP_UTF8, ...)` with a CP_ACP fallback for
invalid UTF-8 at `salamdr6.cpp:2390-2395`). Values written by
pre-Unicode versions used `RegSetValueExA`, so Windows itself
converted ACP→UTF-16 at write time and they read back as correct
UTF-8. **The visible garbling is therefore at the dialog-population
boundary (R3), not in load/save.**

**Decision**: Implement the clarified reset policy as a
**version gate**, the established idiom: bump `THIS_CONFIG_VERSION`
(currently `104`, `src/mainwnd2.cpp:143`) and, in the packer-section
load orchestration (`src/mainwnd2.cpp:2880-2977` — Custom Packers,
Custom Unpackers, Predefined Archivers, Archive Associations), skip
loading saved entries when `Configuration.ConfigVersion` predates the
UTF-8 baseline and rebuild the section from current defaults
(`DeleteAllPackers()` + `AddDefault()`), mirroring the existing
`ConfigVersion >= 6` plugin gate at `mainwnd2.cpp:2876`. This
satisfies FR-004's "no garbled entry may remain after upgrade" without
attempting conversion, exactly per the clarification. The threshold
version must equal the version that first wrote UTF-8 config (feature
004's baseline) so that healthy post-004 configs are NOT reset.

**Alternatives considered**: Heuristic per-value mojibake detection
and repair — rejected by the clarification decision (reset, don't
convert); also unreliable (valid-UTF-8 test cannot distinguish all
legacy strings).

## R5: Tooltip surface (covers Directory Line tooltip and more)

**Finding**: `CToolTip` measures and draws with `DrawTextA`
(`src/tooltip.cpp:323,640`) on text pulled via `WM_USER_TTGETTEXT`;
the Directory Line answers case 4 with its raw UTF-8 `Text`
(`src/stswnd.cpp:1686-1700`), throbber/security tooltips share the
route (`stswnd.cpp:1720-1730`).

**Decision**: Convert inside `CToolTip` (wide measure + `DrawTextW`
with ANSI fallback) — one fix covers the dir-line tooltip, panel
long-name tooltip, and throbber/security tooltips (005 deferred item
A6).

## R6: Remaining owner-draw chrome — menus and toolbars (005 items A4/A5)

**Finding**: Custom owner-draw menus (`src/menu3.cpp` ~9 draw sites,
`menu1.cpp` ~3, `menubar.cpp` ~3, `menu2.cpp`) and toolbars
(`src/toolbar2.cpp:299,633,639`, `toolbar3.cpp:498`,
`toolbar4.cpp:1421`) draw path-bearing text (dir history, drive menu
volume labels, hot paths, user menu) via ANSI GDI with no `-W` calls
in those files.

**Decision**: Apply the same convert-at-draw pattern
(`SalU8ToW`/`SalU8ToWAlloc` + `DrawTextW`/`ExtTextOutW`/
`GetTextExtentPoint32W`, ANSI fallback). Where text is measured for
layout, measurement must move to the wide string in the same change
(lesson of R2).

**Note**: Drive-bar volume labels are today ANSI end-to-end
(`GetVolumeInformationA` → ANSI draw, `src/drivelst.cpp:1110-1121,
1739`) — consistent in/out, **no mojibake**, but labels outside the
active code page cannot render. Recorded in the inventory as a
follow-up limitation, not a 010 defect (spec targets garbled
rendering of stored names; acquisition-side widening is a separate
feature).

## R7: Audit inventory — scope, seed, and verification protocol

**Decision**: The audit inventory (FR-005) will live at
`specs/010-fix-filename-encoding/surface-inventory.md`, seeded from
the sweep below, extended during implementation, and verified by the
clarified manual-walkthrough protocol (sample-name set from feature
005: Czech diacritics composed+decomposed, Cyrillic/Greek/CJK, emoji).

**Sweep result (candidate magnitude ~80–100 sites in ~15–18 files)**:

| Area | Files (samples) | Est. sites | Status |
|------|-----------------|-----------|--------|
| Directory/Info line | `stswnd.cpp` | ~16 draws + measurement | DEFECTIVE (R2) |
| Pack/Unpack dialogs | `dialogs3.cpp:1845,1875,1878,2091` | 4 | DEFECTIVE (R3) |
| Pack config pages | `dialogsp.cpp:238,377,666,781,891,1116` | ~10 | DEFECTIVE (R3, 005 item B11) |
| Custom menus | `menu1/2/3.cpp`, `menubar.cpp` | ~15 | DEFECTIVE (R6, A4) |
| Toolbars/hot paths | `toolbar2/3/4.cpp` | ~5 | DEFECTIVE (R6, A5) |
| Tooltips | `tooltip.cpp:323,640` | ~4 | DEFECTIVE (R5, A6) |
| Misc deferred (B17, label-edit, browse, `packac.cpp:186`, `editwnd.cpp:1567` `PathCompactPath`) | `salamdr2/3.cpp`, `dialogs4/5.cpp`, `shellsup.cpp`, `codetbl.cpp` | ~15 | VERIFY per site |
| ftp plugin | `dialogs1.cpp:1231` (bookmarks, E1), log/operation windows | ~5–10 | PARTIAL |
| sftp plugin | `logs.cpp`, connect dialogs | ~3 | PARTIAL |
| regedt plugin | on-disk path fields (E5) | ~3–5 | PARTIAL |
| Other 15 enabled plugins | listings go through core panel | spot-check | LIKELY OK |

**Already safe (regression references, do not touch)**: panel list
drawing (`fileswn4.cpp` `ExtTextOutW`), window title
(`mainwnd1.cpp:1899-1904`), viewer title (`viewer3.cpp:52-55`),
`CMessageBox`, `CStaticText`, `CTruncatedString`
(`salamdr4.cpp:157+`), command-line combo, 005-fixed dialog transfers
B1–B10/B12–B16, shared `winliblt::EditLine`, plugins diskmap and
filecomp.

## R8: Performance

**Decision**: Convert once per text change (`SetText` /
`BuildHotTrackItems` / menu-item creation), never per `WM_PAINT`
where avoidable; cache the wide string next to the UTF-8 original.
Conversion cost is O(length) on strings ≤ a few hundred bytes,
invoked on path change / focus change — no measurable impact. The
panel drawing (hot path) already pays this cost per item since 004
with no regression, which bounds the risk empirically.

## R9: Out-of-scope confirmations

- Viewer *content* rendering (`src/viewer.cpp` body draws) — file
  content, out of scope per spec.
- Panel header column titles (`filesbx2.cpp:230,270`) — localized
  resource strings, not file names; consistent today.
- Drive-bar volume label *acquisition* widening — follow-up (R6 note).
- Plugins disabled in `plugins.cfg` — out of scope per clarification.
