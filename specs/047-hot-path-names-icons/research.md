# Research: Hot Path Display Names and Custom Icons

**Feature**: 047-hot-path-names-icons · **Date**: 2026-08-02

All unknowns from the Technical Context were resolved by a code survey of the
current hot-path implementation plus the two clarification decisions recorded in
`spec.md` (§ Clarifications). No NEEDS CLARIFICATION items remain.

## Current-state findings (code survey)

- **Data model**: `CHotPathItem { char* Name; char* Path; BOOL Visible; }`,
  30 fixed slots in `CHotPathItems Items[HOT_PATHS_COUNT]` (`src/mainwnd.h:75-237`,
  `HOT_PATHS_COUNT` = 30 at `src/mainwnd.h:7`). No icon field.
- **"Assigned" marker**: every consumer treats *Name non-empty* as "slot is
  assigned": Hot Path Bar skips empty names (`src/toolbar7.cpp:28-104`), the
  Change Drive menu requires `Visible && Name && Path` (`src/drivelst.cpp:2155-2159`),
  `FillHotPathsMenu` keys on name length (`src/mainwnd1.cpp:70-153`),
  `GetUnassignedHotPathIndex` searches for empty names (`src/mainwnd1.cpp:155-164`).
- **Why users see paths**: direct quick-assign sets `Name = raw path`
  (`src/mainwnd1.cpp:1262-1265`); the same defaulting exists in `Load1_52`
  (`src/mainwnd1.cpp:270-271`). The only rename UI is the ListView in-place label
  edit (`LVS_EDITLABELS`, `src/dialogs4.cpp:3056-3084`).
- **Icon today**: one shared `HICON HFavoritIcon` for every surface, loaded from
  **shell32.dll resource 319** at 16 px DPI-scaled (`src/salamdr1.cpp:2356`),
  reloaded on color/DPI change (bar rebuilt at `src/mainwnd1.cpp:3168-3172`).
  Toolbar buttons take raw per-item `HICON`s (`TLBI_MASK_ICON`), the drive menu
  supports per-item `mii.HIcon` (`src/drivelst.cpp:2236-2259`) — so per-item
  icons need **no new rendering capability**, only per-item data.
- **Registry**: `HKCU\Software\Tandem Commander\0.1\Hot Paths\<1..30>` with
  `Name` (REG_SZ), `Path` (REG_SZ), `Visible` (REG_DWORD)
  (`src/mainwnd1.cpp:57-277`); legacy quirk: subkey `"0"` maps to index 9 on
  load. Slots that are empty are deleted rather than written.
- **Per-item icon precedent**: User Menu items carry `char* Icon` + resolved
  `HICON` (`src/usermenu.h:107-151`) and `CUserMenuBar` renders per-button
  icons — confirms the toolbar/menu pipeline handles heterogeneous icons.
- **Icon resource convention**: `IDI_* ICON "res\\*.ico"` statements in
  `src/salamand.rc2:19-31`, IDs in `salamand.rh`, assets in `src/res/`.

## D1: Default icon and variant artwork

- **Decision**: Keep gallery index 0 = the exact current default: `HFavoritIcon`
  loaded from shell32.dll resource 319 at runtime. Ship **9 original color
  variants** (red, orange, yellow, green, teal, blue, purple, pink, gray) of a
  bookmark/star motif as our own `.ico` resources (`src/res/hotpath1.ico` …
  `hotpath9.ico`) — 10 gallery choices total (FR-007 allows 8–10).
- **Rationale**: SC-004 demands zero visible change on upgrade — reusing the
  very same shell32 icon as the default guarantees it. Microsoft artwork cannot
  be extracted, recolored and redistributed (GPLv2 + licensing), so the color
  variants must be original artwork; shipping them as committed `.ico` files
  matches the feature-035 brand-asset pipeline and keeps the build reproducible.
