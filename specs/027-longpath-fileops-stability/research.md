# Research: Long-Path & Unicode File-Operation Stability Revision (027)

**Date**: 2026-07-21 | **Method**: 4 parallel investigation agents — clipboard
pipeline map, F5 engine audit, feature 004–015 history, Release crash-dump
forensics — consolidated here. All file:line references verified on branch
`027-longpath-fileops-stability` (= main + spec commit).

## R1 — Fresh crash root causes (dump-confirmed, both still on main)

Two fresh WER minidumps (`%LOCALAPPDATA%\CrashDumps`) newer than the 014
fixes, symbolicated against commit-matched rebuilt Release PDBs (established
011/013/014 method; PE-timestamp → commit identification; artifacts kept in
session scratchpad):

**D1 — `salamand.exe.23524.dmp` (2026-07-20 20:25, commit 9c7eedc): Alt+F1
Change Drive menu in a long Unicode directory.**
`CDrivesList::CDrivesList` does `lstrcpy(CurrentPath, currentPath)` at
`src/drivelst.cpp:1069` into `char CurrentPath[MAX_PATH]` (`drivelst.h:110`);
the very next member is `TDirectArray<CDriveData>* Drives` (`drivelst.h:111`).
`CFilesWindow::ChangeDrive` (`fileswn3.cpp:2606→2609`) passes `GetPath()` —
the ~570-byte UTF-8 test-tree path overruns the member array, overwrites
`Drives` with path text (RCX=0xBEC599C58DC4A1C5 = UTF-8 "ščřž"), AV in
`TDirectArray::Add` (`array.h:763`) + smashed GS cookie →
0xC0000409 fast-fail, uncatchable (bug reporter never runs). `lstrcpy` is a
kernel32 API — the project's `_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES`
overload does not bound it. The drive-bar site (`toolbar6.cpp:72`) passes
`""` — safe; only the menu path crashes.
**Decision**: widen `CDrivesList::CurrentPath` to `SAL_MAX_PATH_UTF8` with a
bounded copy (`lstrcpyn`-style `_TRUNCATE`); `CDrivesList` instances are
stack temporaries in two non-reentrant UI frames (ChangeDrive, drive bar) —
a ~96 KB object is within the 1 MB-stack convention (010 §4 rule 4).
Truncation beyond 96 KB is impossible (OS max), so bounded copy is belt-and-
suspenders.

**D2 — `salamand.exe.21408.dmp` (2026-07-18 12:31, commit 47cb05a): F4
external editor / file-history reopen of a file in the long directory.**
`ExecuteExpFullPath2` does `strcpy(data->Buffer, data->Name)` at
`src/execute.cpp:878` (current main also `:794`), `CExecuteExpData::Buffer`
is `char[MAX_PATH]` (`execute.cpp:580`). The safe-CRT overload turns this
into `strcpy_s(Buffer, 260, …)` → CRT invalid-parameter fast-fail
(`_invoke_watson`) for a >259-byte path. Chain: `EditFile`
(`fileswn5.cpp:1397`) → `ExpandInitDir` (`execute.cpp:1910`) →
`DoExpandVarString` (`salamdr2.cpp:825`) → callback. Sibling callbacks
(`ExecuteExpPath` `:607` etc.) write the same buffer.
**Decision**: widen `CExecuteExpData::Buffer` to `SAL_MAX_PATH_UTF8` and
bound every writer with `_snprintf_s(_TRUNCATE)`. The external editor
*command line* remains OS-capped (~32K) — already on the external-limit
list; the internal expansion buffer must not be the thing that dies.

**Note on the user's report**: the fatal crashes attributed to "copying"
fire on Alt+F1 and F4 while the panel sits in the long directory; stale
paste/drop frames on the 23524 stack show clipboard operations completed
earlier. Both crashes reproduce the same defect class (fixed member buffer ←
panel path) and are in scope as US2.

## R2 — Why Ctrl+C fails to copy OUT of long directories

