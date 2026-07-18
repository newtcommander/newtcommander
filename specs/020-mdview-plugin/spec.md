# Feature Specification: mdview — Rendered Markdown Viewer Plugin

**Feature Branch**: `020-mdview-plugin`
**Created**: 2026-07-18
**Status**: Draft — clarified 2026-07-18, ready for `/speckit.plan`
**Input**: `features/mdview-plugin.md` (Czech feature brief, 18 sections),
loaded verbatim per user instruction.

## Problem Statement

Open Salamander has no rendered view for Markdown. F3 on a `.md` file shows
raw source in the internal text viewer, while Markdown is now the dominant
format for READMEs, CHANGELOGs, docs trees, and AI-generated notes. The
**mdview** plugin adds a read-only viewer: F3 on a `.md` file opens the
document *rendered* — headings, lists, tables, code blocks with highlighting,
links, and local images — in a window that feels native to Salamander, treats
every document as untrusted input, and ships with 10 selectable color schemes.

Viewing only; editing is out of scope (brief §1).

## Multi-Agent Analysis (brief §2 mandate)

Six independent analyst agents were run before this spec was written. Full
raw reports: [analysis/ux.md](analysis/ux.md),
[analysis/integration.md](analysis/integration.md),
[analysis/markdown.md](analysis/markdown.md),
[analysis/security.md](analysis/security.md),
[analysis/visual.md](analysis/visual.md),
[analysis/testing.md](analysis/testing.md).

**Conflicts found and their consolidation:**

| Topic | Positions | Consolidated position |
|---|---|---|
| Raw HTML in documents | Markdown agent: small inert whitelist (`<br>`, `<b>`, `<i>`, …) for README fidelity. Security agent: ALL raw HTML as inert literal text — provably safe; whitelist = sanitizer-trap risk. | Spec default = **inert literal text** (security wins for v1; whitelist is a reviewed v2 upgrade path). Final ruling deferred to clarify — see Q1. |
| Registered extensions | Integration agent: all of `*.md;*.markdown;*.mdown;*.mkd` (cost = one string). Markdown agent: `*.md;*.markdown` only (`.mdown`/`.mkd` vestigial, pollute the association table). UX agent: `.markdown` yes, rest "PO taste". | Spec default = **`*.md;*.markdown`** with documented justification for the omission (brief §4 requires it). Clarify confirms (deferred decision D1). |
| Rendering surface | UX agent: engine class decides the selection/search quality ceiling. Integration agent: native renderer + vendored parser recommended; WebView2 needs an owner ruling (NuGet-free repo, child processes). Security agent: static script-free renderer satisfies invariants *by construction*; browser engine = continuous assurance burden. | Spec mandates the **security invariants and assurance requirement** (any implementation must demonstrate them); working assumption = static script-free rendering. Whether a browser engine may even be considered, and the v1 selection/search quality bar, deferred to clarify — see Q2. |
| Space/Backspace keys | Browser habit (Space = page down) vs host convention (Space/Backspace = next/previous file, used by every existing viewer plugin). | **Host convention wins** (consistency is the point); documented as deliberate. |
| Remote images | All three agents that touched it (security, UX, Markdown) converged independently. | **Blocked by default; per-document, per-session explicit consent** — adopted as the spec decision (brief §6 demanded one). |

## Clarifications

### Session 2026-07-18

- Q: Renderer class / quality bar for v1 — static script-free renderer vs a
  browser engine (WebView2)? → A: **Static renderer** — vendored
  GPLv2-compatible CommonMark+GFM parser + custom native rendering; the
  security invariants hold by construction (no script engine exists);
  viewer-grade selection + search. A browser engine is NOT used in v1.
- Q: Raw HTML policy — all-literal vs a safe micro-whitelist? → A: **Inert
  literal text** for all raw HTML (HTML entities still decode; no HTML element
  is ever instantiated). A reviewed micro-whitelist is a post-v1 upgrade only.
- Q: Local-file link navigation model for v1? → A: Local **`.md` links open in
  a new mdview window** (read-only, limits re-applied); other local files show
  the resolved path only (copy-path, not launched); UNC/absolute/`file:`
  targets stay fully blocked.
- Q: v1 scope of the optional viewer controls (search / zoom)? → A: **Both
  search and zoom ship in v1** (Ctrl+F with F3/Shift+F3 next/previous; zoom via
  Ctrl+wheel and Ctrl+plus/minus/0, persisted).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - F3 shows rendered Markdown (Priority: P1)

