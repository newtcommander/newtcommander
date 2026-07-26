# Research: Directory-Listing Crash on Long Multi-Byte Names (031)

**Date**: 2026-07-23 | **Spec**: [spec.md](spec.md)

## R0 — Dump forensics: the crash is confirmed, located, and reproducible

Five fresh WER crash dumps from the user's machine, all from today
(2026-07-23 10:40–10:48), all of process
`D:\Projects\newtcommander\build\newtcommander\Release_x64\salamand.exe`
(the current Release x64 build, PDB matches bit-for-bit).

Newest dump `salamand.exe.61172.dmp` analyzed with DbgEng against the local
PDB:

```
ExceptionCode: c0000409 (Security check failure or stack buffer overrun)
Subcode: 0x2 FAST_FAIL_STACK_COOKIE_CHECK_FAILURE

salamand!__report_gsfailure
salamand!CFilesWindow::DrawIcon+0xb00            [src\fileswn4.cpp @ 369]   <- /GS epilogue of DrawIcon
salamand!CFilesWindow::DrawBriefDetailedItem+0x2d9 [src\fileswn4.cpp @ 578]
salamand!CFilesBox::PaintAllItems+0x3c9          [src\filesbx1.cpp @ 233]
salamand!CFilesWindow::RefreshListBox+0x221f     [src\fileswn2.cpp @ 4354]
salamand!CFilesWindow::Execute+0x11eb            [src\fileswn2.cpp @ 547]
salamand!CFilesBox::WindowProc (WM_LBUTTONDBLCLK)
```

The user double-clicked into `D:\Temp`, the panel listed it, and the first
repaint of the long-named directory item smashed the /GS stack cookie of
`CFilesWindow::DrawIcon`. All four older dumps carry the same
`FAST_FAIL_STACK_COOKIE_CHECK_FAILURE` subcode. Because the fast-fail fires
in the function **epilogue**, line 369 is `DrawIcon`'s closing brace; the
overflowing buffer is one of its locals.

The perceived "freeze then crash": the /GS fast-fail path suppresses SEH and
invokes WER dump collection (~4 MB dump written) before the process dies, so
the window stops responding a moment before it disappears.

## R1 — Root cause

`CFilesWindow::DrawIcon` (`src\fileswn4.cpp`):

```cpp
char fileName[MAX_PATH + 4];              // line 220 — 264 bytes
...
memmove(fileName, f->Name, f->NameLen);   // line 227 (directory branch)
*(DWORD*)(fileName + f->NameLen) = 0;     // line 228 — writes 4 more bytes
```

Since feature 004, `CFileData::Name` is **UTF-8** and `CFileData::NameLen`
is its **byte** length — up to 765 bytes for a 255-character component
(enumeration converts into `nameU8[SAL_FIND_NAME_U8]`, 780 bytes, in
`ReadDirectory`; verified in `fileswna.cpp:406-435` and
`salfileio.cpp:45-69`). The reported directory name is 215 characters =
**330 UTF-8 bytes**; `330 + 4 > 264` ⇒ stack smash ⇒ fast-fail.

**Why only multi-byte names crash**: a disk name component is ≤ 255 UTF-16
characters. Pure-ASCII names are ≤ 255 bytes and always fit in 264 bytes.
The buffer overflows only when `NameLen > 260`, i.e. when multi-byte UTF-8
expansion pushes the byte length of a legal-length name past the legacy
buffer — precisely the "byte length vs. character count" defect class named
in the spec. This is why the historic character-count-driven long-path
testing (features 011/013/014/027) never hit these sites, and why feature
027's audit — which pattern-matched `AlterFileName` targets and full-path
copy routes — missed this second idiom: `memmove(buf, f->Name, f->NameLen)`
+ DWORD-null-terminate, and the analogous unbounded extension-lowercase
loops.

## R2 — Defect-site inventory (systematic review of the class, FR-005)

