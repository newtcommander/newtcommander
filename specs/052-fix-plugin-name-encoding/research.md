# Phase 0 Research: Fix Plugin Name Encoding in Plugin Manager

**Feature**: 052-fix-plugin-name-encoding · **Date**: 2026-08-06
**Root-cause evidence**: [investigation.md](investigation.md) (three independent
traces, byte-level verification). No NEEDS CLARIFICATION markers remained in the
spec; the decisions below resolve the design unknowns.

## D1 — Encoding contract for `CPluginData` translated metadata

**Decision**: Define the translated metadata strings of `CPluginData`
(`Name`, `Version`, `Copyright`, `Description`, `Extensions`,
`ChDrvMenuFSItemName`; plus any other REG_SZ translated field found by the
implementation audit of `CPlugins::Save`) as **UTF-8**, and enforce it at the
producer boundary: `CSalamanderPluginEntry::SetBasicPluginData`
(`src/plugins1.cpp:1537`) normalizes every incoming string — if the bytes are
valid UTF-8 they are kept; otherwise they are converted CP_ACP → UTF-8. The
registry read path already returns UTF-8, so after this both producers agree.

**Rationale**:
- The registry facade contract from feature 004 is "narrow strings cross as
  UTF-8" (`src/salamdr6.cpp:2298-2303`); the whole codebase direction
  (salunicode.h header comment) is UTF-8 narrow strings. Aligning the field
  with the platform direction fixes the class, not the symptom (spec FR-003).
- The probe-then-convert heuristic is the same transitional tolerance already
  shipped in `SalRegSetValueExW8` (`src/salamdr6.cpp:2390-2399`) — proven in
  production, deterministic for all in-tree plugins (they pass CP1250 from
  `LoadStringA`; Czech diacritic sequences are invalid UTF-8, ASCII is
  identical in both encodings).
- ANSI cannot represent non-Latin scripts, so normalizing the other way would
  permanently block re-enabling Russian/Ukrainian/Chinese (spec edge case).

**Alternatives considered**:
- *Convert registry reads back to ACP for these values* — re-introduces the
  system-code-page ceiling, contradicts the feature-004 contract, and leaves
  the field's encoding undefined for future consumers. Rejected.
- *Make plugins pass UTF-8 (SDK change)* — plugin ABI/interface change for no
  benefit; host-side normalization achieves the same with ABI 105 unchanged.
  Rejected (constitution II, V).
- *Fix only the listview call* — display heals but `Name` stays a
  mixed-encoding trap for every future consumer; this is exactly how features
  010/042/043 each fixed one surface and the defect resurfaced. Rejected as
  sole fix; the listview call is still converted (D2) as defense in depth.

**Residual risk (accepted, documented)**: a legacy string whose CP1250 bytes
happen to form valid UTF-8 would be kept as UTF-8. Practically requires
adjacent high-bit letter pairs that no shipped translation produces; the same
risk is already accepted on the registry write side.

## D2 — Display sink for the Plugins Manager name column

**Decision**: `CPlugins::AddNamesToListView` name column
(`src/plugins2.cpp:1048`) switches from `ListView_SetItemText` (ANSI) to the
existing tolerant helper `SalListViewSetItemTextU8`
(`src/common/winlib.h:346`, used at 20+ sites). Version/DLLName/Loaded
columns stay ANSI (`LoadStr` ANSI template, ASCII data — consistent today).

**Rationale**: same pattern feature 043 applied to the language picker
(`dialogs2.cpp:905`); tolerant fallback keeps the dialog correct even if a
non-normalized producer ever reappears.

## D3 — The mixed-composition sweep D1 forces

**Decision**: The 15 sites annotated
`// encoding-check: allow mixed-composition - plugin-supplied metadata`
(`plugins1.cpp` ×9, `dialogs5.cpp` ×4, `fileswn7.cpp` ×2) compose a `LoadStr`
(ANSI) template with plugin metadata. Once D1 makes the metadata UTF-8, these
compositions would become genuinely mixed. Every such site is converted to the
established 042/043 pattern (`LoadStrU8` template + UTF-8-capable sink), and
its allow-annotation is **removed** so `tools/check_encoding.py`'s
`mixed-composition` rule actively guards it from then on.
`src/dialogs6.cpp:645` (network share name) and `src/mainwnd3.cpp:2827`
(configuration name) are out of scope — not plugin metadata.

**Rationale**: the annotations encode the 042/043 assumption "plugin metadata
encoding is undefined"; this feature defines it, so the waivers become both
false and unnecessary. Leaving any of them would reintroduce the half-true
assumption that caused this regression (investigation.md, regression history).

