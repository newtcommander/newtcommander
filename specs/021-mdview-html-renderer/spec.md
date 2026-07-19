# Feature Specification: mdview HTML Rendering Surface

**Feature Branch**: `021-mdview-html-renderer`
**Created**: 2026-07-19
**Status**: Draft
**Input**: User description: "Nacti pripravenou analyzu z specs/020-mdview-plugin/analysis/html-renderer.md, resp. soucasneho kontextu a zahaj pripravu" (Load the prepared analysis and begin preparation — evolve the mdview viewer to render Markdown through an HTML rendering surface for richer, faithful output.)

**Origin**: This feature operationalizes the technical analysis in
[`specs/020-mdview-plugin/analysis/html-renderer.md`](../020-mdview-plugin/analysis/html-renderer.md),
which evaluated replacing the current RTF/RichEdit rendering path of the
mdview plugin (feature 020) with an HTML-based rendering surface. It is the
"v2" evolution of the mdview viewer; the parser/rendering internals change,
the user-facing plugin (F3 Markdown viewer) stays the same product.

---

## Overview

The mdview plugin (feature 020) renders Markdown files on F3. Its v1
rendering path (hand-written parser → RTF → RichEdit control) reached a
fidelity ceiling: tables are drawn only as monospaced ASCII text, body text
is not inset from the window edges, images appear as placeholders, and
embedded raw HTML is shown as literal source text. This feature replaces the
rendering surface so that Markdown is displayed with publication-quality
fidelity — real tables, comfortable margins, quality typography, inline
images, and faithful display of embedded HTML — while **preserving every
existing viewer behavior and every security guarantee**, keeping the build
fully self-contained, and laying a foundation for later enrichment
(diagrams, math, export).

---

## Clarifications

### Session 2026-07-19

- Q: Rendering-surface direction — browser-class OS HTML engine vs. in-tree static engine? → A: Browser-class OS HTML engine (WebView2 / Evergreen, a Windows 11 OS component). Security invariants are enforced by configuration + re-verification test (not by construction); the FR-070 Q2 amendment is therefore required. The exact SDK/loader and integration remain plan-phase details; the in-tree static engine (litehtml) is retained only as a recorded fallback.
- Q: Embedded raw-HTML policy — render all natively, safe subset whitelist, or defer? → A: Render all embedded HTML natively; safety comes solely from the engine lockdown (scripts off, network default-deny, navigation/forms/iframes blocked). No HTML sanitizer/whitelist (a sanitizer is a larger, historically weaker trusted surface). This fixes the wording of the FR-070 Q1 amendment.
- Q: Fallback when the HTML rendering surface cannot initialize — single surface + text viewer, or keep the v1 RTF/RichEdit renderer as a second backend? → A: Single HTML rendering surface. On init failure show a clear error state and hand off to the internal text viewer; the v1 RTF/RichEdit renderer is removed entirely (one rendering backend to maintain and test).

---

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Faithful rendering of rich Markdown (Priority: P1)

A user presses F3 on a `README.md` that contains headings, nested lists,
fenced code, blockquotes, and GitHub-style tables. The document is displayed
as a well-formatted page: tables appear as real grids with column alignment
and cell borders, body text is inset from the window edges with a
comfortable reading width, headings and code blocks have clear typographic
hierarchy, and the whole page scrolls smoothly.

**Why this priority**: This is the core motivation for the feature and the
minimum viable product. Faithful rendering of standard Markdown — especially
tables and margins — is the primary user complaint the feature exists to fix.
Delivered alone (even without images or embedded HTML), it already makes the
viewer substantially better than v1.

**Independent Test**: Open a document containing a GFM table and multi-level
content; verify the table renders as a bordered grid with correct alignment
and the body text is not touching the window edges, at several window widths.

**Acceptance Scenarios**:

1. **Given** a Markdown file with a GFM pipe table using `:---`, `:---:`,
   `---:` alignment markers, **When** the user opens it on F3, **Then** the
   table is displayed as a visual grid whose columns are left/center/right
   aligned according to the markers.
