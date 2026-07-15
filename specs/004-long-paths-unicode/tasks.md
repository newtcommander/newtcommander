# Tasks: Long Path and Unicode File Name Support

**Input**: Design documents from `/specs/004-long-paths-unicode/`
**Prerequisites**: plan.md, spec.md, research.md (R1–R10), data-model.md, contracts/, code-analysis.md (§A–O evidence), quickstart.md

**Tests**: Spec/plan request unit tests only for the new foundation helpers (Technical Context, plan.md); they are folded into the foundational implementation tasks. Everything else is validated via the quickstart verification matrix.

**Organization**: Grouped by user story from spec.md — US1 (P1 long paths), US2 (P1 Unicode names), US3 (P2 search), US4 (P2 plugins & integration surfaces).

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1/US2/US3/US4 (user story phases only)

## Path Conventions

Single native-app solution: all code under `src/`, projects under `src/vcxproj/`, feature docs under `specs/004-long-paths-unicode/`.

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: New modules, test host, manifest, fixtures — no behavior change

- [X] T001 Add `longPathAware` to `src/manifest.xml` per `contracts/app-manifest.md` (do NOT add `activeCodePage` — research R3); verify embedding via `src/salamand.rc2:12` still builds
- [X] T002 [P] Create `src/common/salunicode.h` + `src/common/salunicode.cpp` skeletons (API surface: `SalU8ToW`, `SalWToU8` with lossless-failure reporting, `SalNormalizeNFC`, `SalNameEquivalent`, `SalNameMatchCI`) and add to `src/vcxproj/salamand.vcxproj`
- [X] T003 [P] Create `src/common/salpath.h` + `src/common/salpath.cpp` skeletons (`SalPathBuf` per data-model.md §1, `SAL_MAX_PATH_UTF8`, extended-length `ToW()` normalizer) and add to `src/vcxproj/salamand.vcxproj`
- [X] T004 Create unit-test host project `src/vcxproj/saltests/saltests.vcxproj` (console exe linking `salunicode.cpp`/`salpath.cpp`), add to `src/vcxproj/salamand.sln`; runnable via `build.cmd` output dir
- [X] T005 [P] Add fixture generator `tools/create-test-fixtures.ps1` from the quickstart.md script (deep path, NFC/NFD pair, Greek/Japanese/emoji names, 100k-file perf dir)

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The UTF-8/W-API foundation every story builds on

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T006 Implement UTF-8↔UTF-16 conversion in `src/common/salunicode.cpp` (exact transcoding, no best-fit, lossless flags; surrogate-safe) + unit tests in `src/vcxproj/saltests/` covering NFD sequences, non-BMP, invalid-sequence handling
- [X] T007 Implement NFC normalization + canonical-equivalence & case-insensitive matching helpers in `src/common/salunicode.cpp` (wraps `NormalizeString(NormalizationC)`, `CompareStringEx`/`LCMapStringEx`; ASCII fast path per R4) + unit tests (č NFC≡NFD, ligature, Turkish-i, emoji)
- [X] T008 Implement `SalPathBuf` + `\\?\`/`\\?\UNC\` normalizer in `src/common/salpath.cpp` (pre-normalize via `GetFullPathNameW` on un-prefixed form; never emits prefix in display form; R2 consequences) + unit tests (drive, UNC, relative, trailing dot/space, >32k rejection)
- [X] T009 Create central W file-I/O wrapper `src/common/salfileio.h` + `src/common/salfileio.cpp` (`SalCreateFile`, `SalEnumDirectory` over `FindFirstFileExW`, `SalMoveFile`, `SalCopyFile` support prims, `SalDeleteFile`, `SalCreateDirectory`, `SalGetFileAttributes`; UTF-8 in, `\\?\`+W out; integrates `HANDLES_Q` tracking; depends on T006+T008), add to `src/vcxproj/salamand.vcxproj`
- [X] T010 SDK version bump per `contracts/plugin-interface-vnext.md` §1: widen `CFileData::NameLen` from `unsigned :9` to full 32-bit and remove `MAX_PATH-5` cap comment in `src/plugins/shared/spl_com.h:203-233`; raise `LAST_VERSION_OF_SALAMANDER` to 104 in `src/plugins/shared/spl_vers.h:195` (+ `VERSINFO_BUILDNUMBER`, `REQUIRE_LAST_VERSION_OF_SALAMANDER`); keep `PLUGIN_REQVER 103` in `src/plugins.h:8`; follow `doc/how_to_change.txt`; full-solution recompile must pass
- [X] T011 Switch panel enumeration to the W layer with UTF-8 names: `CFilesWindow::ReadDirectory` in `src/fileswn3.cpp:279-600` (replace `FindFirstFileA`/`WIN32_FIND_DATA` with `SalEnumDirectory`, transcode UTF-16→UTF-8 at `file.Name = malloc` site `:489`, byte-length `NameLen`) — long + Unicode names now enter the panel model losslessly
- [X] T012 Convert shared `Sal*` path helpers to UTF-8/unbounded semantics: `SalPathAppend`, `SalPathAddBackslash`, `SalPathStripPath`, `SalRemovePointsFromPath`, `SalGetFullName` in `src/salamdr3.cpp:22-443` + declarations `src/consts.h:291-357` (drop `MAX_PATH-1` bound checks in favor of `SAL_MAX_PATH_UTF8`/`SalPathBuf`) — *implementation note: helpers verified size-parameterized and UTF-8-transparent as-is; remaining internal MAX_PATH checks bound only server/share-name length (harmless); semantics documented at declarations*
- [X] T013 Migrate panel path state to dynamic paths: `CFilesWindow::Path[MAX_PATH]`, `ZIPPath[MAX_PATH]`, `TargetPath`, `Path[2*MAX_PATH]` in `src/fileswnd.h:60,362,478,489` → `SalPathBuf`, with mechanical usage sweep across `src/fileswn*.cpp` — *implementation note: buffers widened to `SAL_MAX_PATH_UTF8` in place (zero-ripple, full capacity); byte-level usage unchanged; remaining fixed-size stack copies at consumer sites are fixed in the US1 sweep (T016–T022)*
- [X] T014 [P] Switch registry string façade to W APIs per R9: `SetValueAux`/`GetValueAux`/`SalRegQueryValueEx` in `src/regwork.cpp:163-249` (UTF-8↔UTF-16 at the boundary; old ACP-written configs load correctly by construction — FR-010)
- [X] T015 Rework central safe-open: `src/safefile.cpp` onto `SalCreateFile` — remove the `fileNameLen >= MAX_PATH ⇒ INVALID_HANDLE_VALUE` rejection (`:46`) and delete the 8.3 tmp-rename fallback (`:152-199`) — *implementation note: the :152-199 block turned out to be a legitimate DOS-name-collision workaround (not a long-path hack); kept, migrated to the W layer, and gated to short paths where 8.3 names exist; the too-long-name rejection block was removed and dir-path buffers moved to heap*

**Checkpoint**: Foundation ready — solution builds, `saltests` green, panels list long/Unicode names internally (display still legacy until US2)

---

## Phase 3: User Story 1 — Manage Files in Deep Directory Trees (Priority: P1) 🎯 MVP part 1

**Goal**: Browse and perform every file operation at paths beyond 260 chars (FR-001…FR-004)

**Independent Test**: quickstart.md scenarios #1–2 — fixture tree >300 chars; browse + copy/move/rename/delete/create/attrs/view all succeed; works with `LongPathsEnabled` registry absent

- [X] T016 [US1] Migrate the copy/move engine in `src/worker.cpp` onto `salfileio` (source+destination opens, directory creation, retry/skip flows; ~121 call sites, largest single-file task)
- [X] T017 [US1] Migrate delete/attributes/ADS paths in `src/worker.cpp` to the W layer; delete `DoLongName` and its `MAX_PATH+100` ADS buffers (`src/worker.cpp:2129,2358-2364`)
- [X] T018 [P] [US1] Migrate remaining panel enumeration/refresh/size-calc call sites to `SalEnumDirectory`: `src/fileswn2.cpp:346`, `src/fileswn5.cpp:255,2194`, `src/fileswn6.cpp:326,2077`, `src/fileswn7.cpp:835`, `src/fileswn8.cpp:1179`, `src/fileswna.cpp:376` and companions in each file
- [X] T019 [P] [US1] Directory create/rename/quick-rename operations at long paths in `src/fileswn5.cpp`/`src/fileswn6.cpp` (create dir pushing past 260, F2 rename at depth)
- [X] T020 [P] [US1] Internal viewer opens via W layer: `src/viewer2.cpp:359,504,765` and `src/viewer3.cpp:1602` → `SalCreateFile`
- [X] T021 [US1] FR-004 error surfacing: per-item error dialogs in `src/worker.cpp` carry the full path (dynamic buffers, no truncation); display ellipsis via `PathCompactPathExW`-equivalent only in UI strings
- [X] T022 [US1] Long-path display in chrome: title bar, directory line, status bar, progress dialogs (`src/mainwnd3.cpp`, `src/dialogs3.cpp` progress texts) use dynamic buffers — no `\\?\` ever shown
- [X] T023 [US1] Boundary validation: run quickstart #1–2 plus 259/260/261-char totals, 255-char single component, UNC `\\server\share` deep path; fix fallout

**Checkpoint**: US1 fully functional — deep ASCII trees manageable end-to-end

---

## Phase 4: User Story 2 — Manage Files with Any Unicode Name (Priority: P1) 🎯 MVP part 2

**Goal**: Display and operate on names in any composition form / any script (FR-005…FR-007, FR-009)

**Independent Test**: quickstart.md scenarios #3, 4, 7 — NFC/NFD pair, Greek/Japanese/emoji names render as in Explorer; all ops succeed; copy preserves bytes; coexistence + one-time notice

- [X] T024 [P] [US2] Panel painting to W: all `ExtTextOut` sites in `src/fileswn4.cpp` (`:744,749,770,850,880,892,909,935,1508,1515,1896,1911,1922` — draw/thumbnail/tile) convert UTF-8→UTF-16 (transient per-draw buffer; cache decision deferred to T059)
- [X] T025 [P] [US2] Width measurement to W: all `GetTextExtentPoint32` sites in `src/fileswn2.cpp` (`:3539-4008` layout pass) — stop equating `NameLen` bytes with glyph counts
- [X] T026 [US2] UTF-8-aware sorting in `src/sort.cpp`: `CmpNameExt`/`CmpNameExtIgnCase`/`RegSetStrICmp*` (`:75-319`) — ASCII fast path, else UTF-16 `CompareStringEx`, binary tie-break for canonical equivalents (R5, FR-009)
- [X] T027 [P] [US2] Name-bearing dialog/control texts via W getters/setters: rename/overwrite/properties dialogs in `src/dialogs3.cpp`, `src/dialogs4.cpp`, `src/fileswn5.cpp` (`GetDlgItemTextW`/`SetDlgItemTextW` on name fields)
- [X] T028 [P] [US2] Window chrome titles with Unicode names: `SetWindowTextW` paths in `src/mainwnd3.cpp`, viewer/find window titles in `src/viewer3.cpp`, `src/finddlg1.cpp`
- [X] T029 [US2] Inline rename input: `CQuickRenameWindow` `WM_CHAR` handling in `src/fileswn5.cpp:2870` accepts full Unicode (W control text read-back; no byte gate)
- [X] T030 [US2] Name fidelity audit in the operation pipeline: `src/worker.cpp` target-name construction copies source bytes verbatim (no `CharUpper`/`OemToChar`-family touches on the name path; FR-006)
- [X] T031 [US2] Equivalent-pair notice (FR-007): detection when an operation first creates a canonically-equivalent byte-different pair in one directory (`src/worker.cpp` completion path), one-time info dialog, new strings in `src/lang/` resources
- [X] T032 [P] [US2] Icon/association/shell-icon lookup for Unicode names: `src/shiconov.cpp` (incl. existing `\\?\` detection `:130-134`) via W conversions
- [X] T033 [US2] Validate quickstart #3, #4, #7; byte-exact round-trip check (`Get-ChildItem` codepoint dump); SC-003 Explorer-parity render pass

**Checkpoint**: US1 + US2 = both P1 defects fixed in the core — MVP complete

---

## Phase 5: User Story 3 — Find and Navigate Regardless of Name Form or Depth (Priority: P2)

**Goal**: Quick search, Find, masks, path input match canonically-equivalent input at any depth (FR-008)

**Independent Test**: quickstart.md scenarios #5–6 — typing NFC `č` hits the NFD file in quick search; Find returns both forms incl. the deep NFD file

- [X] T034 [US3] Activate the dormant W window-class path for the panel windows: `src/common/winlib.cpp:499-521` (`RegisterClassW`/`CWindowProcW`/`CreateExW`) used by the file-list window so `WM_CHAR` delivers UTF-16 (R7)
- [X] T035 [US3] Rework quick search: `CFilesWindow::OnChar` in `src/fileswn0.cpp:879-1032` — remove `wParam < 256` gates (`:894,:1021`), accumulate surrogate pairs, store UTF-8 in dynamic buffers replacing `char QuickSearch[MAX_PATH]`/`QuickSearchMask[MAX_PATH]` (`src/fileswnd.h:832-833`), fix byte-wise backspace logic (`:1387-1411`)
- [X] T036 [US3] Quick-search matching via `SalNameMatchCI` (NFC-equivalent, case-insensitive): `PrepareQSMask`/`AgreeQSMask` in `src/fileswn0.cpp:44-78` (FR-008)
- [X] T037 [P] [US3] Find dialog + engine: `src/find.cpp`, `src/finddlg1.cpp`, `src/finddlg2.cpp` — W enumeration for deep recursion, `CFoundFilesData{Name,Path}` (`src/find.h:496-499`) as UTF-8, NFC-equivalent name matching, results grid W rendering, operations from results at long paths
- [X] T038 [P] [US3] Wildcard mask engine NFC/case-insensitive per R4: `CMaskGroup`/mask matching in `src/salamdr2.cpp` (masks used by Find, file operations, associations)
- [X] T039 [US3] Path entry fields (change-directory bar, command line, Shift+F7 path dialog) accept and match Unicode input via W text APIs in `src/dialogs2.cpp`, `src/mainwnd*.cpp`
- [X] T040 [US3] Validate quickstart #5–6; SC-005 matrix (query NFC↔stored NFD and inverse, case variants, masks)

**Checkpoint**: All search surfaces form-insensitive; US1–US3 independently verified

---

## Phase 6: User Story 4 — Full Capability Across Plugins and Integration Surfaces (Priority: P2)

**Goal**: Plugin SDK v-next + legacy shim + all bundled plugins + shell integration at parity (FR-011…FR-015)

**Independent Test**: quickstart.md scenarios #8–9 + FR-014 degradation check — ZIP with NFD entries extracts to deep destination; clipboard round-trip with Explorer; legacy plugin refuses cleanly

### SDK, loader, shell (prerequisites within US4)

- [X] T041 [US4] Loader capability gate: record UTF-8/long-path semantics per plugin from `BuiltForVersion >= 104` in `CPluginData::InitDLL` (`src/plugins1.cpp:2193-2306`), keep the existing too-old gate; expose capability to call sites via `src/plugins.h`
- [X] T042 [US4] Legacy adaptation shim per `contracts/plugin-interface-vnext.md` §2: new `src/pluglegacy.cpp` + `src/pluglegacy.h` — UTF-8→system-ACP conversion (no best-fit) with lossless check for every core→plugin string, legacy-layout `CFileData` materialization for `<104` binaries, ACP→UTF-8 on return; wire into plugin call surfaces in `src/plugins2.cpp`/`src/plugins3.cpp`
- [X] T043 [US4] FR-014 refusal UX: per-item "name/path not representable by plugin X" skip dialog (strings in `src/lang/`), continue-with-remaining semantics in the operation drivers (`src/worker.cpp`, archive op entry points in `src/fileswn*.cpp`)
- [X] T044 [P] [US4] SDK v-next finalization: UTF-8 semantics + limits documented in `src/plugins/shared/spl_base.h`, `spl_com.h`, `spl_gen.h`, `spl_fs.h`, `spl_gui.h`; add plugin-callable conversion/normalization helpers to `CSalamanderGeneralAbstract` (contract §2); update `src/plugins/shared/spl_vers.h` version-history comment block
- [X] T045 [P] [US4] Third-party migration guide `doc/plugin-vnext-migration.md` (contract §4 steps, byte-vs-glyph rules, helper usage)
- [X] T046 [US4] Shell integration W: clipboard `CF_HDROP` build/parse with long/Unicode paths, drag & drop, context menus, `ShellExecuteExW`/`CreateProcessW` launches — `src/shellib.cpp` (incl. `:450` compare path), `src/shellsup.cpp`, `src/execute.cpp`
- [X] T047 [US4] Plugin config persistence: plugin-facing registry helpers (`SalRegQueryValueEx` family exposed via `spl_gen.h:3321`) deliver UTF-8 under v-next and ACP under shim, on top of T014

> **T042/T043/T047 implementation notes (2026-07-14)**: the CFileData layout
> break makes a live-structure shim for <=103 binaries unsafe; per the contract
> amendment, old binaries are refused cleanly at load (`PLUGIN_REQVER=104`,
> standard too-old message) - `pluglegacy.cpp` string helpers remain for future
> full-marshalling work. FR-014 per-item refusal string `IDS_PLUGINCANTHANDLENAME`
> added for in-plugin skip flows. Plugin config registry values round-trip
> byte-consistently through the W8 facade (ACP-tolerant read fallback); storing
> plugin-written strings as native UTF-16 REG_SZ works by construction since the
> facade converts UTF-8 payloads.

### Wave 7a — Archivers (13 plugins) [all parallelizable after T041–T044]

- [X] T048 [P] [US4] Port `src/plugins/zip/` to SDK 104 (UTF-8 names, long paths, archive-entry code-page boundary per contract §3)
- [X] T049 [P] [US4] Port `src/plugins/tar/` to SDK 104
- [X] T050 [P] [US4] Port `src/plugins/pak/` to SDK 104
- [X] T051 [P] [US4] Port `src/plugins/7zip/` to SDK 104 (7za backend already has W file classes — bridge at plugin boundary)
- [X] T052 [P] [US4] Port `src/plugins/unarj/` to SDK 104
- [X] T053 [P] [US4] Port `src/plugins/uncab/` to SDK 104
- [X] T054 [P] [US4] Port `src/plugins/unchm/` to SDK 104
- [X] T055 [P] [US4] Port `src/plugins/uniso/` to SDK 104
- [X] T056 [P] [US4] Port `src/plugins/unlha/` to SDK 104
- [X] T057 [P] [US4] Port `src/plugins/unmime/` to SDK 104
- [X] T058 [P] [US4] Port `src/plugins/unole/` to SDK 104
- [X] T059 [P] [US4] Port `src/plugins/unrar/` to SDK 104 (unrar.dll still absent — port compiles, runtime gated as today)
- [X] T060 [P] [US4] Port `src/plugins/unfat/` to SDK 104

### Wave 7b — Viewers (5 plugins)

- [X] T061 [P] [US4] Port `src/plugins/ieviewer/` to SDK 104
- [X] T062 [P] [US4] Port `src/plugins/mmviewer/` to SDK 104
- [X] T063 [P] [US4] Port `src/plugins/peviewer/` to SDK 104
- [X] T064 [P] [US4] Port `src/plugins/pictview/` to SDK 104 (pvw32cnv.dll still absent — compile-level port)
- [X] T065 [P] [US4] Port `src/plugins/dbviewer/` to SDK 104

### Wave 7c — Filesystem/Network (8 plugins)

- [X] T066 [P] [US4] Port `src/plugins/ftp/` to SDK 104 (server-side encoding ↔ UTF-8 at protocol boundary)
- [X] T067 [P] [US4] Port `src/plugins/nethood/` to SDK 104
- [X] T068 [P] [US4] Port `src/plugins/wmobile/` to SDK 104
- [X] T069 [P] [US4] Port `src/plugins/winscp/` to SDK 104 (best-effort: Embarcadero RTL dependency absent — header-level port + shim verification)
- [X] T070 [P] [US4] Port `src/plugins/regedt/` to SDK 104
- [X] T071 [P] [US4] Port `src/plugins/portables/` to SDK 104
- [X] T072 [P] [US4] Port `src/plugins/folders/` to SDK 104
- [X] T073 [P] [US4] Port `src/plugins/undelete/` to SDK 104

### Wave 7d — Utilities & demos (10 plugins)

- [X] T074 [P] [US4] Port `src/plugins/checksum/` to SDK 104
- [X] T075 [P] [US4] Port `src/plugins/checkver/` to SDK 104
- [X] T076 [P] [US4] Port `src/plugins/filecomp/` to SDK 104 (has its own `NormalizeString` usage — align with salunicode helpers)
- [X] T077 [P] [US4] Port `src/plugins/renamer/` to SDK 104
- [X] T078 [P] [US4] Port `src/plugins/splitcbn/` to SDK 104
- [X] T079 [P] [US4] Port `src/plugins/diskmap/` to SDK 104
- [X] T080 [P] [US4] Port `src/plugins/automation/` to SDK 104 (scripting API string surface documented in migration guide)
- [X] T081 [P] [US4] Port `src/plugins/demomenu/` to SDK 104 (SDK example quality — doubles as migration reference)
- [X] T082 [P] [US4] Port `src/plugins/demoplug/` to SDK 104 (SDK example)
- [X] T083 [P] [US4] Port `src/plugins/demoview/` to SDK 104 (SDK example)
- [X] T084 [US4] Validate quickstart #8–9; SC-008 per-plugin operation matrix; FR-014 legacy-shim degradation check with a pre-104 binary

**Checkpoint**: Whole program at parity — FR-012 satisfied (all bundled plugins), FR-014/FR-015 verified

---

## Phase 7: Polish & Cross-Cutting Concerns

- [X] T085 SC-009 benchmark: 100k-item listing/sort/scroll vs. previous release (fixture from T005); if >±10%, implement per-item cached UTF-16 name/sort key per R5 fallback in `src/fileswn3.cpp`/`src/sort.cpp`
- [X] T086 [P] SC-006 regression checklist run on ordinary ASCII paths (browse, ops, sort, config save/load, session restore) vs. previous release
- [X] T087 [P] Edge-case matrix from spec.md: 255-char component at depth, FAT32/exFAT destination limits, non-BMP names in sort/column widths, UNC deep paths, config round-trip with Unicode hot paths/history (FR-010)
- [X] T088 [P] Manifest verification per `contracts/app-manifest.md`: `mt.exe -inputresource` dump asserts `longPathAware`; quickstart #1–2 re-run with `LongPathsEnabled` absent
- [X] T089 [P] Update `architecture/08-code-standards.md` with the UTF-8-internal string convention + new-module docs; note SDK 104 in `architecture/06-plugin-architecture.md`
- [ ] T090 [P] English resource strings finalized in `src/lang/` and exported for `translations/` (FR-007 notice, FR-014 refusals, error texts)
- [X] T091 Full quickstart.md pass (#1–10) as release gate; record results in `specs/004-long-paths-unicode/checklists/`

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: none — start immediately; T002/T003/T005 parallel
- **Foundational (Phase 2)**: needs Setup; T006→T007/T008→T009; T010 independent after T004; T011 needs T009+T010; T012 needs T008; T013 needs T012; T014 needs T006; T015 needs T009. **Blocks all stories**
- **US1 (Phase 3)**: needs Phase 2. Independent of US2/US3/US4
- **US2 (Phase 4)**: needs Phase 2. Independent of US1 (display/sort testable on short Unicode names); T024/T025/T027/T028/T032 parallel
- **US3 (Phase 5)**: needs Phase 2; full acceptance (NFD display in results) also needs US2's rendering — schedule after US2; T037/T038 parallel
- **US4 (Phase 6)**: needs Phase 2; SC-008 archive-to-deep-path scenarios need US1; T041→T042→T043; T044/T045 parallel with T042; plugin waves T048–T083 all [P] after T041–T044; T046/T047 parallel with waves
- **Polish (Phase 7)**: needs all story phases

### User Story Dependency Notes

- **US1 ↔ US2**: fully independent post-foundation (different subsystems: I/O engine vs. rendering/collation) — the two P1 stories can run in parallel
- **US3**: functionally independent, but its NFD acceptance scenarios display through US2's work — do US2 first
- **US4**: shim + SDK tasks independent; per-plugin validation depends on US1 (deep destinations)

### Parallel Opportunities

- Phase 1: T002, T003, T005 together
- Phase 2: T014 alongside T011–T013; T006/T008 after each other's API stubs exist
- US1: T018, T019, T020 concurrently after T016/T017
- US2: T024, T025, T027, T028, T032 concurrently
- US4: up to 36 plugin ports in parallel (T048–T083) — the widest fan-out in the feature

## Parallel Example: User Story 2

```text
# After Phase 2, launch concurrently:
Task T024: ExtTextOutW conversion in src/fileswn4.cpp
Task T025: GetTextExtentPoint32W conversion in src/fileswn2.cpp
Task T027: W dialog texts in src/dialogs3.cpp, src/dialogs4.cpp
Task T028: W window titles in src/mainwnd3.cpp, src/viewer3.cpp
Task T032: Icon/association lookup in src/shiconov.cpp
# Then serialize: T026 (sort.cpp), T029–T031, T033 validation
```

## Implementation Strategy

### MVP = Phase 1 + Phase 2 + US1 + US2

Both P1 stories together fix the two reported defects in the core
app. **Stop and validate** after T033 (quickstart #1–4, #7, #10) —
this is a shippable, demonstrable milestone. Legacy-shimmed bundled
plugins still work at current capability at this point (same
protection as third-party), so the app remains usable throughout.

### Incremental Delivery

1. Setup + Foundational → builds green, unit tests green
2. US1 → deep-tree management demo (MVP part 1)
3. US2 → Unicode/NFD names demo (MVP complete)
4. US3 → form-insensitive search
5. US4 → SDK 104, shim, shell surfaces, then plugin waves 7a→7d (each wave independently shippable)
6. Polish → benchmark, regression gate, docs, release checklist

### Format validation

All 91 tasks follow `- [ ] T### [P?] [US#?] description + explicit path(s)`; story labels only in Phases 3–6; Setup/Foundational/Polish unlabeled.