## D4 — Regression guard (spec FR-005)

**Decision**, three layers:
1. **Static**: extend `tools/check_encoding.py` `UTF8_IDENT` with the plugin
   metadata identifiers (`plugin->Name`, `p->Name`, `->Description`,
   `->Copyright`, `->Extensions`, `->ChDrvMenuFSItemName` and the local-copy
   spellings found during the sweep), reflecting their now-defined contract.
   Removal of the 15 allow-annotations (D3) activates the existing
   `mixed-composition` and `utf8-to-legacy-sink` rules over them.
2. **Runtime**: new `saltests` cases — (a) registry facade round-trip: ANSI-in
   → UTF-16 stored → UTF-8 out, and UTF-8-in → UTF-8 out, with non-ASCII
   Czech sample; (b) the new normalization helper: valid-UTF-8 kept
   byte-identical, CP1250 converted, ASCII unchanged; (c)
   `SalListViewSetItemTextU8` accepts both encodings (existing tolerant
   behavior pinned as a contract).
3. **Build**: `build.cmd` currently prints `Encoding guard: SKIPPED` when
   python is missing (`build.cmd:212-214`) — with the defect present the
   checker found 0 issues *and* could silently not run at all. Change: missing
   python **fails the build** with an instructive message (python is already a
   committed repo technology — `pyproject.toml`, translation tooling). No
   silent skip; no env-var bypass (a bypass recreates the silent-skip hole).

**Rationale**: FR-005 requires the guard to describe the class and fail loudly
when it cannot run. 043's post-mortem of 042's guard ("it described two
examples, not the defect") applies verbatim to `UTF8_IDENT` today — the fix is
to make the convention list track the *contract* (all plugin metadata fields),
not individual repro sites, and to close the python-absent hole.

**Alternatives considered**: a full data-flow analysis rule ("any
registry-read value into any A-sink") — beyond a regex checker's power, high
false-positive rate; rejected in favor of contract-tracked identifiers plus
runtime tests.

## D5 — ZIP plugin name: identical identifier everywhere (spec FR-007/008)

**Decision** (user decision 2026-08-06): `IDS_PLUGINNAME` (id 1007) of the zip
module is the literal `"ZIP"` in **all** languages. Six `.slt` files change
(`translations/<lang>/zip.slt:311`): czech `"PSČ"`, slovak `"PSČ"`, french
`"Code postal"`, spanish `"Código postal"`, german `"ZIP-Archiv"`,
chinesesimplified `"邮编"` → all `"ZIP"`. Five already read `"ZIP"` (dutch,
hungarian, romanian, russian, ukrainian) — untouched.

**Future-proofing**: add `IDS_PLUGINNAME: "ZIP"` entries for the zip module to
`translations/ui-overrides.json` for every non-English language folder (all
10, including currently disabled languages — source is retained). Overrides
win over machine translation (`tools/translate/merge.py`, `load_overrides`),
so a future re-translation cannot reintroduce the mistranslation.

**Note**: only existing rows change (no new string IDs), so the strictly
positional `.slt` import needs no template regeneration; a `build.cmd full`
rebuilds the affected `.slg` modules.

**Alternatives considered**: fixing only the four "postal code" languages and
keeping German "ZIP-Archiv" — overruled by the user's explicit decision that
the name is identical everywhere.

## D6 — How the fix reaches existing installations (spec FR-004, US2)

**Decision**: no data migration and no cache rewrite. Two mechanisms cover it:
1. The display fix (D1+D2) renders the *existing, intact* registry values
   correctly immediately.
2. The corrected ZIP name propagates through the normal re-registration flow:
   a release ships a regenerated `plugins.ver`, and on first start
   `CPlugins::ReadPluginsVer` → `InitDLL` for every plugin
   (`src/plugins2.cpp:3014-3026`, `src/salamdr1.cpp:4517`) refreshes every
   cached `Name` via `SetBasicPluginData` and saves the configuration.

**Rationale**: matches the verified registration behavior (investigation.md
§plugins.ver); satisfies "stored values MUST NOT be rewritten by the fix"
(FR-004) — values are rewritten only by the pre-existing registration flow,
which is normal product behavior on any release.

## D7 — Release documentation

**Decision**: add a `CHANGELOG.md` entry (Fixed: garbled plugin names in
Plugins Manager for non-English UI; Changed: ZIP plugin is named "ZIP" in all
languages) under the next unreleased version heading, following the file's
existing convention. Version/build-number bump happens in the release change
per constitution §Release Documentation, not in this feature's code changes
(unless the user asks to release).