2. **Given** any Markdown document, **When** it is displayed, **Then** the
   text content is inset from the window edges and constrained to a readable
   line length rather than stretching edge-to-edge.
3. **Given** a document with fenced code containing a supported language,
   **When** displayed, **Then** the code appears in a monospaced block with
   syntax highlighting consistent with the active color scheme.
4. **Given** headings, ordered/unordered/nested/task lists, blockquotes, and
   thematic breaks, **When** displayed, **Then** each is rendered with
   correct structure and spacing (checkboxes shown for task lists, nested
   quotes indented, etc.).

---

### User Story 2 - Safe viewing of untrusted documents (Priority: P1)

A user opens a Markdown file obtained from an untrusted source (email
attachment, downloaded archive, unknown repository). The document renders as
formatted text and nothing else happens: no script runs, no network request
is made, no application or protocol handler launches, and no file on disk is
read except the explicitly relative resources the document legitimately
references.

**Why this priority**: The viewer must be safe to point at hostile input.
The v1 security invariants (feature 020, `analysis/security.md`) are
non-negotiable and must continue to hold under the new rendering surface —
whether guaranteed "by construction" or "by configuration plus test". A
rendering upgrade that weakened safety would be unacceptable.

**Independent Test**: Open an adversarial corpus (documents embedding
`<script>`, `onerror=` handlers, `javascript:` links, remote image/beacon
references, forms, meta-refresh, iframes, path-traversal resource
references) with a network monitor attached; verify zero script execution,
zero unsolicited network I/O, and zero out-of-directory file access.

**Acceptance Scenarios**:

1. **Given** a document embedding a `<script>` block or an element with an
   inline event handler, **When** opened, **Then** no script executes and no
   handler fires.
2. **Given** a document referencing a remote image or beacon URL, **When**
   opened, **Then** no network request is made until the user gives explicit
   per-document consent.
3. **Given** a document with a link to a non-web scheme (`file:`, `ftp:`, a
   custom scheme) or an absolute/UNC path, **When** the user activates it,
   **Then** it is blocked (no launch), and only http/https/mailto links are
   opened, and only on an explicit gesture.
4. **Given** a maliciously large, deeply nested, or malformed document,
   **When** opened, **Then** the viewer does not crash, does not hang the UI,
   and does not exhaust memory — degrading to an error or to the text-viewer
   fallback.

---

### User Story 3 - Inline images (Priority: P2)

A user opens a Markdown document that references local images with relative
paths (screenshots, diagrams, logos). The images are displayed inline at
their natural place in the document. Images referenced from remote URLs are
not fetched automatically; instead a placeholder is shown together with an
offer to load remote content for this document, naming the host(s) involved.

**Why this priority**: Inline images are common in real-world READMEs and
documentation and are a visible gap in v1 (placeholders only). They build on
US1 but are a distinct, separately shippable slice.

**Independent Test**: Open a document with a relative-path local image and a
remote-URL image; verify the local image renders inline and the remote image
shows a placeholder plus a consent affordance; after consenting, verify the
remote image loads.

**Acceptance Scenarios**:

1. **Given** a document referencing a local image by relative path under the
   document's directory, **When** opened, **Then** the image is displayed
   inline.
2. **Given** a document referencing a remote (`http`/`https`) image, **When**
   opened, **Then** no request is made and a placeholder with a consent
   affordance naming the remote host is shown.
3. **Given** the user grants remote-content consent for the document, **When**
   the view reloads or refreshes, **Then** the remote images load and the
   consent applies for that document/session only (no global always-allow).
4. **Given** a document referencing an image by absolute path, UNC path, or
   a path escaping the document directory, **When** opened, **Then** the
   image is refused (placeholder only), never loaded.

---

### User Story 4 - Faithful display of embedded HTML (Priority: P2)

