# Source Code Analysis: Long Path & Unicode Support

**Feature**: [spec.md](spec.md)
**Created**: 2026-07-13
**Purpose**: Evidence base for the specification and input for the
`/speckit.plan` phase. Documents where and how deep the ANSI/MAX_PATH
assumptions run in the current codebase.

## A. Compilation model — the core is ANSI (MBCS)

- The main project `src/vcxproj/salamand.vcxproj` has **no**
  `<CharacterSet>` element → MSBuild default "Not Set" (multi-byte /
  ANSI). The only Unicode project in the tree is the auxiliary trace
  server (`src/vcxproj/tserver/tserver.vcxproj:26,33`).
- `src/vcxproj/sal_base.props:16` defines `WINVER`/`_WIN32_WINNT` but
  never `UNICODE`/`_UNICODE`. Confirmed by
  `architecture/07-preprocessor-defs.md` (effective-definitions
  summary): `UNICODE` absent from every core configuration.
- `sal_base.props:14` sets `/J` globally (plain `char` is unsigned) —
  relevant for byte comparisons.
- Consequence: `TCHAR` = `char`; every un-suffixed Win32 call
  (`FindFirstFile`, `CreateFile`, `WIN32_FIND_DATA`, `CompareString`,
  …) resolves to the **A** variant. Filenames are `char*` everywhere;
  the shared panel item structure `CFileData` uses `char* Name`
  (`src/plugins/shared/spl_com.h:205`).

## B. Directory enumeration and file opening — all ANSI A-variants

- Panel directory reading `CFilesWindow::ReadDirectory`:
  `src/fileswn3.cpp:293-295` — `WIN32_FIND_DATA` (= `…A`) +
  `FindFirstFile(fileName, …)` on a `char` buffer
  (`char buf[2 * MAX_PATH + 100]`, `fileswn3.cpp:279`).
- ~20 `FindFirstFile` sites across panel/operation files:
  `fileswn2.cpp:346`, `fileswn3.cpp:295/870/1716/…`,
  `fileswn5.cpp:255/2194`, `fileswn6.cpp:326/2077/…`,
  `fileswn7.cpp:835`, `fileswn8.cpp:1179`, `fileswna.cpp:376` — all
  A-variant with `char*` buffers.
- File-operations worker: `src/worker.cpp` (121 file-API call sites —
  densest single file).
- Central safe-open wrapper `src/safefile.cpp:46` **hard-rejects**
  long names:
  ```cpp
  hFile = fileNameLen >= MAX_PATH ? INVALID_HANDLE_VALUE
                                  : HANDLES_Q(CreateFile(fileName, ...));
  ```
  A fallback (`safefile.cpp:152-199`) renames overly-long targets to
  an 8.3 tmp-name and back — a workaround, not a long-path solution.
- Path resolution `SalGetFullName` (`src/salamdr3.cpp:412`) operates
  on `char* name` with explicit `MAX_PATH - 1` bounds checks
  (`salamdr3.cpp:443`).

## C. MAX_PATH buffers — pervasive; only vestigial `\\?\` support

- `MAX_PATH` appears **4,881 times across 441 files** in
  `src/**/*.{cpp,h}`; core files dominate. Highest counts:
  `fileswn5.cpp` (100), `salamdr2.cpp` (99), `fileswn2.cpp` (94),
  `fileswn6.cpp` (85), `salamdr1.cpp` (67), `drivelst.cpp` (63),
  `mainwnd3.cpp` (62), `worker.cpp` (62), `plugins2.cpp` (57),
  `salamdr3.cpp` (55), `dialogs3.cpp` (55), `fileswn3.cpp` (51),
  `fileswn7.cpp` (51), `shellib.cpp` (45), `shellsup.cpp` (42).
- Panel data structures embed fixed buffers (`src/fileswnd.h`):
  `char Path[MAX_PATH]` (:478), `char ZIPPath[MAX_PATH]` (:489),
  `char TargetPath[MAX_PATH]` (:60), `char Path[2 * MAX_PATH]` (:362).
- `CFileData::NameLen` is a **9-bit bitfield**, commented
  "POZOR: maximalni delka jmena je (MAX_PATH - 5)"
  (`src/plugins/shared/spl_com.h:218`).