- **Alternatives considered**:
  - *Full custom set including a custom default* — rejected: risks a visible
    default-icon change on upgrade and gratuitously replaces a familiar glyph.
  - *Runtime tinting of the shell32 icon (HICON → DIB → hue shift)* — rejected:
    derivative processing of Microsoft artwork, fragile across Windows versions
    (the source glyph can change), uncontrollable legibility in dark theme, and
    the most code for the least predictability.
  - *SVG assets* (the repo has some `.svg` resources) — rejected: the existing
    icon path for per-item HICONs is `.ico`-based (`LoadImage`), and `.ico`
    carries hand-tuned small sizes; no reason to invent a second pipeline.

## D2: Persistence format and upgrade/downgrade compatibility

- **Decision**: Add per-slot `Icon` (REG_DWORD, gallery index 0–9) written only
  when ≠ 0; out-of-range or missing values load as 0. Keep the legacy meaning of
  the `Name` value on disk: **Save always writes the effective label** — the
  custom name when set, otherwise the path text — and **Load treats
  `Name == Path` as "unnamed"** (per clarification #2). In memory, `Name` holds
  only the custom name (empty = unnamed). The empty-slot rule changes from
  "name+path empty" to "path empty" (an empty slot's subkey is still deleted;
  a deleted entry also resets `Icon`).
- **Rationale**: Additive and both-direction compatible: an older build reading
  a new config sees exactly the format it expects (non-empty `Name` for every
  assigned slot) and ignores `Icon`; the new build reading an old config gets
  the clarified migration rule for free, because historical auto-fill produced
  `Name == Path`. Round-trip is stable (unnamed → saves `Name=Path` → loads
  unnamed). A custom name typed to be identical to the path degrades to
  "unnamed" — explicitly accepted in clarification #2 (no visible difference).
- **Alternatives considered**:
  - *Write empty `Name` for unnamed entries* — rejected: a downgraded build
    would treat those slots as unassigned (menus key on name length) — silent
    data loss on downgrade for zero benefit.
  - *New separate `DisplayName` value, `Name` untouched* — rejected: duplicates
    state, two sources of truth, and still needs the `Name==Path` heuristic for
    configs written before this feature.

## D3: The "assigned slot" marker moves from Name to Path

- **Decision**: A slot is assigned iff `Path` is non-empty. Add central helpers
  on `CHotPathItems`: `GetDisplayName(index)` (custom name if non-empty, else
  the path in its user-visible form — stored `$$` unescaped to `$`, matching
  the text quick-assign used to put into `Name`) and `GetIconIndex(index)` /
  `SetIconIndex(index, i)`. Convert every consumer to the helpers:
  `CHotPathsBar::CreateButtons` (`src/toolbar7.cpp`), `CDrivesList::BuildData`
  (`src/drivelst.cpp` — condition becomes `Visible && Path non-empty`),
  `CHotPathItems::FillHotPathsMenu` + `GetUnassignedHotPathIndex`
  (`src/mainwnd1.cpp`), jump list titles (`src/jumplist.cpp`), and the config
  page list (`src/dialogs4.cpp`). Quick-assign (`SetUnescapedHotPath`,
  `SetUnescapedHotPathToEmptyPos` in `src/fileswn1.cpp`; direct-write branch in
  `src/mainwnd1.cpp:1242-1272`) stops filling `Name` and sets only `Path` (and
  leaves `IconIndex` at 0), satisfying FR-011.
- **Rationale**: One resolution rule in one place (FR-002/SC-003 demand all
  seven surfaces agree); eliminating the auto-filled name is what makes the
  Name field genuinely optional.
- **Alternatives considered**: *Keep Name as the marker and auto-mirror the
  path into it* — rejected: perpetuates the tangle this feature exists to
  remove, and makes "clear the name" (FR: fallback display) unimplementable
  without sentinel values.

## D4: Settings page UI for name and icon

- **Decision**: Extend `IDD_CFGPAGE_HOTPATH` (`src/lang/lang.rc:272-286`) with a
  labeled **Name** edit box (`IDC_HOTPATH_NAME`, above the existing Path edit)
  and an **Icon** owner-drawn drop-list combo (`IDC_HOTPATH_ICON`,
  `CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED`, icon swatches only — no text, so no
  translatable color names). The ListView gains a small imagelist (`LVSIL_SMALL`)
  showing each row's gallery icon; rows show the display label (name, else
  path). In-place label editing (F2) remains and now edits the *custom name*,
  where an empty result is valid and means "unnamed". Delete resets name, path,
  visibility and icon (FR-013). Validation: name without path → error on page
  confirm (FR-004); names are trimmed (FR-005, existing `CleanName`).