A user opens a Markdown document that embeds raw HTML — common formatting
elements (`<kbd>`, `<sub>`, `<sup>`, `<br>`, `<b>`, `<code>`), an HTML table,
or a small block of markup that GitHub would render. Instead of showing the
markup as literal source text (v1 behavior), the viewer displays the HTML as
formatted content, while still honoring all safety guarantees (no scripts,
no active content, no network without consent).

**Why this priority**: Many real-world Markdown files embed HTML for
formatting that plain Markdown cannot express. Displaying it as literal text
(v1) is a visible fidelity gap. This depends on the new rendering surface and
is a distinct slice from US1.

**Independent Test**: Open a document containing embedded formatting HTML and
an HTML table; verify they render as formatted content, and open the
adversarial corpus (US2) to verify active/script content embedded as raw
HTML remains inert.

**Acceptance Scenarios**:

1. **Given** a document with inline HTML formatting elements (e.g. `<kbd>`,
   `<sub>`), **When** opened, **Then** they are displayed as formatted
   content, not as literal `<kbd>…</kbd>` text.
2. **Given** a document with an embedded HTML table or block, **When**
   opened, **Then** it renders as structured content.
3. **Given** a document with embedded HTML that contains scripts, event
   handlers, forms, iframes, or remote references, **When** opened, **Then**
   the formatting is rendered but the active/network parts remain inert
   (consistent with US2).

---

### User Story 5 - No regression of existing viewer behavior (Priority: P1)

A user who relied on the v1 viewer finds every existing capability still
present and working: in-viewer search, zoom with persistence, the ten color
schemes with follow-system and F9 cycling, internal anchor navigation, local
`.md` links opening in a new viewer window, external links opening on a
gesture, correct handling of file encodings, long paths, the large-file
gate, and the "open as text" fallback.

**Why this priority**: The feature is a rendering-surface replacement, not a
new product. Shipping it with any loss of existing function would be a
regression users would immediately feel. Parity is a hard, cross-cutting
requirement that must hold for every slice.

**Independent Test**: Run the v1 feature-parity checklist (search, zoom+
persist, schemes+follow-system+F9, anchors, `.md`→new window, external link
gesture, encoding detection, long paths, size gate, Ctrl+U text fallback,
window/thread lifecycle) against the new build; verify each item behaves as
in v1.

**Acceptance Scenarios**:

1. **Given** any rendered document, **When** the user invokes find
   (Ctrl+F) and next/previous (F3/Shift+F3), **Then** matches are located
   with wrap-around as before.
2. **Given** a rendered document, **When** the user zooms (Ctrl+Plus/Minus/0
   or Ctrl+mouse-wheel), **Then** content scales within the supported range
   and the level persists across documents and sessions.
3. **Given** the ten color schemes, **When** the user cycles them (F9) or the
   system theme changes with follow-system enabled, **Then** the view
   re-colors correctly and the choice persists.
4. **Given** a link to another local `.md`/`.markdown` file, **When**
   activated, **Then** it opens in a new viewer window; a non-Markdown local
   target shows its resolved path only (not launched); an internal `#anchor`
   scrolls to the target heading.
5. **Given** files encoded as UTF-8 (with/without BOM), UTF-16 LE/BE, or
   legacy ANSI, and files in very long (`>MAX_PATH`) directories, **When**
   opened, **Then** text decodes correctly and the file opens without error.
6. **Given** a file larger than the size gate or one the viewer cannot render,
   **When** opened, **Then** the user is offered / handed off to the internal
   text viewer.

---

### User Story 6 - Foundation for future enrichment (Priority: P3)

A maintainer wants to later add rendered enhancements — diagram rendering
(e.g. Mermaid), mathematical notation, a table of contents, copy-to-clipboard
on code blocks, or document export — without re-architecting the viewer. The
new rendering surface makes such additions incremental rather than requiring
another rewrite.

**Why this priority**: Extensibility is a strategic driver stated in the
feature brief, but no specific enrichment ships in this feature. It is a
design quality, verified by architecture review, not an end-user deliverable
here.

**Independent Test**: Architecture review confirms that adding a new rendered
enhancement (e.g. a diagram type or a TOC) is a localized change to the
HTML/stylesheet generation layer and does not require replacing the rendering
surface or the parser.

