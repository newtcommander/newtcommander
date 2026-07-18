# Agent 2 — Plugin Integration Analysis (raw report, specify phase)

Independent analysis; evidence cites the codebase as of branch base.

## Findings

1. **Viewer contract is two methods** — `CPluginInterfaceForViewerAbstract`
   (src/plugins/shared/spl_view.h:29–66): `ViewFile(name, placement, showCmd,
   alwaysOnTop, returnLock, HANDLE* lock, BOOL* lockOwner, viewerData,
   enumFilesSourceUID, enumFilesCurrentIndex)` and `CanViewFile(name)`.
   `CanViewFile` must not show UI; FALSE ⇒ Salamander tries the next viewer in
   the priority list (spl_view.h:59–65).
2. **Plugin skeleton**: exports `SalamanderPluginEntry` +
   `SalamanderPluginGetReqVer` (.def); entry calls `LoadLanguageModule`
   (mandatory; NULL = plugin refuses to load, demoview.cpp:151–153),
   `GetSalamanderGeneral()`, `GetSalamanderGUI()`, `SetBasicPluginData(name,
   FUNCTION_VIEWER | FUNCTION_CONFIGURATION | FUNCTION_LOADSAVECONFIGURATION,
   ...)` (demoview.cpp:122–179). Interface version 104 =
   `LAST_VERSION_OF_SALAMANDER` (spl_vers.h:200) — the UTF-8 + long-path ABI
   (architecture/06:242–252).
3. **Extension registration in `Connect`, first install only**:
   `AddViewer("*.dmv", FALSE)` (demoview.cpp:253). Core (plugins1.cpp:636–773):
   no-op unless first-install capability transition (plugins1.cpp:2192, 2316) or
   `force=TRUE` (re-adds missing masks — pictview.cpp:1052 upgrade pattern).
   New mask inserted at **index 0** of `ViewerMasks` = top priority
   (plugins1.cpp:758–762). `'|'` forbidden in masks; 300-char buffer.
