# Phase 0 Research: mdview HTML Rendering Surface

Decisions resolving the Technical Context unknowns. Full trade-off analysis
lives in `../020-mdview-plugin/analysis/html-renderer.md`; this file records
the *chosen* resolutions.

## R1 — Markdown parser: md4c (vendored)

**Decision**: Vendor `md4c` (single `md4c.c` + `md4c.h`, MIT, CommonMark
0.31, linear-time) into `src/common/dep/md4c/`. Parse with a custom
`MD_PARSER` callback renderer (`htmlgen.cpp`) rather than stock `md4c-html.c`,
because we need heading `id=` slugs, `hl-*` highlight spans, image consent
placeholders, link classification, resource caps, and the raw-HTML pass-through
switch. Flags: `MD_DIALECT_GITHUB` (tables, strikethrough, task lists,
permissive autolinks) + `MD_FLAG_PERMISSIVEAUTOLINKS`. Raw HTML pass-through:
do **not** set `MD_FLAG_NOHTML` (embedded HTML flows to the `text` callback as
`MD_TEXT_HTML` and is emitted verbatim into the document — see R6).

**Rationale**: Already the documented upgrade for mdview (`render.h:5-7`);
MIT is GPLv2-compatible; single-file keeps the build self-contained.
**Alternatives rejected**: cmark-gfm (heavier, multi-file, was the removed
ieviewer dep); keeping the hand parser (the fidelity ceiling this feature
fixes).

## R2 — Rendering engine: WebView2 (Evergreen OS runtime)

**Decision**: Render the generated HTML in an embedded WebView2 control.
Vendor the **SDK** (headers `WebView2.h` + `WebView2EnvironmentOptions.h`, and
`WebView2LoaderStatic.lib` for x86 and x64) into `src/common/dep/webview2/`;
link the static loader so no `WebView2Loader.dll` ships. The **runtime** is the
Evergreen WebView2 Runtime, preinstalled as a Windows 11 OS component — never
distributed by us.

**Build-reproducibility argument (Constitution I)**: everything compiled or
linked is committed (headers + prebuilt loader lib, version-pinned, BSD-3
license committed alongside). The build performs no download. The runtime is a
system component (Principle IV, Win11), analogous to `user32.dll`; GPLv2 §3
system-library clause covers it. SDK license is BSD-3-Clause → GPLv2-compatible.
**Alternatives rejected**: litehtml (in-tree, but must hand-build
selection+find, weaker CSS — kept only as recorded fallback); RTF/RichEdit
evolution (plateaus, no raw HTML); MSHTML (dead).

## R3 — Content serving: WebResourceRequested on a private virtual host

**Decision**: Serve the document as `https://mdview.invalid/doc.html` via a
`WebResourceRequested` handler backed by an in-memory `IStream`
(`SHCreateMemStream`), with `AddWebResourceRequestedFilter(L"*", ALL)` and a
**default-deny** handler (only `https://mdview.invalid/*` is answered).

**Rationale**: `NavigateToString` has a ~2 MB content cap that a 20 MB source
can exceed after HTML expansion. `SetVirtualHostNameToFolderMapping` would
expose the whole document directory to any crafted URL. The interceptor is both
the content server *and* the network default-deny net (invariant 3) and the
per-request local-resource gate (invariant 5). `.invalid` (RFC 2606) never
resolves in real DNS.

## R4 — Security lockdown mapped to invariants (config + test)

**Decision**: Apply the full per-invariant lockdown from
`analysis/html-renderer.md` §4.1.5 at controller-ready time:
`put_IsScriptEnabled(FALSE)`; context menus / devtools / status bar / built-in
error page off; `AreBrowserAcceleratorKeysEnabled(FALSE)`;
`IsZoomControlEnabled(FALSE)` (we own zoom); autofill/password off; reputation
checking (SmartScreen) off; `put_AdditionalBrowserArguments(L"--disable-background-networking")`;
`add_NavigationStarting`/`add_NewWindowRequested` → Cancel-all (link activation
routed through our allowlist gate); `add_WebResourceRequested` default-deny;
`add_ProcessFailed` → error + text-viewer fallback. Ship an adversarial
self-test corpus (fixtures) exercising script/beacon/scheme/form/iframe/traversal
and assert no effect; re-run on runtime version bump.

**Residual risk recorded**: the runtime's own OS-level updater/telemetry is
outside the app frame; invariant 3 governs *content-triggered* I/O only.

