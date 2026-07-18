# Data Model (Phase 1): mdview

Internal, in-memory only (no persistence except the config key). Types below are
the design; exact C++ names may be refined in code but the boundaries hold.

## Document model (output of `mdparser`, input to `rtfrender`)

A parsed document is an ordered list of **blocks**; inline-bearing blocks hold an
ordered list of **inline spans**.

### Block kinds

| Kind | Fields |
|------|--------|
| `Heading` | level 1–6; inline content; computed `anchorSlug` (GitHub algorithm, Czech diacritics preserved, dedup -1/-2) |
| `Paragraph` | inline content |
| `CodeBlock` | language info-string (raw + normalized via alias table); verbatim text; `highlighted` token runs (from `highlight`) |
| `BlockQuote` | nesting depth; child blocks |
| `List` | ordered flag; start number; tight/loose; items |
| `ListItem` | optional task-checkbox state (none / unchecked / checked); child blocks |
| `Table` | column alignments (left/center/right/none); header cells; body rows (each cell = inline content) |
| `ThematicBreak` | — |
| `HtmlLiteral` | raw HTML text rendered inert/literal (FR-015) |
| `FrontMatter` | raw YAML text (rendered as a distinct metadata code block, FR-014) |
| `Placeholder` | reason text (degradation for math/mermaid fences etc.) |

### Inline span kinds

| Kind | Fields |
|------|--------|
| `Text` | UTF-16 text (entities/escapes already resolved) |
| `Emph` / `Strong` / `Strike` | child spans |
| `Code` | verbatim text |
| `Link` | display children; resolved target URL; is-internal-anchor flag |
| `Image` | alt text; resolved/canonicalized source; kind (local-relative / missing / blocked-absolute / remote) |
| `LineBreak` | hard flag |
| `HtmlInline` | raw inert text |

### Render artifacts (produced alongside RTF)

- **Anchor table**: `slug → character offset` (for `#anchor` navigation).
- **Link table**: `linkId → target URL` (RTF `CFE_LINK` carries the id; click
  looks up the URL and runs the security gate).
- **Plain-text length / mapping**: enough to support Ctrl+F semantics via
  `EM_FINDTEXTEXW` (RichEdit owns the text; we only need offsets for anchors).

## Theme model (`themes`)

```
struct Rgb { BYTE r,g,b; };            // COLORREF-compatible
struct SyntaxPalette {                 // 9 token classes
  Rgb keyword, string, number, comment, type, function, op, diffAdd, diffDel;
};
struct Theme {
  const char* id;         // stable ASCII: "paper","softgray",...,"hicontrast"
  UINT nameStringId;      // localized display name (.slg)
  bool isDark;
  Rgb docBg, body, heading, link, linkActive;
  Rgb quoteText, quoteAccent, inlineCodeFg, inlineCodeBg;
  Rgb codeBg, codeText, tableBorder, tableHeaderBg, rule;
  Rgb selBg, selFg, focus, imgPlaceholderText, imgPlaceholderBorder;
  SyntaxPalette syntax;
};
extern const Theme g_themes[10];       // 5 light then 5 dark
```

Contrast values are pre-corrected so every role passes FR-061 (debug self-check
asserts it). Two designated defaults: light = `paper`, dark = `graphite`.

## Configuration (`config`, persisted to the plugin registry key)

| Value | Type | Default | Notes |
|-------|------|---------|-------|
| `Version` | REG_DWORD | current | schema guard (FR-100) |
| `ColorScheme` | REG_SZ | `paper` | stable ASCII id; unknown → default |
| `FollowSystemTheme` | REG_DWORD | 0 | FR-064 |
| `SchemeLight` | REG_SZ | `paper` | per-polarity slot for auto mode |
| `SchemeDark` | REG_SZ | `graphite` | per-polarity slot for auto mode |
| `ZoomPercent` | REG_DWORD | 100 | FR-073, clamped 50–300 |
| `WindowPlacement` | REG_BINARY | (unset) | `WINDOWPLACEMENT` (FR-072) |

Any missing/unknown/wrong-type value falls back to its default; the load never
crashes and self-heals on next save (FR-063/100).

## Viewer session (runtime, per open window)

```
struct ViewerSession {
  wchar_t* filePathW;        // long-path capable (\\?\), no fixed MAX_PATH
  Encoding detectedEncoding; // utf8/utf8bom/utf16le/utf16be/ansi-fallback
  Document doc;              // parsed model (may be discarded after RTF built)
  HWND hRichEdit;            // MSFTEDIT child
  const Theme* theme;        // active scheme
  int zoomPercent;
  AnchorTable anchors;
  LinkTable links;
  RemoteImageConsent consent; // per-document (default: blocked) — FR-025
  HANDLE lock; BOOL lockOwner; // temp-file handshake for archive/SFTP sources
  EnumFilesContext enumCtx;   // next/prev-file navigation
};
```

## Key relationships

- `mdparser`: bytes → `Encoding` + `Document` (bounded; caps in FR-092).
- `highlight`: `CodeBlock.text` + language → token runs.
- `rtfrender`: `Document` + `Theme` → RTF string + AnchorTable + LinkTable.
- `viewer`: hosts RichEdit, streams RTF (`EM_STREAMIN`), applies bg color + zoom,
  wires keys/menu/search/links, persists via `config`, honors the lock handshake.