A user selects `README.md` in a panel and presses F3. Instead of raw source,
a viewer window opens showing the rendered document: styled headings, lists,
tables, syntax-highlighted code blocks — the way the same file looks on
GitHub. Czech text renders correctly. Esc closes the window and returns focus
to the panel.

**Why this priority**: This is the feature. Everything else refines it.

**Independent Test**: F3 on a fixture `.md` with GFM content → rendered view
(no visible `#`/`*`/backtick markers); Esc returns to the panel with the file
still selected; Alt+F3 still opens raw source in the internal viewer.

**Acceptance Scenarios**:

1. **Given** a `.md` file in a local panel and mdview installed, **When** the
   user presses F3, **Then** mdview opens showing rendered Markdown (not
   source, not the internal text viewer, not an external app).
2. **Given** the viewer is open, **When** the user presses Esc, **Then** the
   window closes and panel focus is restored.
3. **Given** a document containing Czech UTF-8 text (with or without BOM),
   **When** rendered, **Then** all diacritics display correctly in headings,
   body, tables, code, and link text.
4. **Given** the same file, **When** the user presses Alt+F3, **Then** the
   internal text viewer shows the raw Markdown source (unchanged behavior).

---

### User Story 2 - Untrusted documents are safe to open (Priority: P1)

A user downloads a README from the internet (or opens one from an archive or
SFTP) and presses F3. The document may contain hostile content — script tags,
`javascript:` links, remote tracking images, pathological nesting. Nothing
executes, nothing phones home, the app never hangs or crashes.

**Why this priority**: The viewer runs in-process at full user privilege with
no OS sandbox; a rendering viewer that can be weaponized by a text file is
worse than no viewer.

**Independent Test**: Open the hostile fixture (`16-html.md`) and the
pathological fixtures; observe zero process/network/dialog activity and a
responsive UI throughout.

**Acceptance Scenarios**:

1. **Given** a document with `<script>`, event-handler attributes, `<iframe>`,
   forms, or SVG script, **When** opened and idle for 30 s and then clicked
   anywhere, **Then** no script executes, no navigation occurs, no network
   request is made.
2. **Given** a document referencing a remote (http/https) image, **When**
   opened, **Then** no network request occurs; a placeholder is shown until
   the user explicitly invokes "Load remote images" for this document.
3. **Given** a document with `javascript:`, `file:`, UNC, or unknown-scheme
   links, **When** clicked, **Then** the link is inert and visibly blocked.
4. **Given** a document with 10,000-deep nesting or pathological emphasis
   patterns, **When** opened, **Then** the UI stays responsive, rendering
   completes or aborts within the watchdog budget, and Esc always works.

---

### User Story 3 - Local images display (Priority: P2)

A user opens a project's documentation whose Markdown references images by
relative path (`images/architecture.png`, paths with spaces or Czech
characters, `../shared/logo.png`). The images appear inline, scaled to fit
the window width; a missing or broken image shows a labeled placeholder
instead of breaking the page.

**Independent Test**: Fixture `06-images/images.md` with its `assets/` tree.

**Acceptance Scenarios**:

1. **Given** an image referenced relative to the document's directory,
   **When** rendered, **Then** it displays with preserved aspect ratio,
   downscaled to the content width when wider (never upscaled by default).
2. **Given** a reference to a missing, corrupt, or unsupported image,
   **When** rendered, **Then** an in-flow placeholder shows the alt text and
   the reason; the rest of the document is unaffected.
3. **Given** an absolute, drive-letter, UNC, or `file:` image reference,
   **When** rendered, **Then** the error placeholder is shown and the path is
   never opened (no disk/network probe).
4. **Given** the window is resized, **Then** images re-fit to the new width.

---

### User Story 4 - Links and document navigation (Priority: P2)

A user reads a long document with a table of contents. Clicking a TOC entry
jumps to that section (including headings with Czech diacritics). Clicking an
`https://` link opens the system browser — but only on an explicit click, with
the real target visible beforehand.

**Acceptance Scenarios**:

1. **Given** `[Section](#section)` links, **When** clicked (or activated via
   keyboard focus + Enter), **Then** the view scrolls to that heading;
   a link to a nonexistent anchor is a no-op with a non-modal notice.
2. **Given** an external `http`/`https`/`mailto` link, **When** hovered or
   keyboard-focused, **Then** the canonical target is displayed; **When**
   clicked, **Then** the system default handler opens it and the viewer stays.
