# Phase 1 Data Model: mdview HTML Rendering Surface

Internal state (in-process; no persistence beyond the existing registry config).

## MdTheme (unchanged, render.h)
Ten entries (5 light / 5 dark). Fields: `id` (stable ASCII), `nameStrId`,
`dark`, and ~19 `COLORREF` roles + `MdSyntax` (9 token colors). **Reused as-is**;
the CSS generator reads these to emit `:root` variables. Never indexed for
persistence (feature-007 pitfall) — persisted by `id`.

## MdHtmlResult (new, htmlgen.h) — replaces MdRenderResult
Produced by `MdRenderHtml`. Fields:
- `std::string html` — full UTF-8 HTML document (doctype + `<head>` with inline
  `<style>` + `<body>`), self-contained, no external refs except `img` (served
  by the interceptor).
- `std::vector<MdImageRef> images` — each referenced image: original `src`,
  classification (`LocalRelative` | `Remote` | `Refused`), resolved absolute
  path (long-path, for LocalRelative) or remote URL (for Remote).
- `std::vector<std::wstring> anchors` — heading slugs present (for `#fragment`
  validation on internal links).
- Counts for caps (nodes, bytes) to enforce `MdRenderLimits`.

Note: link classification is encoded directly in the emitted `<a>` (href kept
verbatim; the webhost's NavigationStarting gate classifies at click time using
DocDir), so no separate link range table is needed (RichEdit CHARRANGE model
retired).

## MdImageRef (new)
`{ std::wstring srcOriginal; enum Kind {LocalRelative, Remote, Refused} kind;
   std::wstring resolvedPathOrUrl; }`. The interceptor maps a requested
`https://mdview.invalid/img/<token>` back to an `MdImageRef` to serve or block.

## ConsentState (new, per viewer window; R9/D2)
`{ bool remoteAllowed = false; std::set<std::wstring> hosts; }`. Set true only
by the explicit per-document consent action. Never global; reset on window
close. Governs whether the interceptor fetches remote images.

## CViewerWindow (edited, viewer.h)
Removed: `HWND HRich`, RichEdit-specific fields.
Kept: `Lock`, `Name` (UTF-8 heap path, may exceed MAX_PATH), `HSchemeMenu`,
`Theme`, `FilePathW`, `DocDir`, `Encoding`, `FindText`, enum-file source ids.
Added:
- `CMdWebHost* Web` — the WebView2 host (owns env/controller/webview).
- `MdHtmlResult Html` — current generated document (for find re-generation).
- `ConsentState Consent`.
- `std::wstring DecodedText` — the decoded UTF-16 source (kept for find /
  re-generation without re-reading the file).
- find state: current match index, match count.

## CMdWebHost (new, webview.h) — the render surface
Owns `wil`/raw `ICoreWebView2Environment`, `ICoreWebView2Controller`,
`ICoreWebView2`, and event tokens. State:
- `ready` flag; `pendingHtml`/`pendingBaseDir` (render-deferred until ready).
- callbacks to owner: `OnReady`, `OnNavigateLink(uri)`, `OnProcessFailed`.
- resource table: current `MdHtmlResult*` for the interceptor to serve
  document + images.

## Registry config (unchanged, mdview.h globals)
`g_scheme`, `g_followSys`, `g_schemeLight`, `g_schemeDark`, `g_zoom`,
`g_savePos`, `g_wndPlacement`. No new persisted keys (consent is session-only).

## Lifecycle
1. F3 → `ViewFile` → thread + `CViewerWindow` (unchanged).
2. `WM_CREATE` → create `CMdWebHost`, start async env→controller.
3. `OpenFile` → read file (long-path), `MdDetectDecode` → `DecodedText`; set
   pending-render.
4. Controller ready → lockdown + events; `RenderDocument`: UTF-16→UTF-8 →
   `MdRenderHtml(theme)` → `Html`; navigate to `https://mdview.invalid/doc.html`.
5. Interceptor serves `Html.html` and images per `ConsentState`.
6. Scheme/zoom/find → regenerate/re-navigate or `put_ZoomFactor`.
7. Close → release per-document resources; env/controller torn down; leak-free
   over repeated cycles (FR-082).