**Acceptance Scenarios**:

1. **Given** the shipped rendering pipeline, **When** a maintainer prototypes
   a new rendered enhancement, **Then** it is achievable by extending the
   HTML/stylesheet generation without changing the viewer window, threading,
   or file-handling layers.

---

### Edge Cases

- **Malformed / adversarial input**: unterminated code fences, pathological
  nesting, gigantic tables, invalid UTF-8 mid-file, binary content with a
  Markdown extension — must degrade safely (US2), never crash or hang.
- **Missing resources**: referenced local images that do not exist (common
  when viewing a `.md` extracted from an archive as a temporary copy) — show
  a placeholder, not an error dialog.
- **Rendering surface unavailable**: if the HTML rendering surface cannot
  initialize on the user's machine (runtime disabled by policy, corrupt, or
  below the minimum supported version), the viewer must show a clear error
  state and hand off to the internal text viewer, never leaving a blank or
  broken window. There is no secondary in-process renderer — the v1
  RTF/RichEdit path is removed.
- **Focus and keyboard**: all viewer accelerators (find, zoom, scheme cycle,
  open-as-text, close) must continue to work even though input focus lives
  inside the rendering surface.
- **Very wide content**: tables/code wider than the window must scroll
  horizontally within their own region without breaking the page layout.
- **Theme switching mid-view**: cycling schemes or a follow-system theme
  change must preserve the reading position as closely as practical.
- **Many open windows**: opening many viewer windows must not exhaust
  resources; per-document resources must be fully released on close.

---

## Requirements *(mandatory)*

### Functional Requirements — Rendering fidelity

- **FR-001**: The viewer MUST render GitHub-Flavored Markdown (CommonMark
  plus tables, strikethrough, task lists, and autolinks) with visual
  fidelity comparable to common Markdown viewers.
- **FR-002**: Tables MUST render as visual grids with cell borders and with
  per-column left/center/right alignment honored from the separator row.
- **FR-003**: Body content MUST be inset from the window edges and
  constrained to a comfortable reading measure, with an option to use the
  full window width.
- **FR-004**: Headings, paragraphs, ordered/unordered/nested lists, task
  lists (with checkboxes), blockquotes (including nesting), fenced code
  blocks, inline code, thematic breaks, and emphasis/strong/strikethrough
  MUST each render with correct structure and typography.
- **FR-005**: Fenced code blocks MUST display syntax highlighting for the
  supported language set, colored consistently with the active scheme.
- **FR-006**: Headings MUST expose stable anchor identifiers (slugs) enabling
  internal `#anchor` navigation, preserving the v1 slug algorithm (including
  non-ASCII/diacritic handling).

### Functional Requirements — Images

- **FR-010**: Local images referenced by relative paths resolving under the
  document's directory MUST render inline for formats decodable by the
  platform image codecs.
- **FR-011**: Remote (`http`/`https`) image references MUST NOT be fetched on
  open; a placeholder with an explicit per-document consent affordance naming
  the remote host(s) MUST be shown instead.
- **FR-012**: Remote-content consent MUST be scoped to the document/session
  only; there MUST NOT be a global "always allow remote content" setting in
  this feature.
- **FR-013**: Image references using absolute paths, UNC paths, device paths,
  `file:` URLs, or paths escaping the document directory MUST be refused and
  never loaded.
- **FR-014**: Missing local image resources MUST degrade to a placeholder,
  not an error dialog.
- **FR-015**: SVG, if rendered, MUST use a static, script-free, non-fetching
  renderer (external entities / DTD disabled); otherwise a placeholder MUST
  be shown.

### Functional Requirements — Embedded HTML

- **FR-020**: Embedded raw HTML (block and inline) in Markdown MUST render as
  formatted content rather than literal source text.
- **FR-021**: Rendering embedded HTML MUST NOT enable any active behavior:
  scripts, event handlers, forms, meta-refresh, iframes/objects/embeds, and
  remote references embedded via raw HTML MUST remain inert under the same
  guarantees as Markdown-authored content (see Security Invariants).