- Only `\\?\` generator: `DoLongName` (`src/worker.cpp:2129`), used
  solely by the ADS-copy path (`worker.cpp:2360-2361`) — and even
  there into a `char longSourceName[MAX_PATH + 100]` ANSI buffer
  converted to wide via `CP_ACP`. Other spots merely *detect* the
  prefix: `salamdr2.cpp:1105`, `shiconov.cpp:130-134`,
  `salamdr3.cpp:428` (comment: such paths "are simply not supported
  here"). No general long-path plumbing exists.

## D. Existing Unicode conversion layer — small, isolated, unused on the filename path

- `src/common/strutils.cpp`: `ConvertU2A` (:12, wraps
  `WideCharToMultiByte`), `ConvertAllocU2A` (:57), `ConvertA2U`
  (:121). Used only sparsely: `drivelst.cpp:1383-1384,1481` (OneDrive
  UTF-8 path parsing), `fileswn9.cpp:642`, `jumplist.cpp` (commented
  out). Not part of the main filename pipeline.
- Where the core does call wide APIs, it round-trips through `CP_ACP`
  first, destroying anything not representable in the active
  codepage, e.g. `worker.cpp:2362-2364`:
  ```cpp
  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, longSourceName, -1, srcName, 2*MAX_PATH)
  ```
- **Plugin API is entirely ANSI**: `spl_com.h`, `spl_gen.h`,
  `spl_fs.h`, `spl_gui.h` expose only `char*` structures/functions
  (`CFileData::Name/Ext/DosName`, `spl_com.h:205-215`). No wide
  variants. A Unicode migration touches the plugin ABI.

## E. Unicode normalization — one dead experiment

- The only normalization code is a commented-out test block in
  `src/common/strutils.cpp:283-360` that explicitly wrestles with NFD
  (e.g. `L"D:\\\x0061\x0308"` — "a" + combining diaeresis) and
  ligatures, including `FoldString(MAP_FOLDCZONE | MAP_PRECOMPOSED, …)`
  at :324-325. Dead code; surrounding notes treat the problem as
  unsolved.
- No `NormalizeString` usage anywhere in the core; no active NFC/NFD
  handling. The `MB_PRECOMPOSED` flag on ANSI→wide conversions is the
  only live "precompose" attempt and is gated behind `CP_ACP`.

## F. Filename comparison / sorting — byte-wise ANSI or CompareStringA

- All comparison entry points take `const char*` — `src/sort.cpp`.
  Panel sort key `CmpNameExt` / `CmpNameExtIgnCase` (`sort.cpp:266,287`)
  → `RegSetStrICmpEx` on `char*` names (`sort.cpp:284,319`).
- `RegSetStrICmp*` (`sort.cpp:177-259`) branch on config:
  `CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, …)` (=
  `CompareStringA`, `sort.cpp:187`) or byte-wise `StrICmp`.
- NFC vs NFD names compare unequal even when visually identical — and
  names outside CP_ACP never reach the panel intact in the first
  place. `shellib.cpp:450` uses `CompareStringW` but only after a
  per-item ANSI→wide conversion, inheriting the CP_ACP loss.

## G. Find / viewer / file-ops — shared path helpers (leverage point)

- Find, internal viewer, and file operations share the core path
  helpers declared in `src/consts.h` (`SalPathAppend:291`,
  `SalPathAddBackslash:295`, `SalPathStripPath:305`,
  `SalRemovePointsFromPath:340`, `SalGetFullName:357`), defined in
  `src/salamdr3.cpp:22-412`. Call sites across `find.cpp`,
  `finddlg1.cpp`, `finddlg2.cpp`, `viewer.cpp`, `viewer2.cpp`,
  `viewer3.cpp`.
- Every helper is `char*`/MAX_PATH-based — fixing them touches all
  three subsystems at once (both risk and leverage).
- Find keeps ANSI per-item buffers: `CFoundFilesData { char* Name;
  char* Path; }` (`src/find.h:496-499`).
- The sole wide-path helper in the core: a `WCHAR*` overload of
  `SalRemovePointsFromPath` (`salamdr3.cpp:365`).

## Overall assessment

The ANSI/MAX_PATH assumptions run to the bottom of the architecture
and are not confined to a thin I/O layer. Filenames are `char*` from
the OS boundary (`FindFirstFileA`/`CreateFileA`) through the panel
model (`CFileData::Name`, 9-bit `NameLen` capped at MAX_PATH-5), the
fixed `char Path[MAX_PATH]` panel buffers, the shared `Sal*` path
utilities, sorting/comparison, and out to the entire plugin ABI.
MAX_PATH is baked into ~4,900 sites; the central safe-open wrapper
actively rejects long names; the only extended-path (`\\?\`) code is a
single-purpose ADS helper still bounded by an ANSI MAX_PATH buffer and
routed through `CP_ACP`. A conversion layer and one dead `FoldString`
experiment show the original authors knew about the NFD problem, but
nothing on the live filename path uses them — any character outside
the active ANSI codepage is destroyed at the OS boundary before the
application ever sees it.

**Implication for planning**: both fixes require a staged wide-string
(UTF-16) migration of the core filename model plus systematic
replacement of MAX_PATH buffers and a plugin-ABI compatibility
strategy — not a localized patch. The shared `Sal*` path-helper layer
and the single `ReadDirectory`/worker choke points are the natural
staging areas. This matches the spec's phased-scope assumption
(core first, plugin boundary degrades gracefully per FR-012).

---

# Supplement: plan-phase recon (mechanics for research.md/contracts)

## H. Plugin interface versioning mechanics

- `src/plugins/shared/spl_vers.h:195` — `#define LAST_VERSION_OF_SALAMANDER 103` (5.0);
  `:102` `VERSINFO_BUILDNUMBER 183`; `:122-193` documents the scheme;
  `:196` `REQUIRE_LAST_VERSION_OF_SALAMANDER`; change procedure referenced
  as `doc\how_to_change.txt` (`spl_vers.h:143`).