4. **Feature-016 lesson applies to viewer masks, softer failure**:
   `CPlugins::CheckData` deletes viewer-mask rows with invalid plugin index
   (plugins2.cpp:2260–2281); the 016 self-heal covers ONLY archive associations
   (plugins2.cpp:2156–2238). Lost `*.md` row is never re-created — but F3 then
   falls through to internal text viewer `*.*` (graceful). Dropping mdview.spl
   into `plugins\` on an existing config IS a first install (plugins.ver /
   SearchForSPLs, architecture/06:266) → mask gets registered.
5. **F3 selection algorithm** — `ViewFileInt` (fileswn5.cpp:958–1227): walks
   `ViewerMasks` in order; first matching row wins; plugin rows call
   `CanViewFile` first, FALSE ⇒ continue to next matching row
   (fileswn5.cpp:1039–1051). Defaults: `*.rpm`→TAR, `*.*`→internal viewer last
   (mainwnd1.cpp:352–366). No other plugin claims .md/.markdown/.mdown/.mkd
   (verified across all AddViewer calls).
6. **`ViewFile` returning FALSE has NO fallback** (fileswn5.cpp:1189–1215):
   no other viewer tried, no core error dialog. Fallback must be engineered in
   `CanViewFile` (cheap pre-checks) or inside the plugin window.
7. **Programmatic internal-viewer fallback exists**:
   `ViewFileInPluginViewer(pluginSPL=NULL, ...)` opens the internal text/hex
   viewer (spl_gen.h:1884–1908; main thread only). **Alt+F3 always shows raw
   source**: `AltViewerMasks` default `*.*` internal viewer (mainwnd1.cpp:
   368–374); plugins cannot register alt-viewer masks. "View With" menu
   enumerates all viewers (fileswn5.cpp:1596–1705).
8. **Threading/window model (demoview pattern)**: `ViewFile` on main thread
   spawns a dedicated thread per viewer window; lock/lockOwner via event
   handshake; own GetMessage loop; ordinary top-level WS_OVERLAPPEDWINDOW via
   shared WinLib (winliblt.cpp); SalamanderGUI menu bar/toolbar/rebar
   (demoview viewer.cpp:115–292).
9. **Archive/FS files arrive as temp copies under a lock event** (fileswn5.cpp:
   850–906, 947–950; spl_view.h:40–53): signal lock when viewing ends (temp
   deleted); without lock, `name` valid only during the call. Relative images
   resolve inside the cache dir ⇒ siblings generally absent ⇒ legitimately
   "missing" placeholders.
10. **Unicode/long paths**: interface 104 passes UTF-8 paths up to ~32k chars.
    SDK helpers src/plugins/shared/splunicode.h: `SplU8ToWAlloc`,
    `SplWToU8Alloc`, `SplU8ToWExtAlloc` (adds `\\?\`). Core's content-encoding
    detection (`ViewerDetectEncoding`, src/viewer.h:35–46 — BOM, strict UTF-8
    validation, else legacy) is NOT exported — mdview must mirror it (small).
    SDK exports legacy code-page services: `EnumConversionTables`,
    `GetConversionTable`, `RecognizeFileType`, `IsANSIText`
    (spl_gen.h:2183–2221).
11. **No Markdown parser or HTML renderer vendored** (src/common/dep: zlib,
    bzip2, sqlite, fmt, wil, nanosvg, pnglite, libssh2, crypt). **nanosvg**
    (zlib license) already rasterizes SVG for the core (src/svg.cpp). **WIC via
    COM is accepted precedent** (pictview 006, wicengine.cpp:67) —
    PNG/JPEG/GIF/BMP/WebP via system codecs. No WebView2 anywhere; MSHTML
    flagged dead (architecture/10:52).
12. **Repo history had a Markdown pipeline**: removed ieviewer bundled
    **cmark-gfm** (autolink, strikethrough, table, tagfilter, tasklist) +
    markdown.cpp glue + CSS themes (git `ce42e70~1:src/plugins/ieviewer/`).
    BSD-2/MIT — GPLv2-compatible. Attribution: doc/third_party.txt.
13. **Build checklist**: `mdview.vcxproj` + `lang_mdview.vcxproj` importing
    plugin_base.props chain (sftp is the newest reference); `.def` exports;
    add to salamand.sln; **`mdview=on` line in plugins.cfg (mandatory — build
    stops on unlisted plugin dirs)**; optional baseaddr_x64.txt entry (sftp
    ships without one); `build.cmd full` regenerates plugins.ver.
14. **Config persistence**: FUNCTION_LOADSAVECONFIGURATION +
    Load/SaveConfiguration on plugin-private registry key with a Version DWORD
    (demoview.cpp:213–238); config-change broadcast to open windows via
    `ViewerWindowQueue.BroadcastMessage(WM_USER_VIEWERCFGCHNG)` (demoview.cpp:93).

## Integration contract summary

- Exports: `SalamanderPluginEntry`, `SalamanderPluginGetReqVer` (=104).
- `CPluginInterfaceAbstract`: About, Release (close windows/threads),
  LoadConfiguration, SaveConfiguration, Configuration, Connect (AddViewer +
  icons), ClearHistory, Event, GetInterfaceForViewer.
- `CPluginInterfaceForViewerAbstract`: ViewFile (honor returnLock),
  CanViewFile (no UI).
- Entry obligations: version check, LoadLanguageModule (english.slg),
  SetBasicPluginData.
- Optional services: GetNext/PreviousFileNameForViewer + enumFilesSourceUID;
  ViewFileInPluginViewer(NULL,...); RecognizeFileType/GetConversionTable;
  SalamanderGUI chrome; splunicode helpers.

## Constraints on rendering approach (constitution gates)

Constitution IV (pure WinAPI, no cross-platform frameworks, GPLv2-compatible
deps), VI (no process-wide visual side effects, house-style dialogs), zero
NuGet, all deps vendored.

- **A. Vendored MD parser (md4c MIT / cmark-gfm BSD-2) + custom native
  renderer (GDI or DirectWrite/D2D via COM)** — fully compliant (WIC/COM
  precedent legitimizes DirectWrite; nanosvg+WIC cover images); security §8
  trivially satisfied by construction; highest implementation effort
  (layout/selection/hit-testing/tables).
- **B. WebView2** — fidelity ~free, but: spawns msedgewebview2.exe children;
  SDK normally via NuGet (repo: zero NuGet); must PROVE JS/active content off
  across engine auto-updates; heavyweight vs app philosophy. Not letter-banned;
  needs explicit owner ruling.
- **C. MSHTML** — dead, excluded.
- **D. RichEdit** — VI-compliant and gives selection for free, but weak
  tables/images/theming; likely dead end for §5/§10.

## Recommendations for v1

1. FUNCTION_VIEWER | FUNCTION_CONFIGURATION | FUNCTION_LOADSAVECONFIGURATION;
   demoview thread-per-window + lock model verbatim.
2. Register `*.md;*.markdown;*.mdown;*.mkd`, `force=FALSE`, one AddViewer call
   (cost = one mask string; no conflicts; FALSE respects user deletions).
3. `CanViewFile` = fallback mechanism: check only openability (+ size cap);
   never reject on content (any byte stream is "valid Markdown"). Post-open
   failures: in-window error state + "Open in text viewer"
   (ViewFileInPluginViewer(NULL)). Document Alt+F3 as always-source.
4. Prefer option A; evaluate resurrecting in-history cmark-gfm vs adopting
   md4c at plan phase. Defer WebView2 unless owner accepts policy costs.
5. Encoding: mirror core ViewerDetectEncoding contract locally; use
   RecognizeFileType for CP1250/ISO-8859-2 guess; undecidable → fallback per
   (3); never crash.
6. Theme persistence via plugin registry key + Version DWORD + broadcast
   re-theme (WM_USER_VIEWERCFGCHNG pattern).
7. Constitution VI: no ICC_STANDARD_CLASSES, no plugin manifest, DIALOGEX +
   DS_SHELLFONT config dialog.

## Risks

- No fallback after ViewFile=TRUE — plugin window owns all §13 error states.
- Mask priority: install inserts at index 0, silently outranking a user's
  pre-existing `*.md` row until reordered (standard behavior; release-note it).
- Viewer masks have no self-heal (016 covered archives only) — acceptable
  (degrades to internal viewer).
- Archive temp copies: relative images won't resolve; lock lifetime must equal
  window lifetime.
- No API size guard — plugin needs own cap + degradation (§14).
- Interface 104: every file open via `\\?\`-style conversion; no fixed
  MAX_PATH buffers (feature 011–014 defect class).
- Missing english.slg / bad .def = silent load refusal; missing plugins.cfg
  line = build error.
- WebView2 (if chosen): NuGet-free vendoring, loader licensing, JS-off
  verification, child-process behavior — all unresolved.

## Open questions (ranked)

1. Rendering engine ruling: native renderer over vendored parser (recommended)
   vs WebView2; if native — resurrect cmark-gfm or adopt md4c?
2. Extension set: all four (recommended here) or fewer?
3. Failure/fallback UX: automatic hand-off (CanViewFile FALSE /
   ViewFileInPluginViewer) vs explicit in-window error page?
4. Document size threshold for degraded mode / text-viewer offer; incremental
   rendering in v1?
5. Priority collision: silent takeover of `.md` from user-configured rows
   (standard) vs install-time notice?
