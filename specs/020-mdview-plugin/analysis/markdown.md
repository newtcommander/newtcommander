# Agent 3 — Markdown Compatibility Analysis (raw report, specify phase)

Independent analysis. Repo facts verified: `AddViewer` mask registration
(src/plugins1.cpp:636); legacy codepage infra `CCodeTables` (src/codetbl.cpp +
convert/); UTF-8 helpers src/common/salunicode.h (`SalU8ToW`,
`SalNormalizeNFC`); vendored deps in src/common/dep (incl. nanosvg; NO
Markdown parser).

## Findings

1. **Baseline**: CommonMark **0.31.2** as normative core (pinning the version
   matters — delimiter-run/list rules differ across older dialects).
2. **GFM layer**: name the four concrete extensions instead of "the GFM spec"
   (frozen 0.29-gfm, stale 2019): **tables** (pipe + alignment),
   **strikethrough**, **task lists**, **extension autolinks** (bare http(s)://,
   www., e-mail).
3. **Line breaks**: CommonMark default — soft break = space (GitHub *file*
   rendering, the user's mental reference); hard break via double-space or `\`.
4. **Heading anchors** (slugger is renderer-side by design everywhere):
   GitHub algorithm — plain-text content, Unicode-aware lowercase, remove all
   but letters/digits/spaces/hyphens/underscores (**Czech diacritics
   preserved**: `## Úvod a cíle` → `#úvod-a-cíle`), spaces→hyphens, duplicates
   get `-1`, `-2`. Missing anchor: **no-op scroll + non-modal status notice**
   (never modal, never external-handler fallthrough); optional case-insensitive
   retry. Pandoc `{#custom}` OUT (renders literally). `other.md#anchor` owned
   by the cross-file-navigation decision.
5. **Syntax highlighting = best-effort lexical** (not semantic). Tier-1:
   c, cpp, csharp, js, ts, python, json, yaml, xml, html, css, shell, batch,
   powershell, cmake, ini, toml, sql, diff, markdown. Alias table: c++→cpp,
   cs→csharp, javascript→js, typescript→ts, py→python, yml→yaml,
   sh|bash|zsh|console→shell, bat|cmd→batch, ps1|ps→powershell, patch→diff,
   text|plaintext|txt→plain. Unknown/absent language → plain monospaced block
   **with the info-string label displayed**. Highlighter failure degrades to
   plain text, never blocks rendering.
6. **Encoding infrastructures**: modern salunicode.h (UTF-8↔UTF-16, NFC) = the
   natural pipeline; legacy CCodeTables + convert/ (CP1250, ISO-8859-2) with
   heuristic recognition backs the internal text viewer — which is the natural
   fallback target, so mdview need not replicate codepage UI.
7. **Registration mechanics**: one `AddViewer("*.md;...", FALSE)` at install
   (pattern: dbviewer.cpp:581, pictview.cpp:1037); users can extend the mask
   later in viewer config, weakening the case for preregistering rare
   extensions.
8. **One deliberate silent-loss exception**: HTML comments `<!-- -->` hidden
   (matches GitHub/VS Code; non-content); document explicitly; "open source"
   escape hatch covers inspection.

## Proposed syntax matrix for v1 (all IN unless noted)

| # | Element | v1 | Note |
|---|---------|----|------|
| 1 | Headings ATX+Setext | IN | feeds anchors |
| 2 | Paragraphs, soft/hard breaks | IN | soft=space |
| 3 | Bold/italic (incl. nesting) | IN | CM 0.31 rules |
| 4 | Strikethrough `~~` | IN (GFM) | |
| 5 | Ordered+unordered lists | IN | start numbers, tight/loose |
| 6 | Nested lists | IN | |
| 7 | Task lists | IN (GFM) | static, non-interactive |
| 8 | Blockquotes incl. nested | IN | `[!NOTE]` degrades (below) |
| 9 | Horizontal rules | IN | |
| 10 | Inline code | IN | |
| 11 | Fenced (```/~~~) + indented code | IN | verbatim content |
| 12 | Info string on fences | IN | first word = language |
| 13 | Syntax highlighting | IN, degraded by design | tier-1 list |
| 14 | Links (inline, reference, titles) | IN | badges use reference links |
| 15 | Autolinks (core `<>` + GFM bare) | IN | |
| 16 | Images (inline+reference, alt) | IN | policy per §6/security |
| 17 | GFM pipe tables + alignment | IN | cells = inline content only |
| 18 | Backslash escapes | IN | |
| 19 | Internal anchor links | IN | slugger per Finding 4 |
| 20 | HTML entities (named+numeric) | IN | |

Single-parser deliverable; only slugger + highlighter are custom by design.

## Degradation rules

**Master rule**: unsupported construct stays *visible as literal source text* —
never dropped, never half-rendered. Forms: **L** = literal text, **C** = plain
code block.

| Construct | Rule |
|---|---|
| Footnotes `[^1]` | OUT → L (automatic) |
| Definition lists | OUT → L (GitHub doesn't render either) |
| Math `$…$` / ```math | OUT → L / C (source readable) |
| Mermaid/diagram fences | OUT → C with label visible |
| YAML front matter (byte 0) | degraded-IN → distinct metadata code block (else CM renders `---` as Setext/thematic break — misleading) |
| GitHub alerts `> [!NOTE]` | OUT → plain blockquote with literal `[!NOTE]` (cheap v1.x upgrade) |
| Emoji shortcodes `:tada:` | OUT → L |
| Wiki links `[[Page]]` | OUT → L |
| Pandoc attrs `{#id}` | OUT → L |
| HTML comments | Hidden (documented exception) |
| Raw HTML | split rule — see below; coordinate with security |

**Raw HTML proposal (this agent)**: small **inert whitelist** mapped onto the
native renderer (never a browser engine): `<br>` → break; `<b>/<strong>`,
`<i>/<em>`, `<code>`, `<kbd>`, `<sub>`, `<sup>`, `<s>/<del>` → native style;
optionally `<img>`/`<a href>` routed through the SAME pipeline as Markdown
images/links; everything else (script, iframe, div, table, unknown, all other
attributes) → L as escaped, dimmed literal text. No handlers/CSS/execution;
attributes other than href/src/alt/align dropped. Fallback position "all raw
HTML → L" is acceptable and safe (cost: ugly headers on modern READMEs).
**→ Open Question 1 (co-sign with security agent).**

## Encoding policy proposal

1. NUL-byte/binary sniff (no UTF-16 BOM) → binary; text/hex viewer fallback.
2. UTF-8 BOM → strip, decode. **Mandatory.**
3. UTF-16 LE/BE BOM → decode. **Recommend IN** (PowerShell 5.1 `>` emits
   UTF-16 LE — real on target OS). BOM-less UTF-16 OUT.
4. No BOM → strict whole-buffer UTF-8 validation → valid = UTF-8 (ASCII passes
   automatically; cannot misfire).
5. Validation failed → decode via **system ANSI codepage (CP_ACP = CP1250 on
   Czech Windows)** — maps every byte, cannot fail — plus non-modal warning bar
   "not valid UTF-8 — shown as Windows-1250 [Open in text viewer]". Rejected
   alternative: U+FFFD lossy replace (destroys every Czech diacritic in CP1250
   files — worse for this user). OUT of v1: heuristic multi-codepage detection
   and per-file encoding menu (text viewer already has it).
6. Czech guarantee: steps 2–4 exact; step 5 correct for CP1250. Use
   salunicode.h helpers. CRLF/LF/CR all accepted. BOM never rendered.

## Extension registration proposal

- `*.md` — mandatory; dominant by a wide margin.
- `*.markdown` — **recommended IN**: original long-form, second GitHub Linguist
  default, Jekyll scaffolding; one mask token cost.
- `*.mdown`, `*.mkd` — **recommended OUT**: vestigial pre-standardization,
  effectively absent post-2015; polluting the association table for no benefit;
  user can add manually in viewer config.
- Never: `.mdx` (JSX), `.rmd`/`.qmd` (literate), `.mdtxt`/`.mdtext`.
- Mechanics: `AddViewer("*.md;*.markdown", FALSE)`.

## Feasibility note

**md4c** (MIT; single md4c.c/.h, zero deps, CommonMark 0.31.2-compliant,
fuzz-tested, powers Qt Markdown; GFM flags MD_FLAG_TABLES/STRIKETHROUGH/
TASKLISTS/PERMISSIVE*AUTOLINKS; SAX callbacks suit a direct-to-native renderer
with no HTML intermediate — simplifies security). Alternative **cmark-gfm**
(BSD-2; adds footnotes) was archived/deprecated by GitHub — maintenance
liability. Vendoring matches src/common/dep policy; nanosvg incidentally
covers §6 SVG.

## Open questions (ranked)

1. **Raw HTML policy**: inert whitelist vs all-literal (product impact:
   modern README headers) — co-signed with security.
2. **Legacy-encoding fallback**: CP_ACP+warning bar (proposed) vs strict
   UTF-8-only + text-viewer deferral.
3. **Highlighting scope**: confirm tier-1 list + contractual best-effort
   definition (bounds acceptance criterion §17.5).
4. **Extension set**: confirm `*.md;*.markdown` and deliberate omission of
   `.mdown`/`.mkd` (§4 requires accepted justification).
5. **Near-miss degradations**: confirm OUT-with-degradation for footnotes,
   alerts, front matter — or pull alerts/front-matter styling into v1 (cheap);
   footnotes are NOT cheap with md4c.