- `src/plugins.h:8` — `#define PLUGIN_REQVER 103` (core's accepted floor).
- Exports (`src/plugins/shared/spl_base.h`): `SalamanderPluginEntry`
  (:812), `SalamanderPluginGetReqVer` (:824, called first),
  `SalamanderPluginGetSDKVer` (:839, optional);
  `CSalamanderPluginEntryAbstract::GetVersion` (:718).
- Negotiation (`src/plugins1.cpp` `CPluginData::InitDLL`): resolve entry
  :2193; `BuiltForVersion = GetReqVer()` :2226-2239 (raised to SDKVer if
  present); too-old gate `BuiltForVersion < PLUGIN_REQVER` :2240 → skip
  entry + `IDS_OLDPLUGINVERSION` box :2280-2306. **No too-new gate.**
- v-next bump = raise `LAST_VERSION_OF_SALAMANDER`; keep `PLUGIN_REQVER`
  at 103 so legacy plugins still load (shim applies); `BuiltForVersion`
  tells the core per-plugin which semantics to use.

## I. Application manifest

- Linker manifest disabled: `src/vcxproj/sal_base.props:9-10`
  (`GenerateManifest=false`, `EmbedManifest=false`).
- Embedded as resource: `src/salamand.rc2:12` →
  `CREATEPROCESS_MANIFEST_RESOURCE_ID RT_MANIFEST "manifest.xml"`.
- `src/manifest.xml`: comctl32 v6 (:12-19), asInvoker (:25-28),
  supportedOS (:34-38), legacy `dpiAware=true` (:43).
  **`longPathAware` and `activeCodePage` absent.**

## J. Config persistence chokepoint

- `src/regwork.cpp` — `SetValueAux` :215 (`RegSetValueExA`, strings as
  `REG_SZ`, `dataSize==-1` → `strlen+1` :213-214), reads via
  `SalRegQueryValueEx` :163/205/249 (declared `src/consts.h:2439`,
  plugin-exposed `spl_gen.h:3321`), delete :241. All string config
  (e.g. history `salamdr3.cpp:2117/2141`) flows through this façade.

## K. Panel rendering & measurement sites

- Draw (`src/fileswn4.cpp`, `DrawBriefDetailedItem` :467 +
  icon/thumbnail/tile): `ExtTextOutA` at :744, :749, :770, :850, :880,
  :892, :909, :935, :1508, :1515, :1896, :1911, :1922.
- Measure (`src/fileswn2.cpp` layout pass): `GetTextExtentPoint32A` at
  :3539, :3548, :3661, :3728, :3739, :3747, :3776, :3800, :3837,
  :3848, :3860, :3865, :3901, :3920, :3927, :4008.
- Caveat: code equates `NameLen`/`strlen` bytes with drawn character
  counts — breaks under UTF-8; W conversion must happen at these sites.

## L. Quick search input

- `CFilesWindow::OnChar` `src/fileswn0.cpp:879` (panel `WM_CHAR`);
  byte-gated `wParam > 32 && wParam < 256` :894 (Alt-search :1021);
  append `(char)wParam` :931; buffers `char QuickSearch[MAX_PATH]`,
  `QuickSearchMask[MAX_PATH]` (`src/fileswnd.h:832-833`); byte-wise
  mask compare `PrepareQSMask`/`AgreeQSMask` :53/:76-78; backspace via
  `strlen/strcpy` :1387-1411. Inline rename `WM_CHAR`:
  `src/fileswn5.cpp:2870` (`CQuickRenameWindow`).

## M. Window classes — dormant W scaffolding

- All core windows ANSI: `src/common/winlib.cpp:491,496`
  (`RegisterClassA`), `:133` (`CreateWindowExA`).
- **Dormant W path already exists** (compiled, unused):
  `winlib.cpp:499-521` `RegisterClassW` + `CWindowClassW`
  (`CWINDOW_CLASSNAMEW`, `:37`), `CreateExW/CreateW` :181-232,
  `CWindowProcW` :350, `unicode ? DefWindowProcW : DefWindowProcA`
  :468. No core caller uses it — natural foundation for R7.

## N. Viewer file access

- Direct `CreateFileA` (HANDLES_Q-wrapped, no helper, no `\\?\`):
  `src/viewer2.cpp:359`, `:504`, `:765`; write side
  `src/viewer3.cpp:1602`.

## O. String/allocation infrastructure

- Hand-rolled helpers: `src/common/str.cpp` `DupStr` :76
  (malloc+memcpy), `DupStrEx` :91, `StrNCat` :101; containers
  `TDirectArray`/`TIndirectArray` (`src/common/array.h:65/:218`;
  plugin copy `arraylt.h`).
- `CFileData::Name` allocation: core uses plain `malloc`/`free`
  (`fileswn3.cpp:489,546,585`, `fileswn7.cpp:952-1195`,
  `cache.cpp:36/81`); plugins must use
  `CSalamanderGeneralAbstract::Alloc/Free` (`spl_com.h:206,216`).