- **Rationale**: Smallest coherent extension of the existing page; an icon-only
  owner-drawn combo is the standard WinAPI idiom for a fixed swatch gallery,
  avoids a modal sub-dialog for a 10-item choice, and adds zero translatable
  strings beyond the two static labels. Constitution VI allows functional
  owner-draw (this is not restyling a standard text control).
- **Alternatives considered**:
  - *`CChangeIconDialog`-style popup browser* (User Menu precedent) — rejected:
    designed for arbitrary icon files, which is explicitly out of scope.
  - *Icon column with click-to-cycle* — rejected: undiscoverable, no preview of
    the full gallery.
- **Translation impact**: two new static labels (and the validation message if a
  new string is needed) enter `src/lang/lang.rh`/`lang.rc` and must be merged
  into the `.slt` modules with the feature-038 `translate.merge` tooling for the
  enabled languages (`translations/languages.cfg`); `.slt` import is strictly
  positional, so the English template is regenerated first.

## D5: Icon loading, DPI and theme lifecycle

- **Decision**: Introduce `HICON HHotPathIcons[HOT_PATH_ICON_COUNT]` (10) beside
  `HFavoritIcon` in `src/salamdr1.cpp`: index 0 aliases the shell32 default,
  1–9 load via `LoadImage` from the new exe resources at
  `IconSizes[ICONSIZE_16]`. Create/destroy/reload exactly where `HFavoritIcon`
  is handled today (creation `src/salamdr1.cpp:2356` area, destruction
  `:2691-2694`, color/DPI reload with existing Hot Path Bar rebuild at
  `src/mainwnd1.cpp:3168-3172`). The settings dialog builds its ListView
  imagelist from the same handles at dialog init. Variant artwork is designed
  with midtone fills + dark outline so a single asset set stays legible in
  light and dark themes (verified in quickstart; SC-005/FR-007/FR-014).
- **Rationale**: Reuses a proven lifecycle — every reload point and rebuild hook
  already exists for `HFavoritIcon`; per-item `HICON`s are already the render
  contract of both the toolbar and the drive menu (survey findings), so no
  imagelist plumbing is needed on the display surfaces.
- **Alternatives considered**: *A shared HIMAGELIST for hot path icons* —
  rejected: neither consumer takes imagelists for these items; would add
  conversion code with no gain at N=10.

## D6: Asset pipeline

- **Decision**: Extend `tools/brand/gen_icons.py` with a hot-path section: a new
  original master `tools/brand/hotpath-master.png` (bookmark motif) plus a
  9-entry tint table generating `src/res/hotpath1.ico` … `hotpath9.ico`, each
  with 16/20/24/32 px frames (covers 100–200 % scaling of the 16 px base).
  Generated `.ico` files are committed; `tools/brand/README.md` documents the
  regeneration command, per the feature-035 convention.
- **Rationale**: One command regenerates everything (Build Reproducibility);
  tinting one master guarantees the variants differ only in hue (the gallery's
  whole point); frame sizes follow the existing `IconSizes[]` ladder.
- **Alternatives considered**: *Hand-authoring nine `.ico` files* — rejected:
  drift between variants, painful to re-tint; *build-time generation* —
  rejected: the build must not depend on Python (feature-038 precedent:
  developer-side only).