Copy/Cut does **not** build the clipboard data itself — it delegates to the
shell: `ClipboardCopy/Cut` (`fileswn9.cpp:522/528`) → `ShellAction`
(`shellsup.cpp:1676`) → `CreateIContextMenu2` + `InvokeCommand("copy"/"cut")`
(`shellsup.cpp:1695-1711`) → the shell builds CF_HDROP from PIDLs.
PIDL construction (`GetItemIdListForFileName`, `shellib.cpp:1415`) hands a
plain wide path (heap `SalU8ToWAlloc`, `:1512`) to
`IShellFolder::ParseDisplayName` (`:1538`) **without** the `\\?\` prefix —
for >260 paths the shell parser rejects it → PIDL NULL → no data object →
**nothing lands on the clipboard, silently** (version/provider dependent —
"sometimes works").

**Decision**: length-gated dual route. When the panel path and all selected
full names are < MAX_PATH, keep today's shell-verb route unchanged (zero
regression, richest interop). When any full name ≥ MAX_PATH, build
**Salamander's own IDataObject** carrying a wide (`fWide=1`) DROPFILES /
CF_HDROP block with full long paths (the format itself has no length limit),
plus `CFSTR_PREFERREDDROPEFFECT` and the existing `SALCF_IDATAOBJECT` marker
(`SetClipCutCopyInfo`, `shellsup.cpp:704`) so that paste selects the
long-path-capable route B (own engine). Explorer can *read* wide CF_HDROP;
whether it can paste >260 targets is Explorer's own limit (documented,
external-limit list).
**Alternatives rejected**: `\\?\` into `ParseDisplayName` /
`SHCreateItemFromParsingName` — shell parsing of extended-length names is
undocumented/inconsistent across builds and still ends in a shell-owned data
object with MAX_PATH internals; always-own data object — changes sub-260
interop behavior (Constitution II).

## R3 — Why Ctrl+V fails INTO long directories (and from Explorer)

`ClipboardPaste` (`fileswn9.cpp:107`) routes three ways:
- **Route A** (fake object, archive/FS): `SalShExtPastedData` engine.
- **Route B** (CF_HDROP + `SALCF_IDATAOBJECT` marker, `:195/:226`): own
  engine — `CImpDropTarget::Drop` (`shellib.cpp:967`) → `TryCopyOrMove`
  (`:303`) → `ProcessClipboardData` (`:220`, parses DROPFILES honoring
  `fWide`, heap `CCopyMoveRecord` UTF-8 names) → `DoCopyMove`
  (`shellsup.cpp:139`) → `DropCopyMove` (`fileswn6.cpp:1038`) → the F5
  engine. **This route is already long-path clean** (013/014 widenings:
  `CurDir`/`OldDataObjectSrcFSPath`/`DropPath`/`TargetPath` etc. all
  `SAL_MAX_PATH_UTF8`).
- **Route C** (foreign data, no marker — e.g. copied in Explorer):
  `InvokeCommand("paste")` with `lpDirectory = GetPath()`
  (`fileswn9.cpp:297-319`) — hands a long target dir to the shell's
  MAX_PATH-bounded paste → fails inside the shell.

**Decision**: length-gated takeover of route C. When the clipboard holds
real disk paths in CF_HDROP **and** (target dir ≥ MAX_PATH or any source
path ≥ MAX_PATH), parse CF_HDROP ourselves and run route B's engine
(`ProcessClipboardData` → `DoCopyMove`), honoring
`CFSTR_PREFERREDDROPEFFECT` for copy-vs-move. Otherwise route C stays
byte-for-byte unchanged (sub-260 pastes of Explorer data keep today's shell
behavior — Constitution II). `CFSTR_FILECONTENTS`-only data (virtual files:
Outlook, zips) stays on the shell route always; Paste Shortcut
(`CM_CLIPPASTELINKS`) stays shell (`IShellLink` is on the external-limit
list).

## R4 — Remaining F5/F6/F7 gaps in the in-program engine

The byte-copy core is long-path/Unicode correct: script records are heap
`char*` (`COperation`, `worker.h:216`), one shared 320 KB copy buffer
allocated once (`StartWorker`, `worker.cpp:8359`; async 8×1 MB reused,
`worker.cpp:875-887`), all file I/O through `Sal*` wrappers →
`SalPathToWExtAlloc` (`\\?\`, both source and target). The failures are
concentrated at:

1. **`fileswn6.cpp:1587`** — `BuildScriptDir` **source** gate
   `… >= MAX_PATH - 2` → hard "too long" reject when a *folder*'s source
   path reaches ~258. Files go through `BuildScriptFile`→`BuildName`
   (gated at `SAL_MAX_PATH_UTF8`, `salamdr1.cpp:991`) — hence the user's
   "F5 works in some cases" (files yes, folders no). Stale gate, untouched
   by 012/014. **Fix: gate at `SAL_MAX_PATH_UTF8`.**
2. **`fileswn6.cpp:1655`** — `BuildScriptDir` **target** gate
   `… >= PATH_MAX_PATH` (=248, `spl_gen.h:293`) → folder copy INTO a long
   directory rejected. **Fix: gate at `SAL_MAX_PATH_UTF8`.** (`PATH_MAX_PATH`
   itself is plugin ABI — do not touch the constant.)
3. **`salamdr5.cpp:968`** — ANSI `CreateDirectory(newDirs, NULL)` creating
   typed/pasted intermediate target dirs (buffer already
   `SAL_MAX_PATH_UTF8`, call still `-A`). **Fix: `SalCreateDirectory`.**
4. **F7 Create Directory** — `fileswn5.cpp:1986/1988` gate at
   `PATH_MAX_PATH` + `IDS_TOOLONGPATH`; dialog/aux buffers `:1963`
   (MAX_PATH), `checkPath[MAX_PATH]` `:2003`, `newDir[MAX_PATH]` `:2009`.
   (Also the documented 012 follow-up.) **Fix: widen + SAL gates.**
5. **Copy-security/attrs sub-routes ANSI**: `GetNamedSecurityInfo`/
   `SetNamedSecurityInfo` (`worker.cpp:1505-1620`, `:5644`),
   `EncryptFile`/`DecryptFile` (`worker.cpp:1793/1812/3149, 1852/1871/3158`)
   — option-dependent intermittent failures on long/Unicode paths.
   **Fix: W variants via `SalPathToWExtAlloc`** (note:
   `*NamedSecurityInfoW` accepts `\\?\`; `EncryptFileW` needs plain-wide
   fallback test — degrade with clear message if the API refuses the
   prefix).
6. **`worker.cpp:7295`** — `tmpFileName[MAX_PATH]` in `DoConvert` → convert
   fails in long dirs. **Fix: widen.**
7. `SalGetFileSize2` ANSI `CreateFile` (`salamdr5.cpp:1446`) — check
   callers; widen to `SalCreateFile` for consistency.

Recycle-bin gates (`worker.cpp:6293/6298/6935` + ANSI `SHFileOperation`)
stay: documented external limit.

## R5 — Drag&drop and archive/FS clipboard residue

- **`shellsup.cpp:477`** — `GetCurrentDir` subdir gate
  `l + NameLen >= MAX_PATH` blocks drag-drop **onto a subdirectory item**
  when the composed target ≥260 ("too long file name!"), even though the
  engine downstream handles it. **Fix: compose on heap, gate at
  `SAL_MAX_PATH_UTF8`.** (Archive variant `:310` stays bounded — archive
  paths are on the external/ABI list.)
- **`shellib.cpp:445`** — `IsSimpleSelection` `prefixBuf[MAX_PATH]` (wide)
  clamps the common-prefix compare (`:502-503/:611-612`) → long selections
  wrongly classified "not simple" → arc/FS paste refused. **Fix: heap
  prefix.**
- **`SalShExtSharedMem::TargetPath[2*MAX_PATH]`** (`shexreg.h:218`; writes
  `shellib.cpp:1238` bounded to MAX_PATH, `:1316` to 2*MAX_PATH) — shared-
  memory ABI with the registered `salextx64.dll` (Explorer-side). Truncation
  today means the shell extension operates on a **wrong target path**
  (data-loss / undefined behavior risk on the archive-extract paste route).
  **Decision: keep the ABI (an old registered DLL in Explorer must not be
  handed a resized struct); add explicit pre-checks at both write sites —
  if the target does not fit, fail that archive/FS paste with a clear
  bounded message and add the case to the external-limit list.** Widening
  the IPC struct is deferred to a future shellext-versioned feature.
- `CSalShExtPastedData::ArchiveFileName/PathInArchive[MAX_PATH]`
  (`salshlib.h:176/177`), `CTmpDragDropOperData` members (`fileswnd.h:68-69`,
  `lstrcpyn` at `shellsup.cpp:199-200`), `userPart[MAX_PATH]`
  (`fileswn9.cpp:485`), archive drag `targetPath/realDraggedPath[2*MAX_PATH]`
  (`shellsup.cpp:1266/1268`) — archive/FS-side names; bounded copies today.
  Verdict per audit pass G/H: bound-verify + clear-message gates (archives
  in long dirs are opened via the panel path; full widening of the archive
  subsystem is out of 027 scope unless the audit proves a CRASH verdict).

## R6 — Performance (SC-005)

- Copy-buffer strategy is already sound (single 320 KB buffer, async pool
  reused) — no per-file allocation on the byte loop. No change.
- The measurable long-path overhead is `SalPathToWExtAlloc` per `Sal*` call
  (~3–5× per copied file): two mallocs + `MultiByteToWideChar` + an
  **unconditional** `SalCanonicalizePathW` (`salpath.cpp:224→293`) that
  collapses `.`/`..`/duplicate separators even for already-clean absolute
  paths. **Decision: add a cheap pre-scan — if the UTF-8 input contains no
  `/`, no `.` path segment, and no doubled separator, skip canonicalization
  entirely.** Keep allocations (correctness first); prefix caching across a
  directory's files is a possible future optimization, not needed for the
  ≤10% SC-005 bar (canonicalization is the dominant avoidable term).
- Verification: harness timing — copy a fixed file set (e.g. 500 × 4 KB +
  5 × 50 MB) ordinary→ordinary vs long→long on the same volume; assert the
  long-path run ≤ 1.10 × ordinary.

## R7 — Completing the 014 exhaustive audit (FR-008)

Feature 014's program-wide audit was **never executed**: tasks T005/T006
blocked mid-session; `specs/014-longpath-systematic-sweep/audit/` contains
only `_BRIEF.md` (methodology + verdict vocabulary: CRASH / BOUNDED /
COMPONENT / EXTERNAL / FIXED); no `A.md`–`H.md` exist; research R4 is an
interrupted stub. The census stands at ~764 fixed-size path buffers, ~90+
on path-copy routes across ~30 core files.
**Decision**: 027 executes the audit to completion using 014's exact
methodology and vocabulary — 8 parallel subsystem passes (A viewer, B
copy/move/delete/pack engine+workers, C file-op UI, D navigation/
change-notify/directory-line, E core path/error primitives, F main window/
persistence/history, G shell/clipboard/drag, H dialogs/find/drive/plugin
handoff) writing `specs/027-…/audit/A.md`–`H.md`, consolidated into a single
inventory (`audit/INVENTORY.md`) with per-site verdicts; every CRASH verdict
fixed in this feature; a repeatable static check (script grepping the
enumerated sites + verdict cross-check) proving zero unresolved CRASH sites.
The R1 crashes (drivelst, execute.cpp) become audit rows fixed up front.
Bundled-plugin secondary pass (014 T012) stays deferred — core-app scope
here, recorded as such in the inventory.

## R8 — External-limit list (carried forward, single source of truth)

Unchanged entries (012/013/014): `ShellExecute(Ex)` command line;
`IShellLink` (.lnk) resolution + Paste Shortcut; `SHFileOperation`
Recycle-Bin delete; shell "New" menu; working-directory inheritance for
launched processes (OS caps cwd at MAX_PATH); shell-extension IPC
`TargetPath` (fixed ABI — now with explicit refusal, R5). New entries from
027: Explorer-side paste of our long-path clipboard data (Explorer's own
limit); `EncryptFile`/`DecryptFile` if the W-API refuses `\\?\` (verify
during implementation). The consolidated list lives in
`audit/INVENTORY.md` §External.

## R9 — Verification strategy

- **Builds**: Debug x64 + Release x64 clean (`build.cmd` / `build.cmd
  release`), zero new warnings in changed files.
- **saltests**: extend the existing test project with unit tests for the
  new pure logic where linkable (DROPFILES wide-block build/parse
  round-trip with >260 Unicode paths; canonicalization-skip pre-scan;
  gate arithmetic).
- **Static exhaustion check**: scripted re-scan of the audit inventory
  (R7) — zero unresolved CRASH sites.
- **Scripted end-to-end where headless allows**: file-system-level
  verification of engine outcomes via the test tree (sizes/names compare)
  where drivable; full GUI walkthrough (Alt+F1, F4, Ctrl+C/V matrix, F5/F6
  matrix) remains the user's follow-up, per spec assumption.
- **Timing harness** for SC-005 (R6).
- **Dump forensics**: if any new crash appears during the user's
  walkthrough, the commit-matched PDB rebuild method from this session's
  scratchpad is the tool.

## Decisions summary

| # | Decision |
|---|----------|
| 1 | Fix D1 `CDrivesList::CurrentPath` and D2 `CExecuteExpData::Buffer` by widening to `SAL_MAX_PATH_UTF8` + bounded copies |
| 2 | Clipboard copy-out: length-gated own wide-CF_HDROP IDataObject (≥MAX_PATH), shell verb unchanged below |
| 3 | Clipboard paste-in: length-gated own-engine takeover of foreign CF_HDROP; virtual-file & PasteLinks stay shell |
| 4 | F5/F6: raise `BuildScriptDir` source/target gates to `SAL_MAX_PATH_UTF8`; F7 widen; ANSI `CreateDirectory`/security/encrypt → W/Sal wrappers; `DoConvert` widen |
| 5 | Drag subdir gate + `IsSimpleSelection` prefix → heap/SAL; shellext IPC kept ABI-stable with explicit refusal |
| 6 | Perf: canonicalization pre-scan skip; timing harness proves ≤10% |
| 7 | Execute 014's 8-pass audit to completion in `specs/027-…/audit/`, fix all CRASH verdicts, static re-check |
| 8 | External-limit list consolidated in `audit/INVENTORY.md` |