Idioms searched program-wide (`src\`, core app):
`memmove/memcpy(<buf>, <x>->Name, <x>->NameLen)`, `[MAX_PATH + 4]` buffers,
`LowerCase[*src++]` copy loops, `AlterFileName` targets, `GetTileTexts`
callers, `TransferBuffer`/`DrawItemBuff` consumers, `SalConvertFindDataW`
call sites.

### CRASH — stack smash (fix: widen to `SAL_FIND_NAME_U8 + 4` + guard)

| # | Site | Buffer | Overflow writer | Trigger |
|---|------|--------|-----------------|---------|
| 1 | `fileswn4.cpp:220,227-228` `DrawIcon` (dir branch) | `fileName[MAX_PATH+4]` | `memmove` NameLen + DWORD 0 | **dump-confirmed**: directory with NameLen > 260 B |
| 2 | `fileswn4.cpp:220,255-256` `DrawIcon` (file dynamic-icon branch) | same `fileName` | same | file (exe/lnk/ico-bearing ext) with NameLen > 260 B |
| 3 | `fileswn4.cpp:1306-1308` `DrawIconThumbnailItem` | `fileName[MAX_PATH+4]` | same | Thumbnails view, file with NameLen > 260 B |
| 4 | `fileswnb.cpp:702-706,716-717` `WM_USER_REFRESHINDEX` | `buf[MAX_PATH+4]` | unbounded `Ext` lowercase loop **and** `memmove` NameLen | icon-thread refresh notification for such a file |
| 5 | `fileswn4.cpp:136,169-172` `DrawIcon` | `lowerExtension[MAX_PATH+4]` | unbounded `Ext` lowercase loop + DWORD 0 | file whose UTF-8 *extension* exceeds 260 B (dot early in a long multi-byte name) |

### CRASH — global-buffer corruption (no /GS for globals: silent memory corruption)

| # | Site | Buffer | Overflow writer | Trigger |
|---|------|--------|-----------------|---------|
| 6 | `salamdr4.cpp:1348,1363-1367` `InternalGetType` | global `InternalGetTypeAux3[MAX_PATH+4]` | unbounded `Ext` lowercase loop + DWORD 0 | Type column painted for a file with UTF-8 ext > 260 B — overwrites adjacent globals |

### CORRUPTION — overflow stays inside a larger buffer (wrong rendering, no smash)

| # | Site | Problem |
|---|------|---------|
| 7 | `filesbx1.cpp:2070-2082` `CFilesBox::GetIndex` (Tiles hit-test) | `buff[3*512]`, `out0` region only 512 B; `GetTileTexts` → `AlterFileName(out0, …)` writes up to 766 B, clobbering the `out1`/`out2` regions |
| 8 | `fileswn0.cpp:3395-3406` `vmTiles` draw path | identical layout — explicitly marked "keep in sync" with #7 |

### SAFE — verified during this review (regression documentation)

- `fileswn2.cpp:3909-3917` Type-column width calc: ext loop into
  `buf[TRANSFER_BUFFER_MAX=1024]` — 764+4 ≤ 1024. Safe.
- `GetCommonFileTypeStr` (`fileswn2.cpp:3535`): loop bounded by
  `uppercaseExt + MAX_PATH - 1`, `_snprintf_s _TRUNCATE`. Safe (truncates).
- `AlterFileName` (`salamdr2.cpp:1743`): byte-wise case remap, output length
  = input length (≤ 765+1); all `TransferBuffer` (1024) targets safe.
- `DrawItemBuff[1024]` consumers (`fileswn4.cpp:753,916`): copy counts
  derived from fitted text ≤ TransferBuffer content. Safe.
- `fileswn1.cpp:1915-1927` clipboard name list: heap, exact size computed
  from `NameLen`. Safe.
- `fileswn3.cpp:1217,1478,1617` `CIconCache` inserts: `NameAndData` is
  heap-allocated with a size computed from `NameLen`. Safe.
- `fileswn3.cpp:631-636` `ReadDirectory` ext-lowercase: writes into the
  enumeration `nameU8[SAL_FIND_NAME_U8]` buffer after the name was copied
  out; ext ≤ 764 + 4 ≤ 780. Safe (re-verify bound in implementation).
- `icncache.cpp:1147+` `ReadAssociations`: `ext[MAX_PATH+4]` filled by
  `RegEnumKeyEx` bounded to `MAX_PATH`. Safe (registry keys, not file names).
- `SalConvertFindDataW` (`salfileio.cpp:45`): size-checked conversions,
  empty string on failure. Safe.
- `drivelst.cpp:869` `root[MAX_PATH+4]` + `GetRootPath`: UNC roots — to be
  re-verified during implementation (server/share components could in theory
  exceed MAX_PATH; out of the paint path, listed for completeness).

## R3 — Fix strategy

**Decision**: for every name-component buffer in the CRASH/CORRUPTION list:

1. Size it `SAL_FIND_NAME_U8 + 4` bytes (784) — `SAL_FIND_NAME_U8` (780) is
   the established worst-case UTF-8 component size from `salfileio.h` (the
   same bound the enumeration conversion uses), `+ 4` for the DWORD
   null-terminator idiom. A `static_assert(sizeof(buf) >= SAL_FIND_NAME_U8 + 4)`
   at each site fences the size against future shrinkage.
2. Guard the copy: if `f->NameLen + 4 > sizeof(fileName)` (possible only for
   plugin/archive-supplied `CFileData` whose names bypass the enumeration
   bound), skip the icon-cache lookup and fall back to the simple symbol —
   graceful degradation, never a truncated lookup acting as a different name
   (FR-004).
3. Bound the extension-lowercase loops by remaining buffer capacity (idiom
   already used by `GetCommonFileTypeStr`); an over-long extension then
   simply misses the association lookup → common file type / simple icon.
4. Tiles pair (#7/#8): enlarge the name region —
   `char buff[(SAL_FIND_NAME_U8 + 4) + 2 * 512]` with
   `out1 = buff + SAL_FIND_NAME_U8 + 4`, `out2 = out1 + 512`, identically in
   both synchronized sites.

**Rationale**: stack growth is 264 → 784 bytes in shallow, non-reentrant
paint frames on the 1 MB main-thread stack — negligible; no heap allocation
enters the hot paint path; behavior for names ≤ 260 bytes is byte-for-byte
identical (Constitution II).

**Alternatives considered**:
- *Heap-allocate per paint call* — rejected: `DrawIcon` runs per item per
  repaint; allocation in the hot path costs more than 520 extra stack bytes.
- *Truncate the copy silently* — rejected: a truncated name used as an
  icon-cache key could match a different entry (acts on a wrong name,
  violates FR-004); the guard + simple-symbol fallback is explicit.
- *Refactor paint code onto a shared helper* — rejected for this fix:
  Constitution III (incremental; don't refactor adjacent untouched code in a
  crash fix). The static_asserts + tests fence the class instead.

## R4 — Regression-protection strategy (FR-006, SC-003)

`saltests.exe` compiles `src\common\{salclip,salfileio,salpath,salunicode}.cpp`
only — the fixed sites live in app-only files it cannot link. The fence is
therefore three-layered:

1. **Compile-time**: `static_assert(sizeof(<buf>) >= SAL_FIND_NAME_U8 + 4, ...)`
   at every widened site — reverting a buffer to `MAX_PATH + 4` fails the
   build (the strongest possible "test fails when fix reverted").
2. **saltests — class invariants** (new `TestLongComponentNames`):
   - the user's exact 215-char diacritics name: `SalWToU8` == 330 bytes —
     asserts `> MAX_PATH + 4` (the class exists) and `< SAL_FIND_NAME_U8`
     (the chosen bound holds);
   - worst-case component: 255 × U+011B (3-byte UTF-8) → 765 bytes, fits
     `SAL_FIND_NAME_U8`; 255 UTF-16 units of surrogate pairs → ≤ 765;
   - `SalConvertFindDataW` with a `WIN32_FIND_DATAW` carrying the 255-char
     multi-byte name fills `nameU8[SAL_FIND_NAME_U8]` completely and
     round-trips exactly (no truncation, no empty-string failure);
   - `SalConvertFindDataW` into a deliberately small buffer yields `""`
     (documented fail-safe), never a truncated name.
3. **saltests — on-disk repro** (extends `TestFileIO`): create a directory
   named with the user's exact 215-character diacritics string under
   `%TEMP%\saltests-deep\`, enumerate its parent via
   `SalFindFirstFile`/`SalFindNextFile`, assert the entry comes back
   byte-exact with length 330, then remove it.

Manual/live verification (quickstart): launch the built app, navigate to
`D:\Temp`, confirm listing + no new WER dump; repeat in Thumbnails and Tiles
view modes.

## R5 — Performance

No new allocations; +520 stack bytes in three paint-path frames; the ext
loops gain one pointer comparison per byte (identical to the existing
`GetCommonFileTypeStr` idiom). Listing/painting cost for ordinary names is
unchanged — satisfies FR-008 / SC-001 (≤ 1 s listing).

## R2b — Implementation-time review results (T010/T011 closure)

Verdicts recorded while executing the fix (all sites re-examined in code):

| Site | Verdict | Evidence |
|------|---------|----------|
| `fileswn3.cpp:631-636` ext-lowercase into `nameU8` | **HARDENED** | writes up to `len-1` ext bytes + 4-byte DWORD terminator into the 780-byte buffer; safe for OS-capped names (≤ 765 B) but only 11 B margin — widened to `3*MAX_PATH + 4` |
| `fileswn3.cpp:1182-1186, 1443-1447` ext loops into `fileName` | SAFE | `fileName` is the `ReadDirectory` local `char fileName[SAL_MAX_PATH_UTF8 + 4]` (98 KB, line 97) |
| `fileswn3.cpp:1264, 1542` ext loops into `buf[SAL_FIND_NAME_U8]` | SAFE | ext ≤ 764 + 4 ≤ 780 (feature 027 sizing) |
| `fileswn3.cpp:1212-1219, 1473-1480` `NameAndData` | SAFE | heap-allocated with exact computed size |
| `drivelst.cpp:869` `root[MAX_PATH+4]` + `GetRootPath` | SAFE (BOUNDED) | `GetRootPath` truncates UNC roots to `MAX_PATH-2` (`salamdr1.cpp:1556`); + `"*.*"` ≤ 263 ≤ 264 |
| `fileswn2.cpp:3909-3917` Type-column width ext loop | SAFE | `buf[TRANSFER_BUFFER_MAX=1024]` ≥ 768 |
| icon reader thread + overlays | SAFE (guarded) | `fileswn1.cpp:484,740,774`, `shiconov.cpp:806-811` skip over-long full paths (lose icon, no overrun) |
| `fileswn5.cpp` `temporarySelected/dosName/newName/nextFocus[MAX_PATH]` | OUT OF PAINT PATH | quick-rename/selection-restore routes; not reachable by listing; deferred to the §Bounded backlog with the 027 leftovers |

**Key bound discovered**: `CSalamanderDirectory::AddFile` (`zip.cpp:5829`)
rejects plugin/archive entries with `NameLen > MAX_PATH - 5` (= 255 bytes),
so plugin-supplied names can never exceed the disk component bound — the
`nameFits` guards added at the fixed sites are defense-in-depth.

## R2c — Second defect found while verifying the fix: mojibake in Icons/Thumbnails/Tiles labels

With the crash fixed, live testing exposed a follow-on defect previously
masked by the crash: `SplitText` and `TruncateSringToFitWidth`
(`fileswn4.cpp`) split/ellipsize the UTF-8 name at **byte** indices. A cut
landing inside a multi-byte sequence produced invalid UTF-8, forcing the
wide-drawing code into its documented byte-wise fallback → mojibake labels
(observed live in Icons/Thumbnails view of the repro directory; screenshot
in validation-results.md).

Fixed by snapping every cut index back to a UTF-8 sequence boundary
(`SnapToU8Boundary`, `fileswn4.cpp`) in: `SplitText` line-1 ellipsis, line-2
ellipsis, line-2 full-fit (also gained the previously missing caller-buffer
cap), first-line space split, first-line full-fit cap, and
`TruncateSringToFitWidth` (incl. the 2-char degenerate case). Two latent
`DrawItemAlpDx[-1]` OOB reads in the ellipsis width math were fixed by the
same restructure. Byte-wise *measurement* of multi-byte text (ANSI metrics)
still over-estimates widths — a cosmetic centering offset in icon-mode
labels, documented as a known limitation (proper fix = wide-unit
measurement, deferred).

## R7 — Pre-existing build blocker fixed en route: exif.dll LNK2005

`build.cmd` failed on this machine (both configs) in
`plugins\pictview\...\exif.dll`: `LNK2005 __ucrt_int_to_float already
defined`. Root cause: Windows SDK **10.0.26100** newly defines
`inline float __ucrt_int_to_float(...)` in `corecrt_math.h` for C
translation units (SDK 22621 does not have it), and MSVC emits plain
`inline` C functions as **non-COMDAT externals** in every C TU including
`math.h` — libexif has 27 such TUs. Neither `/std:c17` nor `/Gy` changes
the emission (verified with `dumpbin /symbols`: `External | __ucrt_int_to_float`,
no selectany). The user's earlier Release builds passed only because a
pre-SDK-update `exif.dll` already existed (incremental skip). Fix:
`/FORCE:MULTIPLE /IGNORE:4006` on the exif link (`exif_base.props`) — the
duplicates are byte-identical UCRT helpers, keeping the first is safe; the
rationale is in a comment at the site. Unrelated to the 031 defect class but
required for the "builds clean" verification gate.

## R6 — Historic context used (features 004, 005, 010–015, 027)

- Internal contract: names are UTF-8 in `char*` plumbing; convert at OS
  boundary via `SalU8ToW`/`SalWToU8`; `\\?\` + W-APIs (`salunicode.h`,
  `salpath.h`, `salfileio.h`).
- `SAL_FIND_NAME_U8 = 3 * MAX_PATH` (`salfileio.h:31`) is the established
  single-component worst case; `SAL_MAX_PATH_UTF8 = 98302` the full-path one.
- 027 completed the 014 audit with verdict vocabulary CRASH / BOUNDED /
  COMPONENT / EXTERNAL / FIXED — this feature's inventory (R2) reuses it.
- 027's audit closed 31 CRASH sites but its idiom scan keyed on
  `AlterFileName` and path-copy routes; the `memmove + DWORD-terminate`
  icon-lookup idiom and unbounded ext-lowercase loops form the residual
  class fixed here.
