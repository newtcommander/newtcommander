# Contract: Markdown → HTML generator (htmlgen)

Engine-independent. Pure transformation; no I/O, no network. Unit-testable in
isolation (golden fixtures).

## Entry point
```cpp
// htmlgen.h
struct MdHtmlResult {
    std::string html;                       // full UTF-8 HTML document
    std::vector<MdImageRef> images;
    std::vector<std::wstring> anchors;      // heading slugs
    size_t bytesOut = 0;
};

// Renders UTF-8 Markdown 'md' with 'theme' into 'out'. 'docDir' (UTF-16,
// long-path) classifies image refs. 'findTerm' (optional) wraps matches in
// <mark id="mdfind-N">. Never throws; bounded by 'lim'.
bool MdRenderHtml(const std::string& mdUtf8, const MdTheme& theme,
                  const std::wstring& docDir, MdHtmlResult& out,
                  const std::wstring& findTerm = L"",
                  const MdRenderLimits& lim = MdRenderLimits());
```

## Guarantees
- **Well-formed document**: `<!doctype html><html><head><meta charset=utf-8>`
  + `<meta name="color-scheme">` + inline `<style>` (theme CSS) `</head><body>`
  `<article class="markdown-body">` … `</article>`.
- **No external references** except `<img src>` (served by the webhost
  interceptor). No `<script>`, no external CSS/JS/font links.
- **GFM coverage**: headings (with `id=` slug), paragraphs, ordered/unordered/
  nested lists, task lists (disabled checkboxes), blockquotes (nested), fenced
  code with `<span class="hl-*">` highlight, inline code, tables with alignment
  (`text-align` from md4c `MD_ALIGN_*`), thematic breaks, emphasis/strong/
  strikethrough, links, images, autolinks.
- **Escaping**: all text-callback content is HTML-escaped EXCEPT `MD_TEXT_HTML`
  / `MD_BLOCK_HTML` which is emitted verbatim (raw-HTML pass-through, FR-020;
  safety is the webhost lockdown, not sanitization — FR-022).
- **Slugs**: headings use `MdSlug` (GitHub algorithm, diacritics preserved),
  de-duplicated with `-N` suffixes; recorded in `out.anchors`.
- **Highlight**: `HighlightCode` runs mapped `MDCF_KEYWORD→hl-kw`,
  `MDCF_STRING→hl-str`, `MDCF_NUMBER→hl-num`, `MDCF_COMMENT→hl-cmt`,
  `MDCF_TYPE→hl-type`, `MDCF_FUNC→hl-fn`, `MDCF_OP→hl-op`; colors come from the
  theme's `MdSyntax` via CSS classes.
- **Images**: each `<img>` `src` classified (LocalRelative/Remote/Refused);
  rewritten to `https://mdview.invalid/img/<index>`; the original + resolved
  path recorded in `out.images`. Remote → placeholder markup until consent.
- **Caps** (FR-055): nesting depth ≤ `lim.maxDepth`; output bytes ≤
  `lim.maxNodesText`; on cap hit, stop appending and close open tags (best
  effort, never crash).
- **Find**: when `findTerm` non-empty, literal (case-insensitive) matches in
  rendered text are wrapped `<mark id="mdfind-N">`; match count derivable from
  `out.html`.

## CSS (theme_css, part of htmlgen)
- `MdBuildThemeCss(const MdTheme& theme) -> std::string` emits `:root`
  variables (bg, body, heading, link, code bg/fg, table border/head, quote,
  rule, syntax colors) + a static base stylesheet: body inset (`padding`),
  reading measure `max-width: 46rem` (~900 px @96 DPI) centered, table
  `border-collapse` + cell borders, code block background + overflow-x auto,
  blockquote left accent, responsive `img { max-width: 100%; }`, `mark`
  highlight. Honors light/dark via the theme's own colors (not
  `prefers-color-scheme`, since the app controls the scheme).

## Non-goals
No script, no network, no file reads (docDir is used only to classify paths as
strings). Image bytes are loaded later by the webhost, not here.
