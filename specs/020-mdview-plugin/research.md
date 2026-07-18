# Research (Phase 0): mdview

Decisions and rationale locking the design before implementation. Builds on the
six specify-phase agent reports in `analysis/` and the clarified spec.

## D-R1 — Rendering surface: RichEdit 4.1 + generated RTF

**Decision**: render by converting the parsed document to RTF and displaying it
in a standard **RichEdit 4.1** control (`MSFTEDIT_CLASS`, `MSFTEDIT.DLL`),
read-only, hosted as a child of the viewer window.

**Why (realizes clarify Q2 "static native renderer", not a browser engine)**:
- **Security by construction** (FR-040…FR-046): RichEdit has no script engine,
  no HTML DOM, no network stack. RTF we emit contains only text + formatting;
  active content is impossible. This is the strongest possible assurance story.
- **Selection / search / zoom for free** (FR-071, FR-073): native selection +
  `Ctrl+C`/`Ctrl+A`; `EM_FINDTEXTEXW` powers Ctrl+F and F3/Shift+F3;
  `EM_SETZOOM` powers Ctrl+wheel / Ctrl+±/0 — features a from-scratch engine
  would have to reimplement.
- **Theming**: per-scheme document background via `EM_SETBKGNDCOLOR`; all text
  colors via the RTF color table; monospaced code, headings, bold/italic, list
  indents, blockquote indents, table borders all expressible in RTF.
- **Constitution VI**: RichEdit is a standard control used normally (we set
  colors and read-only, we do not subclass/owner-draw to restyle it, we do not
  touch process-wide class registration).

**Trade-offs & mitigations**: RTF tables are basic and image embedding is
awkward → v1 renders tables as best-effort RTF tables and images as labeled
**placeholders** (safe, never-crash); inline raster images via WIC and a fuller
table model are documented enhancements. A from-scratch DirectWrite renderer
remains the long-term direction for pixel-perfect fidelity.

**Links**: RTF `CFE_LINK` char effect + `EN_LINK` notifications; on click we run
the security gate (D-R4) before doing anything. Internal `#anchor` links are
handled by us (scroll to a recorded character offset), never handed to RichEdit
as URLs.

## D-R2 — Markdown parser: self-contained in-tree (md4c documented upgrade)