3. **Given** any link, **When** the document loads, **Then** nothing is
   opened, resolved, or prefetched automatically.
4. **Given** a link to another local file, **Then** behavior follows the
   clarify ruling (Q3); until then links to non-Markdown local files show the
   resolved path and are not launched.

---

### User Story 5 - Color schemes (Priority: P2)

A user opens a document at night and switches the viewer to a dark scheme
from the View menu (or cycles schemes with a hotkey). The change applies
instantly without losing the reading position. The next document opens in the
same scheme, even after an application restart.

**Acceptance Scenarios**:

1. **Given** the viewer is open, **When** the user opens the scheme picker,
   **Then** at least 10 schemes are offered (5 light: Paper, Soft Gray, Warm
   Sepia, Solar Light, Arctic Light; 5 dark: Graphite, Midnight, Solar Dark,
   Nordic Dark, High Contrast Black), selectable by mouse and keyboard.
2. **Given** a scheme is selected, **When** applied, **Then** the repaint is
   immediate, the file is not re-opened, and the scroll position is kept.
3. **Given** a scheme was chosen, **When** the viewer, then the application,
   is closed and relaunched, **Then** the next document opens in that scheme.
4. **Given** the persisted scheme value is corrupted (unknown name, wrong
   type, deleted), **When** the viewer opens, **Then** the default scheme is
   used without any crash or error-dialog storm.

---

### User Story 6 - Robustness and fallback (Priority: P2)

A user F3s a file that is not really Markdown (binary garbage renamed `.md`),
a 100 MB machine-generated log with `.md` extension, or a file with broken
encoding. They always end up with something usable — mdview renders
best-effort, or hands off to the plain-text view with a clear message — and
the application never crashes.

**Acceptance Scenarios**:

1. **Given** merely malformed Markdown (unclosed fences, broken tables),
   **When** opened, **Then** it renders best-effort with no error surface.
2. **Given** a binary file with `.md` extension, **When** F3 is pressed,
   **Then** the user ends with a usable view (hand-off to the text/hex viewer
   or an explicit "not text" notice with an open-as-text action); no crash.
3. **Given** a document larger than the size gate, **When** opened, **Then**
   a clear "too large" message offers opening as plain text.
4. **Given** any render failure after the window opened, **Then** an error
   state names the file and reason and offers "Open in text viewer".
5. **Given** 50 open/close cycles across mixed fixtures, **Then** GDI/USER
   handles and memory return to baseline within defined tolerances.

---

### Edge Cases

The full inventory (25 cases with expected behavior) is in
[analysis/testing.md](analysis/testing.md). Highlights the requirements below
must cover:

- Empty file → empty document, not an error. Nonexistent / permission-denied
  / exclusively-locked file → distinct, clear errors; no crash.
- Encodings: UTF-8 ± BOM; UTF-16 LE/BE with BOM; invalid UTF-8 → legacy
  fallback with warning; NUL-containing binary → text/hex hand-off. CRLF/LF/CR
  and mixed line endings render identically. BOM never renders.
- Paths: >260-character paths (291-char ASCII and ~540-byte Unicode test
  trees exist) MUST work — **no fixed `MAX_PATH` buffers** (the exact defect
  class fixed in features 011–014); Unicode filenames (Czech, CJK, emoji) in
  content, title bar, and image resolution (features 005/010/015 bug class).
- Files viewed from archives or SFTP arrive as temp-extracted copies: the
  document renders; sibling-relative images legitimately resolve as missing
  (placeholders — correct result); the temp-file lock is held exactly for the
  window lifetime.
- File changed/deleted while open → the loaded snapshot stays; refresh
  re-applies all policies and reports a missing file gracefully.
- Two viewer windows at once → independent state, any close order.

## Requirements *(mandatory)*

### Activation & registration

- **FR-001**: Pressing F3 (and the View command) on a file matching the
  registered masks MUST open the mdview window showing the rendered document.
- **FR-002**: The plugin MUST register viewer masks `*.md;*.markdown` at
  install time, respecting later user changes (no re-forcing on upgrade).
  Justification for omitting `.mdown`/`.mkd` (brief §4): vestigial
  pre-standardization variants, effectively absent from post-2015
  repositories; users can add masks manually in viewer configuration.
  [Deferred decision D1 confirms the set.]
- **FR-003**: Alt+F3 and the "View With" menu MUST continue to offer the raw
  source via the internal viewer (host behavior, not modified).
