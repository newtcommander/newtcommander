# Contract: Per-Icon Dark Override Files

**Feature**: 029-dark-toolbar-icons | **Consumers**: app runtime (`RenderSVGImage`), icon authors, build deploy

## Location and naming

- Source (repo): `src\res\toolbars\dark\<Name>.svg`
- Deployed (runtime): `<directory of salamand.exe>\toolbars\dark\<Name>.svg`
- `<Name>` MUST exactly match the standard glyph's base name, i.e. the
  `SVGName` string in `ToolBarButtons[]` (src/toolbar4.cpp) — the same name
  as the file in `toolbars\` (e.g. `Copy`, `Back`, `ClipboardCut`).
  Matching is done by file path; names are case-insensitive per NTFS but
  SHOULD be written in the exact table casing.

## Semantics

1. Override files are consulted **only while the Dark theme is active**
   and only for the **enabled** button state. The Default theme never
   reads `toolbars\dark\`.
2. Precedence in Dark theme: override SVG (verbatim — no automatic color
   adaptation is applied to it) → standard SVG with automatic dark
   adaptation → legacy raster glyph with the 028 bitmap transform.
3. A missing, unreadable, or unparseable override falls back silently
   (TRACE-level log only) to the next step of the chain. An override can
   therefore never make a button render empty.
4. Overrides are optional: zero override files is a valid, fully
   functional state (automatic adaptation covers all icons).

## Authoring requirements

- Plain SVG subset supported by nanosvg (paths, fills, strokes, solid
  colors; no scripts, no external references). Same canvas/viewBox
  conventions as the standard glyph so the motif stays aligned.
- Design for the dark toolbar background `COLOR_BTNFACE` = RGB(45,45,45)
  (see `THEME_DARK_SYSCOLORS` in src/common/themes_palette.h). Dominant
  strokes SHOULD reach ≥3:1 contrast against it (SC-002).
- Keep the same motif/silhouette as the light icon (spec FR-002).

## Change management

- Adding/removing an override is a data change (no code, no resource
  rebuild) — but note the file is read at image-list build time: the app
  picks it up on next start or next theme switch, not mid-session.
- The build deploys the whole directory via `!populate_build_dir.cmd`;
  new files need no project changes.