**Decision**: implement a pragmatic, dependency-free block+inline parser
(`mdparser.*`) producing the document model (data-model.md). It covers the FR-010
core and FR-011 GFM subset best-effort: ATX/Setext headings, paragraphs,
soft/hard breaks, `*`/`_` emphasis + strong, `~~` strikethrough, inline code,
fenced (```/~~~) + indented code blocks with info string, blockquotes (nested),
ordered/unordered/nested/task lists, thematic breaks, links (inline + reference)
and images, autolinks (angle + bare http/https/www/email), GFM pipe tables with
alignment, backslash escapes, HTML entities, and the degradation rules (FR-014).

**Why not vendor md4c now**: md4c (MIT, ~single translation unit, CommonMark
0.31.2, powers Qt) is the correct production parser (FR-010) and the documented
upgrade. It is deferred only because reliably vendoring a ~2700-line source file
under autonomous execution (no local copy, network-fetch truncation risk) would
jeopardise a clean build. The parser sits behind the document-model boundary, so
swapping to md4c later is localized to `mdparser.*`.

**Consequence for the spec**: FR-010's "CommonMark 0.31.2" is met *by design
intent* (md4c) but the shipped v1 parser is a documented subset — recorded as a
known deviation in this research and the plugin's `IMPLEMENTATION_NOTES`.

## D-R3 — Images: safe placeholders in v1

Per FR-020/024 and the security threat model: local relative raster images are
resolved (canonicalized against the document dir, relative-only — FR-021) and in
v1 shown as an in-flow **placeholder** carrying the alt text + filename; missing/
absolute/UNC/remote refs likewise show a placeholder with the reason. This is a
safe, never-crash baseline. Inline display via **WIC** (PNG/JPEG/GIF/BMP/WebP,
feature-006 pattern) and **nanosvg** (already vendored) for SVG is a documented
follow-up, gated behind the same relative-path + no-network rules. No remote
fetch ever occurs (FR-025).

## D-R4 — Link security gate (allowlist)

A single `ActivateLink(url)` chokepoint (FR-030/031): decode + canonicalize;
`#anchor` → internal scroll; a registered-mask local `.md` (relative) → open in a
**new mdview window** (clarify Q3); scheme in {`http`,`https`,`mailto`} →
`ShellExecute` (system default handler); everything else (`javascript`,
`vbscript`, `data`, `file`, UNC, unknown) → blocked with a status notice. Nothing
is resolved/opened without an explicit click or Enter (FR-030). The real target
is shown on hover/focus (FR-032).

## D-R5 — Encoding pipeline (FR-050)

Mirror the core `ViewerDetectEncoding` contract locally: (1) binary/NUL sniff →
hand off to the internal text viewer; (2) UTF-8 BOM → UTF-8; (3) UTF-16 LE/BE BOM
→ UTF-16; (4) no BOM + strict whole-buffer UTF-8 validation → UTF-8; (5) fail →
system ANSI codepage (CP1250 on Czech Windows) + non-modal warning bar + "Open in
text viewer". Convert to UTF-16 for parsing via SDK `SplU8ToW*`; the plugin holds
no fixed `MAX_PATH` buffers and opens files through `SplU8ToWExtAlloc` (`\\?\`).

## D-R6 — Themes → RTF mapping (FR-060/061)

10 schemes as static tables (`themes.*`): each defines ~15 roles + 9 syntax
tokens. RTF emission builds a per-render color table from the active scheme;
document background via `EM_SETBKGNDCOLOR`. Scheme identifiers are stable ASCII
(`paper`, `graphite`, …) — never localized names, never integer indexes (feature-
007 index-coupling pitfall). A debug-only self-check asserts the FR-061 contrast
gates (relative-luminance formula over scheme × role); stock Solarized/Nord
comment/body values that fail AA are pre-corrected in the tables
(analysis/visual.md). Switching a scheme re-emits RTF and preserves scroll
(record + restore first visible char via `EM_GETFIRSTVISIBLELINE`/`EM_LINEINDEX`).

## D-R7 — Plugin plumbing reuse map (from demoview / sftp)

- Entry point, `SetBasicPluginData(FUNCTION_VIEWER | FUNCTION_CONFIGURATION |
  FUNCTION_LOADSAVECONFIGURATION)`, `LoadLanguageModule`, `Connect` →
  `AddViewer("*.md;*.markdown", FALSE)` + icons: **from demoview** (demoview.cpp).
- Thread-per-viewer-window + lock/lockOwner event handshake + own `GetMessage`
  loop + `ViewerWindowQueue` + `WM_USER_VIEWERCFGCHNG` broadcast: **from
  demoview** (viewer.cpp) — the RichEdit child replaces demoview's custom paint.
- `CanViewFile` = cheap pre-checks only (open + size gate + binary sniff); FALSE
  cascades to the internal text viewer (FR-080).
- Next/previous file: `GetNextFileNameForViewer`/`GetPreviousFileNameForViewer`
  with `enumFilesSourceUID`/`enumFilesCurrentIndex` (FR-075).
- Config Load/Save with a `Version` DWORD (FR-100) + broadcast (FR-101).
- Build wiring (`mdview.vcxproj`, `lang_mdview.vcxproj`, `.props`, `.def`,
  solution entry, `plugins.cfg` line): **from sftp** (newest reference).

## D-R8 — Resource limits & threading (FR-043/090/092)

Parse + RTF build run in a bounded pass with caps: size gate 20 MB (FR-091),
nesting depth 64 (iterative/depth-checked — no recursion blowup, feature-014
lesson), a node/output cap, and a parse watchdog. RichEdit does its own layout;
the heavy step (parse+RTF) is bounded and, for large inputs, may run before the
window shows content. Pathological inputs degrade to literal text / abort to the
text-viewer fallback (FR-081) rather than hang.

## Open follow-ups (documented, not v1-blocking)

- Swap self-contained parser → vendored md4c (restores full CommonMark 0.31.2).
- Inline raster/SVG image rendering (WIC + nanosvg) replacing placeholders.
- Richer table model; per-monitor-V2 DPI; follow-OS-theme live switching polish.