- **FR-004**: Installing mdview MAY take top viewer priority for its masks
  (standard host behavior); this MUST be mentioned in release notes.

### Rendering — supported syntax

- **FR-010**: The renderer MUST implement CommonMark 0.31.2 as the normative
  core: ATX + Setext headings, paragraphs, soft breaks (rendered as spaces)
  and hard breaks (double-space or backslash), emphasis/strong (including
  nesting), ordered/unordered/nested lists (start numbers, tight/loose),
  blockquotes (including nested), thematic breaks, inline code, fenced
  (```/~~~) and indented code blocks, info strings, inline and
  reference links with titles, angle-bracket autolinks, images with alt text,
  backslash escapes, and the full named + numeric HTML entity set.
- **FR-011**: The renderer MUST additionally implement these GFM extensions:
  pipe tables with column alignment, strikethrough, task lists (rendered as
  static, non-interactive checkboxes), and extension autolinks (bare
  http(s)://, www., e-mail).
- **FR-012**: Internal anchors MUST follow the GitHub slug algorithm
  (Unicode-aware lowercase; keep letters/digits/spaces/hyphens/underscores —
  Czech diacritics preserved; spaces→hyphens; duplicates suffixed -1, -2, …).
  A link to a missing anchor is a no-op with a non-modal notice.
- **FR-013**: Code blocks MUST get best-effort lexical syntax highlighting
  for at least: c, cpp, csharp, js, ts, python, json, yaml, xml, html, css,
  shell, batch, powershell, cmake, ini, toml, sql, diff, markdown — with the
  alias table from [analysis/markdown.md](analysis/markdown.md). Unknown or
  absent language → plain monospaced block with the info-string label shown.
  A highlighter failure degrades to plain text and never blocks rendering.
- **FR-014**: **Degradation master rule** — any construct mdview does not
  support MUST remain visible as literal source text (or a plain code block);
  content is never silently dropped. Specific rules: footnotes, definition
  lists, inline math, emoji shortcodes, wiki links, Pandoc attributes →
  literal text; math/Mermaid/diagram fences → plain code block with label;
  GitHub alerts → plain blockquote with the literal `[!NOTE]` tag; YAML front
  matter at byte 0 → rendered as a visually distinct metadata block (never
  misparsed as a Setext heading). Single documented exception: HTML comments
  `<!-- -->` are hidden.
- **FR-015**: Raw HTML tags MUST be rendered as visibly inert literal text
  (HTML entities still decode; no HTML element is ever instantiated). This is
  the v1 policy (clarified); a reviewed safe micro-whitelist (`<br>`, `<b>`,
  `<i>`, `<code>`, `<kbd>`, `<sub>`, `<sup>`, `<s>`, optionally `<img>`/`<a>`
  routed through the same safe pipeline as Markdown) is deferred to a post-v1
  version with its own security review.

### Images

- **FR-020**: Local images referenced by relative path MUST display inline,
  resolved against the document's directory after canonicalization; paths
  with spaces, Unicode characters, and `..` segments included. The resolved
  file is opened read-only as data, never executed.
- **FR-021**: Absolute, drive-letter, UNC, `file:`, device-path, and
  NTFS-ADS image references MUST show the error placeholder and MUST NOT be
  opened or probed.
- **FR-022**: Supported raster formats: PNG, JPEG, GIF, BMP, WebP (platform
  imaging component precedent). SVG MUST be rendered only via a static,
  script-free, non-fetching rasterizer with XML external entities disabled;
  if unavailable, SVG gets the placeholder. Animated formats MAY degrade to
  the first frame.
- **FR-023**: Images wider than the content area MUST downscale to fit with
  preserved aspect ratio (no default upscaling); resize re-fits; images MUST
  NOT cause horizontal scrolling of the document body.
- **FR-024**: Missing/corrupt/unsupported/over-limit images MUST show an
  in-flow placeholder with the alt text and a reason; the document renders on.
- **FR-025**: Remote (http/https) images MUST NOT be fetched when the
  document opens. A placeholder (alt text + host) is shown; an explicit
  per-document, per-session "Load remote images" action fetches them —
  http/https only, redirects confined to http/https, no
  cookies/credentials/custom headers, decode via the same hardened path and
  caps as local images, off the UI thread, with a timeout. No global
  "always allow" default in v1 [deferred decision D2].

### Links

- **FR-030**: No link is resolved, prefetched, or activated without an
  explicit user gesture (mouse click, or Enter on a keyboard-focused link).
- **FR-031**: External activation is allowlist-only: `http`, `https`,
  `mailto` open via the system default handler. Every other scheme —
  `javascript`, `vbscript`, `data`, `file`, `about`, and all unknown/custom
  schemes — is blocked with a visible indication. The decision is made on the
  decoded, canonicalized URL.
- **FR-032**: The real canonicalized target MUST be visible before
  activation (hover tooltip / status surface / focus display).
- **FR-033**: Links MUST be distinguishable by more than color (underline)
  and MUST be keyboard-reachable: Tab/Shift+Tab cycles link focus with a
  visible focus indicator, Enter activates.
- **FR-034**: Links to local Markdown files (registered masks) MUST open the
  target in a **new mdview window** (read-only, all policies and limits
  re-applied), when clicked or activated by keyboard. Links to local
  non-Markdown files are NOT launched in v1: the resolved path is shown with a
  copy-path affordance. UNC/absolute/`file:` local targets remain fully
  blocked. (No confirmation-gated "open with associated application" in v1.)
- **FR-035**: A context-menu "Copy Link Address" MUST be available on links.

### Security invariants (all MUST, all inputs)

- **FR-040**: Opening/rendering a document never executes any script and
  never launches any command, application, or protocol handler.
- **FR-041**: Opening/rendering performs no network I/O of any kind (HTTP(S),
  SMB/UNC, WebDAV, DNS) — including no resolution or probing of remote
  references — without a subsequent explicit user gesture.
- **FR-042**: No HTML event handler ever attaches or fires;
  iframe/object/embed/applet, forms, meta-refresh, and CSS-driven fetches are
  never active under any HTML policy, present or future.
- **FR-043**: Untrusted input of any size, nesting depth, or malformation
  never crashes the application, never blocks the UI thread beyond
  interactive latency, and never exhausts memory: parsing/decoding/layout of
  untrusted bytes runs off the UI thread, is cancelable, and is bounded by
  the resource limits (FR-090…), degrading to an error surface or text-viewer
  fallback.
- **FR-044**: Clipboard is written only on explicit user command, plain text
  (CF_UNICODETEXT) as the primary format; copied content contains no markup.
- **FR-045**: All policies and limits re-apply on every reload/refresh.
- **FR-046**: v1 MUST use a **static, script-free rendering approach** — a
  vendored GPLv2-compatible CommonMark+GFM parser feeding a custom native
  renderer — so that the invariants FR-040…FR-045 hold **by construction** (no
  script engine, no HTML DOM, no network stack exists in the rendering path).
  A browser-engine surface (e.g. WebView2) is out of scope for v1. Should a
  future version reconsider one, that plan MUST enumerate per-invariant
  enforcement and a re-verification strategy across engine updates.

### Encoding

- **FR-050**: Decode order: (1) NUL-containing/binary content → hand off to
  the text/hex viewer path; (2) UTF-8 BOM → UTF-8; (3) UTF-16 LE/BE BOM →
  UTF-16; (4) no BOM + strict whole-buffer UTF-8 validation → UTF-8;
  (5) validation failed → decode via the system ANSI codepage (CP1250 on
  Czech Windows) with a non-modal warning bar naming the assumption and
  offering one-click "Open in text viewer" [deferred decision D3].
- **FR-051**: Czech text MUST render correctly for cases (2)–(4) exactly and
  for case (5) on CP1250 files. CRLF, LF, CR, and mixed endings render
  identically. The BOM is never displayed.
- **FR-052**: Undecodable input never crashes; the text-viewer fallback is
  always reachable.

### Color schemes

- **FR-060**: The viewer MUST ship exactly these named schemes (5 light:
  Paper, Soft Gray, Warm Sepia, Solar Light, Arctic Light; 5 dark: Graphite,
  Midnight, Solar Dark, Nordic Dark, High Contrast Black), each defining the
  full role set: document background; body text; headings; link;
  hover/active link; blockquote text + accent; inline-code fg/bg; code-block
  bg + default text; table border + header bg; horizontal rule; selection
  bg/fg; keyboard focus indicator; image-placeholder text/border; and the
  9-token syntax set (keyword, string, number, comment, type, function,
  operator, diff-added, diff-removed). Visited-link is an optional role
  falling back to Link [deferred decision D7].
- **FR-061**: Contrast gates (unit-testable per scheme × role): body text
  ≥ 4.5:1; large headings ≥ 3:1 (body-size headings 4.5:1); links ≥ 4.5:1
  AND underlined; selection fg ≥ 4.5:1 vs selection bg, selection bg ≥ 3:1
  vs document bg; code text ≥ 4.5:1 vs its own bg + visibly distinct code
  region; non-text elements ≥ 3:1; every syntax token ≥ 4.5:1 — comments
  explicitly included; High Contrast Black ≥ 7:1 everywhere. Design phase
  MUST NOT copy stock Solarized/Nord values that fail these gates (documented
  in [analysis/visual.md](analysis/visual.md)).
- **FR-062**: Scheme selection: View → Color Scheme radio submenu (mouse +
  keyboard), plus scheme-cycling hotkeys (proposal F9/Shift+F9 — F8 is
  reserved by host convention for encoding). Switch applies immediately, no
  reopen, scroll and selection preserved.
- **FR-063**: The chosen scheme persists in the plugin configuration as a
  stable ASCII identifier (never a localized name, never an integer index).
  Corrupt/unknown/missing value → default scheme (light default: Paper; dark
  default: Graphite), no crash, self-heals on next change.
- **FR-064**: An optional "Follow system theme" mode (default off) MAY map
  the OS light/dark state to a designated light + dark scheme, reacting to
  the OS broadcast live; it supplements, never replaces, the 10 schemes
  [deferred decision D5].
- **FR-065**: Schemes style the document canvas only; window chrome, menus,
  scrollbars, and dialogs remain standard system-drawn (constitution VI).
  This is intended behavior, not a defect.

### Viewer controls & window

- **FR-070**: Keyboard map (host conventions win): Esc/Alt+F4 close (F3
  NEVER closes — it is find-next or a no-op); arrows scroll; PgUp/PgDn page;
  Home/End = document start/end (Ctrl+Home/Ctrl+End aliases); wheel per
  system scroll-lines; Shift+wheel horizontal where content overflows;
  Ctrl+A select all; Ctrl+C/Ctrl+Insert copy; Ctrl+R refresh (re-read,
  policies re-applied, position preserved); Shift+F10/Apps context menu;
  Space/Backspace next/previous file in panel (host viewer convention,
  deliberately overriding the browser page-down habit) [deferred decision
  D8]; F11 full screen (optional).
- **FR-071**: Text is selectable with the mouse; selection is visible in
  every scheme; copy yields the text content with Czech characters intact.
- **FR-072**: Window: standard top-level viewer window; title
  `<full path> - Markdown Viewer` (Unicode-correct); placement persisted;
  reflow on resize; menu bar (File/Edit/View/Options/Help) consistent with
  the internal viewer's structure.
- **FR-073**: In-document search (Ctrl+F; F3/Shift+F3 next/previous) MUST ship
  in v1 (the static renderer of FR-046 makes laid-out text enumerable). Zoom
  (Ctrl+wheel, Ctrl+plus/minus/0, ~50–300 %, Ctrl+0 reset, persisted) MUST
  ship in v1.
- **FR-074**: Typography roles: proportional body face, monospaced code
  face, heading hierarchy distinct by size/weight (never color alone), all
  metrics scaling with system DPI (no hardcoded pixel constants), following
  the host's existing DPI-awareness model. Reading measure MAY be capped for
  readability on wide windows [deferred decision D9].
- **FR-075**: Next/previous-file navigation MUST use the host's viewer file
  enumeration so it honors panel ordering and selection.

### Errors & fallback

- **FR-080**: Pre-open: cheap checks only (existence, readability, size
  gate, binary sniff) may decline the file so the host cascades to the next
  configured viewer (internal text viewer). Content is never a reason to
  decline — any byte stream is renderable Markdown.
- **FR-081**: Post-open failures (decode, renderer error, file vanished,
  memory) MUST show an in-window error state naming the file and the
  specific reason, with an explicit "Open in text viewer" action. The host
  provides no fallback after the window opens; the plugin owns every error
  state.
- **FR-082**: Merely malformed Markdown never triggers the error surface
  (best-effort rendering is the contract).
- **FR-083**: Distinct, actionable messages for: file not found, access
  denied, file in use, too large, undecodable, renderer failure. Never a raw
  error code alone.
- **FR-084**: A deterministic failure-injection hook for testing the
  renderer-error path SHOULD exist in debug builds [deferred decision D10].

### Performance & resource limits

- **FR-090**: Performance targets (Release build, F3-to-first-paint):
  typical README (10–50 KB) < 500 ms (target 200 ms); 1 MB document < 2 s
  (fully navigable < 4 s); scheme switch repaint < 300 ms on 1 MB; scrolling
  a 1 MB document shows no perceptible hitch (≥ 100 ms) [deferred decision
  D11 fixes binding vs advisory].
- **FR-091**: Size gate (default 20 MB [deferred decision D4]): larger
  documents are not rendered; the user gets "too large — open as text?".
- **FR-092**: Hard caps MUST exist for: nesting depth (default 64; beyond →
  literal text), parse/layout watchdog (abort ≈ 5 s → error + fallback),
  AST/output size relative to input, decoded image dimensions (~50 MP),
  total decoded image memory (~256 MB), renderer memory (hard abort
  ≈ 512 MB), per-image decode time (~2 s → placeholder). Image decoding is
  lazy/viewport-driven and off the UI thread.
- **FR-093**: The parser MUST have documented linear-time behavior on
  pathological input; deep nesting MUST NOT overflow the stack (bounded
  depth or iterative processing — verified on the Release build).
- **FR-094**: 50 consecutive open/close cycles leak nothing: GDI +10, USER
  +10, handles +20, private memory +5 MB tolerances vs baseline.

### Configuration, persistence & platform integration

- **FR-100**: Plugin configuration persists in the plugin's registry key
  with a schema-version value; every stored value has a default; corrupt or
  missing configuration never crashes and self-heals on next save. Persisted:
  scheme identifier, follow-system-theme flag, per-polarity scheme slots,
  window placement, zoom (if shipped).
- **FR-101**: Configuration changes broadcast to open viewer windows
  (instant re-theme, no reopen).
- **FR-102**: The plugin follows the host viewer plugin contract: viewer +
  configuration capabilities; language module (english); registration via
  the standard install-time mechanism; honoring the temp-file lock handshake
  for archive/SFTP-sourced files exactly for the window lifetime.
- **FR-103**: All file access uses the host's Unicode/long-path conventions:
  UTF-8 paths across the plugin API, converted via the SDK helpers with
  long-path prefixes; **no fixed-size path buffers** (features 011–014
  defect class); Unicode-correct title and rendering (features 005/010/015
  bug class).
- **FR-104**: Constitution compliance: pure WinAPI (IV); no process-wide
  visual side effects — no `ICC_STANDARD_CLASSES`, no plugin manifest, no
  restyling of standard controls; any config dialog uses the house DIALOGEX
  template (VI); all new dependencies vendored, GPLv2-compatible, attributed
  in the third-party notices (no package-manager dependencies).
- **FR-105**: `Release` of the plugin closes all viewer windows and threads
  cleanly; app shutdown with open viewers follows host policy and still
  persists configuration.

## Key Entities

- **Document session**: one opened file — source bytes, decoded text,
  parsed structure, rendered layout, scroll/selection state, per-document
  remote-image consent flag, temp-file lock (when archive/SFTP-sourced).
- **Color scheme**: named, identifier-keyed set of ~15 color roles + 9
  syntax token colors satisfying the contrast gates; 10 shipped instances.
- **Plugin configuration**: versioned registry-persisted settings (scheme
  id, auto-theme flag + polarity slots, window placement, zoom).
- **Viewer registration**: the extension masks (`*.md;*.markdown`) tying F3
  to the plugin, user-overridable in host configuration.

## Success Criteria *(mandatory)*

- **SC-001**: F3 on a GFM fixture opens rendered output — 20/20 brief §17
  acceptance criteria pass via the test catalog in
  [analysis/testing.md](analysis/testing.md) (TC-A01…TC-F07).
- **SC-002**: 100 % of the syntax matrix (20 element classes) renders per
  FR-010…FR-014 on the fixture pack; every unsupported construct in the
  degradation fixtures remains visible as literal text.
- **SC-003**: Czech UTF-8 content (± BOM) renders with zero mojibake in all
  surfaces (headings, body, tables, code, links, title bar).
- **SC-004**: Security: the hostile fixture produces zero script execution,
  zero network I/O, zero handler launches (verified by observation +
  network monitor); all 10 invariants (FR-040…FR-046) demonstrably hold.
- **SC-005**: Remote-image fixture generates no network traffic on open;
  traffic occurs only after the explicit consent action.
- **SC-006**: All 10 schemes pass every contrast gate in FR-061 via an
  automated check (schemes × roles), including AAA for High Contrast Black.
- **SC-007**: Scheme switching is immediate (< 300 ms on 1 MB), preserves
  position, and survives restart; all three corruption variants fall back to
  the default scheme with no crash.
- **SC-008**: Performance targets of FR-090 met on the Release build;
  pathological fixtures never freeze the UI beyond the watchdog and Esc
  always closes.
- **SC-009**: 50× open/close cycle stays within FR-094 tolerances.
- **SC-010**: Long-path (291-char ASCII, ~540-byte Unicode) and
  Unicode-filename (Czech/CJK/emoji) fixtures open and render; `.md` viewed
  from a ZIP archive and from SFTP renders with correct placeholder behavior
  for sibling images.
- **SC-011**: Every error state in FR-083 produces its specific message and
  a working path to a plain-text view; binary `.md` never dead-ends.
- **SC-012**: Debug and Release x64 builds compile cleanly; the plugin
  loads, registers its masks on first install, and appears in Plugin
  Manager; disabling it in the build policy removes it from the shipped set.

## Assumptions

- The brief (`features/mdview-plugin.md`) is the authoritative scope source;
  §16 exclusions (editing, export, print, Mermaid, math, active content,
  remote active media) hold for v1.
- Decided (clarify Q2): a static, script-free rendering approach over a
  vendored GPLv2-compatible CommonMark+GFM parser — supported independently by
  the integration analysis (constitution/licensing/NuGet constraints; in-repo
  history precedent), the security analysis (invariants by construction), and
  the Markdown analysis (feasibility: md4c/cmark-gfm). The specific parser
  choice (md4c vs cmark-gfm) remains plan-phase work.
- Salamander interface version 104 semantics (UTF-8 paths, long-path ABI)
  per architecture/06; the viewer-mask registration happens on genuine first
  install, which includes dropping the built plugin into an existing
  configuration (auto-install mechanism).
- Viewer masks have no self-heal (feature 016 covered archives only);
  accepted, because a lost mask degrades gracefully to the internal text
  viewer.
- English resources (english.slg) are the build/verification language;
  scheme names and UI strings are localizable.
- The existing test infrastructure is reused: long-path trees under
  `%LOCALAPPDATA%\Temp\salamander-test\`, local Docker SFTP server
  (localhost:2222), fixture pack + generator committed with this spec.
- Salamander itself has no application theme; "follow theme" can only mean
  the OS light/dark signal.
- Full back/forward navigation history, PerMonitorV2 DPI, UI Automation
  content exposure, and Windows forced-colors integration are explicitly
  post-v1 (documented gaps), per the UX analysis.

## Decisions Log

The three high-impact questions were resolved in `/speckit.clarify`
(Session 2026-07-18, above); all formal `[NEEDS CLARIFICATION]` markers are
now removed from the requirements:

| # | Question | Location | Resolution |
|---|---|---|---|
| **Q1** | Raw HTML: all-literal vs reviewed micro-whitelist | FR-015 | **All-literal** for v1; whitelist deferred to a reviewed post-v1 version |
| **Q2** | Renderer class / selection-search quality bar | FR-046, FR-073 | **Static script-free renderer** mandated; viewer-grade selection + search |
| **Q3** | Local-file link navigation model | FR-034 | Local `.md` **opens in a new mdview window**; other local files path-only |
| **Q4** | v1 scope of optional controls (search / zoom) | FR-073 | **Both search and zoom** ship in v1 |

Defaults adopted in the spec (confirmed; may still be tuned at plan phase):

| # | Decision | Value |
|---|---|---|
| D1 | Registered extensions | `*.md;*.markdown` (omit `.mdown`/`.mkd`, justified) |
| D2 | Remote images | Blocked; per-document per-session consent; no global always-allow |
| D3 | Legacy encoding fallback | System ANSI codepage (CP1250) + warning bar + text-viewer offer |
| D4 | Size gate | 20 MB |
| D5 | Follow-system-theme mode | Included, default off |
| D6 | Search & zoom in v1 | **Both required in v1** (confirmed by Q4) |
| D7 | Visited-link role | Dropped for v1 (falls back to Link color) |
| D8 | Space/Backspace | Host convention (next/previous file) |
| D9 | Reading measure cap | Capped ~900 px @96 DPI equivalent, "full width" toggle |
| D10 | Debug failure-injection hook | Included (debug builds only) |
| D11 | Performance targets | PR-1/PR-2/PR-4 acceptance-blocking; others advisory |
| D12 | Default schemes | Paper (light), Graphite (dark) |