## R5 — Async init inside the existing thread model

**Decision**: On the viewer thread, `CoInitializeEx(APARTMENTTHREADED)`. In
`WM_CREATE`, kick off `CreateCoreWebView2EnvironmentWithOptions(userDataFolder,
options, envCompletedHandler)`; the env handler creates the controller; the
controller handler applies lockdown + event registration + first navigation.
`OpenFile` records state and sets a "pending render" flag consumed once the
controller is ready. WebView2 objects are STA/thread-affine → each viewer
thread creates its own environment+controller against the shared user-data
folder (allowed with identical options). WRL `Microsoft::WRL::Callback<...>`
provides the handlers; the existing `GetMessage` loop pumps the async
callbacks (the loop already exists in `CViewerThread::Body`).

**Note**: the current `OpenFile` runs before the message loop; it is refactored
to be render-deferred so callbacks can complete.

## R6 — Raw HTML rendered natively, no sanitizer (clarify Q2/Q1)

**Decision**: Embedded raw HTML (`MD_TEXT_HTML`, block `MD_BLOCK_HTML`) is
emitted verbatim into the document. Safety is entirely from R4 lockdown: with
scripts disabled and network default-denied, `<script>`, event handlers,
forms, iframes, and remote refs are inert. No HTML sanitizer/whitelist (FR-022:
a sanitizer is a larger, historically weaker trusted surface — mXSS,
parser-differentials).

## R7 — Search without scripts (mark-injection fallback)

**Decision**: Implement find in-viewer by re-generating the HTML with
`<mark id="mdfind-K">` wrapping matches located in our own decoded source text,
then navigating to `#mdfind-K` for next/prev. This is script-free and works on
any runtime version (no dependency on the newer `ICoreWebView2Find` API).
Ctrl+F prompt reuses the existing `IDD_FIND` dialog; F3/Shift+F3 cycle marks.

**Rationale**: avoids a hard minimum-runtime dependency on the Find API; we own
the generator so match highlighting is deterministic.

## R8 — Zoom, schemes, DPI

**Decision**: Zoom via `ICoreWebView2Controller::put_ZoomFactor(g_zoom/100.0)`,
range 50–300 %, persisted in `g_zoom` (unchanged). Ctrl+±/0 and Ctrl+wheel
routed via `add_AcceleratorKeyPressed` (focus is inside the WebView2 HWND).
Schemes: the ten `MdThemes[]` entries generate a `:root { --md-*: … }` CSS
variables block; F9 cycles by re-navigating with the new stylesheet and
restoring scroll via a `#fragment` anchor (script-free). DPI handled by the
engine.

## R9 — Images + remote consent (D2, US3)

**Decision**: `<img src>` with a relative path under DocDir is served by the
interceptor (bytes read from disk, engine decodes — WIC not needed on this
path). Absolute/UNC/`file:`/traversal → refused (placeholder). Remote
`http(s)` images → interceptor returns a blocked response + CSS placeholder;
a per-document consent action (toolbar/menu affordance naming the host) sets a
flag; after consent the interceptor itself fetches over WinHTTP (no cookies,
redirect-capped) and serves the bytes — the engine never touches the network.
Consent is document/session-scoped; no global always-allow (FR-012). *Phase 3.*

## R10 — Encoding, long paths, fallback (unchanged infra)

**Decision**: Keep `MdDetectDecode` (BOM UTF-8/16LE/BE, strict UTF-8
validation, ANSI fallback), long-path reads via `SplU8ToWExtAlloc`, the 20 MB
`SIZE_GATE`, and `OpenAsText` hand-off. Decoded UTF-16 → UTF-8 for md4c (md4c
is UTF-8-native). On engine init failure or `ProcessFailed`, show an error
state and hand to the internal text viewer (FR-038a — single backend, RTF path
removed).

## R11 — Removed v1 code

**Decision**: Remove RTF emission and the RichEdit host: `MdRenderMarkdown` +
all `emit*`/`BuildHeader` RTF functions in `render.cpp`; `EM_STREAMIN`/
`StreamInCb`/CHARRANGE link machinery/`DoFind` internals in `viewer.cpp`.
Keep in `render.cpp`: `MdDetectDecode`, `MdThemes[]` + lookups, `MdSlug`.
`highlight.cpp` stays (its `HlRun`/`MDCF_*` output is mapped to CSS classes by
htmlgen).