- **FR-022**: Embedded HTML MUST NOT be filtered through an HTML
  sanitizer/whitelist; the full set of embedded HTML is rendered and its
  safety derives solely from the engine lockdown (FR-050..FR-057). Rationale:
  a sanitizer is a larger and historically weaker trusted surface (mXSS,
  parser-differential bugs) than the lockdown it would supplement.

### Functional Requirements — Feature parity (no regression)

- **FR-030**: In-viewer search MUST be available (open find, find
  next/previous, wrap-around) over the rendered document.
- **FR-031**: Zoom MUST be available across the v1 range with the v1 input
  bindings (Ctrl+Plus/Minus/0 and Ctrl+mouse-wheel), and the zoom level MUST
  persist across documents and sessions.
- **FR-032**: The ten color schemes (five light, five dark, WCAG-validated)
  MUST be preserved unchanged in appearance, cycleable (F9), with the
  follow-system option and persisted selection.
- **FR-033**: Local `.md`/`.markdown` links MUST open in a new viewer window;
  other local targets MUST show their resolved path only (never launched);
  internal `#anchor` links MUST scroll to the target.
- **FR-034**: External links MUST require an explicit gesture and MUST be
  restricted to an allowlist of `http`, `https`, and `mailto` (the v1 `ftp`
  entry is dropped); all other schemes MUST be blocked.
- **FR-035**: File encoding detection MUST be preserved (UTF-8 with/without
  BOM, UTF-16 LE/BE, strict UTF-8 validation, legacy ANSI fallback), and the
  detected encoding MUST be indicated to the user.
- **FR-036**: Long paths (beyond `MAX_PATH`) MUST be supported with no
  fixed-size path truncation.
- **FR-037**: A large-file gate MUST remain, offering the internal text
  viewer for files above the threshold.
- **FR-038**: A user-initiated "open as text" fallback (Ctrl+U) and an
  automatic fallback for content the viewer cannot render (e.g. binary) MUST
  hand off to the internal text viewer.
- **FR-038a**: The plugin MUST ship a single rendering backend (the HTML
  surface). The v1 RTF/RichEdit renderer MUST be removed; when the HTML
  surface cannot initialize, the viewer MUST show a clear error state and
  hand off to the internal text viewer (there is no secondary in-process
  renderer).
- **FR-039**: The per-document viewer window, its own thread and lifecycle,
  window placement persistence, configuration keys, and configuration-change
  broadcast MUST be preserved.
- **FR-040**: All viewer accelerators MUST continue to function even though
  input focus is inside the rendering surface (find, zoom, scheme cycle,
  open-as-text, close, find next/previous).
- **FR-041**: Selecting text and copying it MUST be available, producing
  plain text only, on an explicit user command.

### Functional Requirements — Security invariants (non-negotiable)

These restate the feature-020 security invariants and MUST hold under the new
rendering surface, demonstrated either by construction or by explicit
configuration plus test (see Assumptions re: enforcement strategy).

- **FR-050**: Opening or rendering a document MUST NOT execute any script,
  under any input.
- **FR-051**: Opening or rendering MUST NOT launch any command, application,
  or protocol handler (link activation excepted, per FR-034, and only on an
  explicit gesture).
- **FR-052**: Opening or rendering MUST NOT perform any network I/O without a
  subsequent explicit user gesture (no fetch, probe, prefetch, or name
  resolution triggered by content).
- **FR-053**: No HTML event handler may attach or fire; forms, meta-refresh,
  and iframe/object/embed/applet MUST never be active under any HTML policy.
- **FR-054**: Local resource access from content MUST be read-only,
  relative-path-only, canonicalized against the document directory;
  absolute/UNC/device/`file:` targets refused; nothing referenced is executed.
- **FR-055**: Untrusted input of any size, nesting, or malformation MUST NOT
  crash the viewer, hang the UI thread, or exhaust memory; parsing/layout MUST
  be bounded and cancelable, degrading to an error or text-viewer fallback.
