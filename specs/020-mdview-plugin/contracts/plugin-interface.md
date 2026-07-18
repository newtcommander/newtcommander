# Contracts (Phase 1): mdview

## A. Salamander SDK surface implemented (external contract)

Exports (`mdview.def`):
- `SalamanderPluginEntry` — returns the `CPluginInterface*`.
- `SalamanderPluginGetReqVer` — returns `LAST_VERSION_OF_SALAMANDER` (104).

`CPluginInterfaceAbstract` (implemented in `mdview.cpp`):
- `About(HWND parent)` — house About box.
- `Release(HWND parent, BOOL force)` — close all viewer windows/threads.
- `LoadConfiguration/SaveConfiguration` — config key I/O (see config contract).
- `Configuration(HWND parent)` — optional config page (house DIALOGEX).
- `Connect(HWND parent, CSalamanderConnectAbstract*)` —
  `AddViewer("*.md;*.markdown", FALSE)`; `SetBitmapWithIcons`/`SetPluginIcon`.
- `GetInterfaceForViewer()` — returns the viewer interface.
- `Event`, `ClearHistory`, `ReleasePluginDataInterface` — trivial/no-op.

`CPluginInterfaceForViewerAbstract` (implemented in `viewer.cpp`):
- `CanViewFile(const char* name)` — cheap checks only: openable, size ≤ gate,
  not obviously binary. FALSE ⇒ host cascades to the next viewer (FR-080). Never
  rejects on Markdown content.
- `ViewFile(name, left, top, width, height, showCmd, alwaysOnTop, returnLock,
  HANDLE* lock, BOOL* lockOwner, viewerData, enumFilesSourceUID,
  enumFilesCurrentIndex)` — spawns the viewer thread/window, honors `returnLock`
  (signal when the window closes — temp file lifetime, FR-102), returns TRUE.

Entry obligations: version check, `LoadLanguageModule` (english.slg),
`GetSalamanderGeneral`/`GetSalamanderGUI`, `SetBasicPluginData(name,
FUNCTION_VIEWER | FUNCTION_CONFIGURATION | FUNCTION_LOADSAVECONFIGURATION, ...)`.

SDK services used: `SalGeneral->AddViewer`, registry API, `SplU8ToWExtAlloc` /
`SplU8ToWAlloc` (splunicode.h), `GetNextFileNameForViewer` /
`GetPreviousFileNameForViewer`, `ViewFileInPluginViewer(NULL, ...)` for the
"Open in text viewer" fallback, `ViewerWindowQueue.BroadcastMessage`.

## B. Internal module contracts

### mdparser
```
Encoding DetectAndDecode(const BYTE* bytes, size_t len, std::wstring& outText);
bool ParseMarkdown(const std::wstring& text, const ParseLimits& lim, Document& out);
```
- Deterministic, bounded (`ParseLimits`: maxNodes, maxDepth, watchdog). No I/O,
  no network. Returns false only on limit-exceeded (caller → text-viewer
  fallback); malformed input still returns true (best-effort, FR-082).

### highlight
```
void Highlight(const std::wstring& code, const char* langNormalized,
               std::vector<TokenRun>& outRuns); // empty runs = plain block
```

### rtfrender
```
struct RenderResult { std::string rtf; AnchorTable anchors; LinkTable links; };
void RenderRtf(const Document& doc, const Theme& theme, int dpi, RenderResult& out);
```
- Pure function of (doc, theme). Emits RTF only (text + formatting; no active
  content possible). Populates anchor/link tables.

### themes
```
const Theme* ThemeById(const char* id);      // nullptr if unknown
const Theme* DefaultTheme(bool dark);        // paper / graphite
size_t ThemeCount(); const Theme* ThemeAt(size_t);
#ifdef _DEBUG bool AssertContrastGates();    // FR-061 self-check #endif
```

### config
```
struct Config { char scheme[32]; int followTheme; char schemeLight[32];
                char schemeDark[32]; int zoom; WINDOWPLACEMENT wp; bool wpValid; };
void LoadConfig(HKEY key, CSalamanderRegistryAbstract*, Config& out); // defaults on any miss
void SaveConfig(HKEY key, CSalamanderRegistryAbstract*, const Config&);
```

### viewer link gate
```
void ActivateLink(ViewerSession&, const wchar_t* url); // FR-030/031/034, D-R4
```

## C. Invariants the contracts must uphold (from spec §Security)

- No module performs network I/O on document open (FR-041). Remote image /
  external link fetch happens only inside an explicit user-gesture handler.
- `rtfrender` output can never contain executable/active content (RTF text +
  formatting only) — FR-040/042.
- All file access uses long-path-safe wide paths; no fixed `MAX_PATH` buffers
  (FR-103).
- `ActivateLink` is the only place that may launch anything, and only after the
  scheme allowlist passes (FR-031).
