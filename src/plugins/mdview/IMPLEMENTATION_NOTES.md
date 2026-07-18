# mdview — Implementation Notes (v1)

Feature 020. Spec/plan/tasks: `specs/020-mdview-plugin/`.

## What v1 implements

- Viewer plugin registered for `*.md;*.markdown` (F3). Thread-per-window +
  lock handshake adapted from `demoview`; build wiring modeled on `sftp`.
- **Rendering surface**: a standard **RichEdit 4.1** control fed generated RTF
  (research.md D-R1). Realizes the clarified "static, script-free native
  renderer" — the security invariants (FR-040…046) hold by construction (no
  script engine, no HTML DOM, no network in the rendering path). Selection,
  Ctrl+C/A, in-document search (EM_FINDTEXTEXW) and zoom (EM_SETZOOM) are native.
- **Parser** (`render.cpp`): self-contained block+inline Markdown parser →
  document → RTF. Covers headings, paragraphs, emphasis/strong/strike, inline
  code, fenced+indented code blocks with info string, blockquotes, ordered/
  unordered/nested/task lists, thematic breaks, links, images (placeholder),
  autolinks, GFM pipe tables, entities/escapes, YAML front matter, and the
  degradation rules.
- **Syntax highlighting** (`highlight.cpp`): best-effort lexical highlighter for
  the tier-1 languages + alias table; diff coloring; unknown → plain block.
- **10 color schemes** (`render.cpp` themes table), contrast-corrected per
  analysis/visual.md; live switch via View → Color Scheme (+ F9/Shift+F9),
  persisted; Follow-System-Theme mode (AppsUseLightTheme).
- **Encoding** (FR-050): BOM / strict-UTF-8 / UTF-16 / ANSI(CP_ACP) fallback;
  binary → hand off to text viewer.
- **Link security gate** (`ActivateLinkByCp`): `#anchor` scroll; local `.md` →
  new mdview window; http/https/mailto/ftp → ShellExecute; everything else
  blocked. No network on open; remote images are placeholders.
- **Fallback**: `CanViewFile` declines unreadable/binary → internal text viewer;
  size gate (20 MB) + "Open as Text"; long-path safe (`SplU8ToWExtAlloc`,
  no fixed MAX_PATH).

## Documented deviations from the spec (v1 scope)

1. **Parser is a pragmatic subset**, not full CommonMark 0.31.2 (FR-010). md4c
   (MIT, single-file) is the production upgrade behind the same
   parser→document boundary (research.md D-R2). Edge cases of emphasis
   precedence, reference-link definitions, and tight/loose list nuances are
   approximate.
2. **Images render as labeled placeholders** (FR-020/022 inline display
   deferred). Inline raster via WIC + SVG via the vendored nanosvg is the
   documented follow-up; the relative-path-only / no-remote-fetch security
   rules are already enforced.
3. **Tables** render as aligned monospaced text (not an RTF grid).
4. **RichEdit** instead of a from-scratch DirectWrite renderer (Complexity
   Tracking in plan.md); a custom renderer remains the path to pixel-perfect
   tables/images.
5. **Next/previous-file** (Space/Backspace) and the encoding **warning bar**
   are deferred (encoding shown in the title bar instead).
6. Contrast values are hand-tuned; the automated FR-061 gate check is a
   documented follow-up.

## Key files

`mdview.cpp` entry/interface/config · `viewer.cpp/.h` window+RichEdit ·
`render.cpp/.h` parser+themes+RTF · `highlight.cpp` lexer · resources in
`*.rh2`/`*.rc`/`lang/`.