- **FR-056**: The clipboard MUST be written only on explicit user command and
  only as plain text.
- **FR-057**: All policies and limits MUST re-apply on every reload/refresh
  of a document.

### Functional Requirements — Build & platform constraints

- **FR-060**: The build MUST remain fully self-contained: any additional
  libraries required MUST be vendored into the repository (as source, or as
  committed, license-cleared, version-pinned binaries), with no package
  manager and no network access required at build time.
- **FR-061**: A clean checkout MUST build with the project's standard single
  command on a machine that has only the standard toolchain installed (no
  additional downloads), for both supported architectures.
- **FR-062**: Any added dependency MUST be license-compatible with the
  project license, with its notice recorded in the third-party notices file.
- **FR-063**: The feature targets Windows 11 and newer; it MAY rely on
  capabilities that ship as part of that platform, provided FR-060/FR-061 hold
  and the runtime dependency (if any) is not something the project must
  distribute.

### Functional Requirements — Governance

- **FR-070**: Before implementation, the feature-020 clarification decisions
  that this feature supersedes MUST be formally amended: (a) the
  rendering-surface policy — Q2's "a browser engine is NOT used in v1" is
  superseded by adopting a browser-class OS HTML engine; and (b) the raw-HTML
  policy (from "inert literal" to "rendered, with active content inert"). The
  amendment record MUST state the per-invariant enforcement and
  re-verification strategy for the security invariants under the chosen
  surface.

### Functional Requirements — Performance & robustness

- **FR-080**: A typical documentation file MUST open and display its first
  rendered content within a small, interactive latency budget on the target
  platform (target: under ~1 second for a typical README; warm re-open
  faster).
- **FR-081**: Parsing, decoding, and layout of untrusted content MUST NOT
  block the UI thread beyond interactive latency; long operations MUST be
  bounded and cancelable with a text-viewer fallback.
- **FR-082**: Per-document resources MUST be fully released when a viewer
  window closes; repeated open/close cycles MUST NOT leak (verified over a
  many-cycle test).

### Key Entities

- **Markdown document**: the input file; has detected encoding, a source
  directory (for resolving relative resources), and a size (subject to the
  gate).
- **Rendered view**: the visual output; carries the active color scheme, zoom
  level, scroll position, and per-document consent state.
- **Color scheme**: one of ten named light/dark palettes with body and
  syntax colors; selected/persisted by a stable identifier, cycleable,
  follow-system aware.
- **Image resource**: a referenced image; classified as local-relative
  (renderable), remote (consent-gated), or refused (absolute/UNC/escaping).
- **Link**: a reference classified as internal anchor, local `.md` (new
  window), other local (path-only), external-allowlisted (gesture-launched),
  or blocked.
- **Consent state**: per-document/session flag governing whether remote
  content may load; never global.

---

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: For a representative set of documents containing GFM tables,
  100% render as visual grids with column alignment matching the source
  markers (v1 rendered 0% as grids).
- **SC-002**: For any window width, rendered body text is inset from the
  window edges and constrained to a readable measure — verified visually
  across a range of widths.
- **SC-003**: For a representative set of documents with local relative-path
  images, 100% display the images inline; remote-image documents show a
  placeholder with a host-named consent affordance and load only after
  consent.
- **SC-004**: For a curated set of documents embedding common formatting HTML,
  the embedded HTML displays as formatted content (not literal text) in 100%
  of cases.
- **SC-005**: The v1 feature-parity checklist (search, zoom+persistence, ten
  schemes + follow-system + F9, anchors, `.md`→new-window links, external-link
  gesture, encoding detection, long paths, size gate, text fallback, window
  lifecycle) passes with zero regressions.
- **SC-006**: Across the adversarial corpus, there are zero script
  executions, zero content-triggered network requests before consent, zero
  out-of-directory file accesses, and zero launches of non-allowlisted link
  targets.
- **SC-007**: Across malformed/oversized/deeply-nested inputs, there are zero
  crashes, zero UI hangs beyond interactive latency, and graceful degradation
  (error or text-viewer fallback) in 100% of cases.
