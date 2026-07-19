# Contract: CMdWebHost (WebView2 render surface)

Encapsulates all WebView2/COM usage. Owned by `CViewerWindow`. STA / viewer-
thread affine. Everything content-facing is locked down (invariants FR-050..057).

## Interface (webview.h)
```cpp
class CMdWebHost {
public:
    struct Callbacks {
        std::function<void()> OnReady;              // controller ready → render
        std::function<void(const std::wstring&)> OnActivateLink; // allowlisted/nav gate
        std::function<void()> OnInitFailed;         // env/controller/runtime failure
        std::function<void()> OnProcessFailed;      // renderer crashed
    };
    bool Create(HWND parent, const std::wstring& userDataFolder, Callbacks cb); // async
    bool IsReady() const;
    void Resize(const RECT& client);
    void SetZoomPercent(int pct);                   // put_ZoomFactor
    void Navigate(const MdHtmlResult* doc, const std::wstring& docDir,
                  const ConsentState* consent);     // serves via interceptor
    void ScrollToAnchor(const std::wstring& slug);  // navigate #slug
    std::wstring GetSelectedText();                 // plain text (invariant 8)
    void RouteAccelerator(...);                     // from AcceleratorKeyPressed
    void Destroy();                                 // release env/controller
};
```

## Behavior guarantees
- **Async create**: `Create` starts `CreateCoreWebView2EnvironmentWithOptions`
  → controller; on completion applies the full lockdown then calls `OnReady`.
  Any failure (runtime missing/disabled/below min version, HRESULT error) calls
  `OnInitFailed` (owner → error UI + text-viewer fallback, FR-038a).
- **Lockdown at ready** (config, invariants): scripts off; default context
  menu/devtools/status bar/built-in error page off; browser accelerator keys
  off; zoom control off; autofill/password off; SmartScreen off;
  `--disable-background-networking`.
- **Offline serving**: `AddWebResourceRequestedFilter(L"*", ALL)`;
  `WebResourceRequested` handler answers ONLY `https://mdview.invalid/`:
  `doc.html` → the document bytes; `img/<n>` → local image bytes if
  `MdImageRef.kind==LocalRelative` and under DocDir, else 403/placeholder;
  remote images fetched (WinHTTP) only if `consent->remoteAllowed`. Everything
  else → 403 (network default-deny, invariant 3).
- **Navigation gate**: `NavigationStarting` + `NewWindowRequested` cancel every
  navigation except the initial `doc.html` load and same-document `#fragment`.
  Cancelled navigations whose URI maps back to a link are handed to
  `OnActivateLink` (owner classifies: internal anchor → ScrollToAnchor; local
  `.md` → new viewer window; other local → path-only; http/https/mailto →
  ShellExecuteW; else blocked). No `ftp` (FR-034).
- **Process isolation**: `ProcessFailed` → `OnProcessFailed` (owner → error +
  fallback). Renderer runs out-of-process (invariant 7 hardening).
- **Clipboard**: `GetSelectedText` returns plain text only; copy is owner-
  initiated (invariant 8).
- **Teardown**: `Destroy` closes the controller and releases all COM refs; safe
  to call repeatedly; leak-free across open/close cycles (FR-082).

## Dependencies
Headers: `WebView2.h`, `WebView2EnvironmentOptions.h`, `wrl.h`
(`Microsoft::WRL::Callback`), `shlwapi.h` (`SHCreateMemStream`).
Link: `WebView2LoaderStatic.lib`, `shlwapi.lib`, `ole32.lib`, `version.lib`.
Runtime: WebView2 Evergreen (Win11 OS component).