- **SC-008**: A typical documentation file opens to first rendered content
  within the interactive latency budget (target under ~1 second on the target
  platform).
- **SC-009**: Repeated open/close of viewer windows over a many-cycle test
  shows no resource leak (per-document resources fully released).
- **SC-010**: A clean checkout builds with the standard single command on a
  toolchain-only machine (no network), for both supported architectures.

---

## Assumptions

- **Read-only viewer**: Markdown editing, authoring, and export remain out of
  scope; this feature only changes how documents are displayed.
- **Evolves feature 020**: the feature reuses the existing mdview plugin
  infrastructure — the per-document viewer window and its thread/lifecycle,
  configuration storage, encoding detection, long-path handling, file masks
  (`*.md;*.markdown`), and the text-viewer fallback. Only the parser and
  rendering surface are replaced.
- **Rendering approach — browser-class OS HTML engine (direction locked in
  clarify, Session 2026-07-19)**: the technical analysis
  (`analysis/html-renderer.md`) evaluated four options; the chosen direction
  is an HTML pipeline (Markdown → HTML → rendering surface) rendered by a
  browser-class HTML engine that ships as a Windows 11 OS component (WebView2
  / Evergreen the leading concrete candidate). The exact SDK/loader and
  integration are plan-phase details. An in-tree static HTML engine
  (litehtml) is retained only as a recorded fallback should the plan phase
  surface a blocking issue. The Markdown → HTML generation layer (parser,
  stylesheet/theme generation, slug/anchor generation, highlight mapping) is
  engine-independent and can be built and tested before the engine
  integration.
- **Security enforcement is by configuration + test** (consequence of the
  browser-class engine): each security invariant becomes enforced engine
  configuration that must be verified and re-verified across engine updates,
  rather than guaranteed by construction. The plan MUST include a
  per-invariant enforcement table and a re-verification strategy (a debug
  self-test corpus run on engine version changes), per FR-070 and feature-020
  `analysis/security.md`; the lockdown table is already drafted in
  `analysis/html-renderer.md` §4.1.5.
- **Governance prerequisite**: implementation is gated on ratifying the
  amendments in FR-070 (feature-020 Decisions Q1 raw-HTML and Q2
  rendering-surface). The rendering-surface direction (browser-class OS
  engine) is confirmed in this clarify session, so the Q2 amendment is
  definitely required; the formal amendment text is proposed in
  `analysis/html-renderer.md` §9 and is to be
  confirmed in the clarify phase.
- **Embedded-HTML safety via engine lockdown, not sanitization** (confirmed
  in clarify, Session 2026-07-19): all embedded HTML is rendered natively and
  kept safe by locking down the rendering surface (scripts off, network
  default-deny, navigation blocked), rather than by an HTML
  sanitizer/whitelist (which the security analysis flags as the historically
  weakest link). See FR-022.
- **Self-contained build is mandatory** (project constitution I): added
  libraries are vendored; on Windows 11 the feature may depend on an HTML
  capability that ships as an OS component, since that is not a library the
  project must distribute — analogous to other system libraries.
- **Extensibility is a design goal, not a deliverable here**: diagram
  rendering (Mermaid), mathematical notation, table of contents,
  copy-code affordances, and document export are explicitly out of scope for
  this feature; the feature only ensures they become incrementally feasible.
- **Language of viewer content**: rendering is content-driven; the plugin UI
  language follows the existing localization model (English resource module
  today).

## Dependencies

- **Feature 020 (mdview plugin)**: this feature evolves it; the plugin,
  its registration, masks, and infrastructure are prerequisites (present).
- **Owner ratification of FR-070 amendments** (feature-020 Decisions Q1/Q2)
  before implementation begins.
- **Analysis document** `specs/020-mdview-plugin/analysis/html-renderer.md`:
  the technical basis for the plan phase (options matrix, per-invariant
  lockdown table, build/vendoring appendix, open questions OQ-1..OQ-9).
