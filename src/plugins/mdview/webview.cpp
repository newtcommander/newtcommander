// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// webview.cpp - locked-down WebView2 rendering surface. Serves a single
// in-memory document from a private virtual host (https://mdview.invalid/),
// default-denies every other request (no content-triggered network), cancels
// all navigation except the document itself, disables scripts and active
// content, and routes viewer accelerators back to the owner window.

#include "precomp.h"
#include "render.h"

// WRL's implements.h (pulled in by <wrl.h> and WebView2EnvironmentOptions.h)
// is incompatible with the debug leak-tracking "new" macro from precomp.h.
// Suspend it across these headers, then restore it.
#pragma push_macro("new")
#undef new
#include <wrl.h>
#include <shlwapi.h>
#include <winhttp.h>
#include "WebView2.h"
#include "WebView2EnvironmentOptions.h"
#pragma pop_macro("new")

#include "webview.h"

using namespace Microsoft::WRL;

static const wchar_t* kBase = L"https://mdview.invalid/doc.html";
static const wchar_t* kImgPrefix = L"https://mdview.invalid/img/";

// ==========================================================================
// helpers
// ==========================================================================

static std::wstring MakeExtPath(const std::wstring& p)
{
    if (p.size() >= 4 && p.compare(0, 4, L"\\\\?\\") == 0) return p;
    if (p.size() >= 2 && p[0] == L'\\' && p[1] == L'\\') return L"\\\\?\\UNC\\" + p.substr(2);
    return L"\\\\?\\" + p;
}

static bool ReadFileBytes(const std::wstring& path, std::vector<BYTE>& out)
{
    HANDLE h = CreateFileW(MakeExtPath(path).c_str(), GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER sz;
    bool ok = false;
    if (GetFileSizeEx(h, &sz) && sz.QuadPart >= 0 && sz.QuadPart <= (64LL * 1024 * 1024))
    {
        DWORD n = (DWORD)sz.QuadPart;
        out.resize(n);
        DWORD rd = 0;
        if (ReadFile(h, n ? &out[0] : NULL, n, &rd, NULL)) { out.resize(rd); ok = true; }
    }
    CloseHandle(h);
    return ok;
}

static const wchar_t* SniffContentType(const BYTE* d, size_t n)
{
    if (n >= 8 && d[0] == 0x89 && d[1] == 'P' && d[2] == 'N' && d[3] == 'G')
        return L"Content-Type: image/png";
    if (n >= 3 && d[0] == 0xFF && d[1] == 0xD8 && d[2] == 0xFF)
        return L"Content-Type: image/jpeg";
    if (n >= 6 && (memcmp(d, "GIF87a", 6) == 0 || memcmp(d, "GIF89a", 6) == 0))
        return L"Content-Type: image/gif";
    if (n >= 2 && d[0] == 'B' && d[1] == 'M')
        return L"Content-Type: image/bmp";
    if (n >= 12 && memcmp(d, "RIFF", 4) == 0 && memcmp(d + 8, "WEBP", 4) == 0)
        return L"Content-Type: image/webp";
    if (n >= 4 && (memcmp(d, "<svg", 4) == 0 || memcmp(d, "<?xm", 4) == 0))
        return L"Content-Type: image/svg+xml";
    return L"Content-Type: application/octet-stream";
}

// Minimal WinHTTP GET (no cookies, capped size). Only reached for a remote
// image the user explicitly consented to.
static bool FetchRemote(const std::wstring& url, std::vector<BYTE>& out)
{
    URL_COMPONENTS uc;
    ZeroMemory(&uc, sizeof(uc));
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256] = {0}, path[2048] = {0};
    uc.lpszHostName = host; uc.dwHostNameLength = _countof(host);
    uc.lpszUrlPath = path; uc.dwUrlPathLength = _countof(path);
    if (!WinHttpCrackUrl(url.c_str(), (DWORD)url.size(), 0, &uc)) return false;
    bool https = (uc.nScheme == INTERNET_SCHEME_HTTPS);

    HINTERNET hs = WinHttpOpen(L"OpenSalamander-mdview", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                               WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hs) return false;
    bool ok = false;
    HINTERNET hc = WinHttpConnect(hs, host, uc.nPort, 0);
    if (hc)
    {
        HINTERNET hr = WinHttpOpenRequest(hc, L"GET", path, NULL, WINHTTP_NO_REFERER,
                                          WINHTTP_DEFAULT_ACCEPT_TYPES,
                                          https ? WINHTTP_FLAG_SECURE : 0);
        if (hr)
        {
            if (WinHttpSendRequest(hr, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                   WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                WinHttpReceiveResponse(hr, NULL))
            {
                out.clear();
                DWORD avail = 0;
                ok = true;
                do
                {
                    avail = 0;
                    if (!WinHttpQueryDataAvailable(hr, &avail)) { ok = false; break; }
                    if (avail == 0) break;
                    size_t base = out.size();
                    out.resize(base + avail);
                    DWORD rd = 0;
                    if (!WinHttpReadData(hr, &out[base], avail, &rd)) { ok = false; break; }
                    out.resize(base + rd);
                    if (out.size() > 32u * 1024 * 1024) { ok = false; break; } // cap
                } while (avail > 0);
            }
            WinHttpCloseHandle(hr);
        }
        WinHttpCloseHandle(hc);
    }
    WinHttpCloseHandle(hs);
    return ok && !out.empty();
}

// ==========================================================================
// Impl
// ==========================================================================

struct CMdWebHostImpl
{
    HWND parent = NULL;
    CMdWebHost::Callbacks cb;
    ComPtr<ICoreWebView2Environment> env;
    ComPtr<ICoreWebView2Controller> controller;
    ComPtr<ICoreWebView2> webview;
    EventRegistrationToken navTok{}, newWinTok{}, resTok{}, procTok{}, accelTok{}, zoomTok{}, navDoneTok{};
    bool ready = false;
    bool comInit = false;
    const MdHtmlResult* doc = nullptr;
    std::wstring docDir;
    std::wstring userDataFolder;
    int pendingZoom = 100;
    int docVersion = 0; // bumped on SetDocument; cache-busts the doc URL (search reload)
};

// the request path with any ?query / #fragment stripped
static std::wstring PathOnly(const std::wstring& u)
{
    size_t q = u.find_first_of(L"?#");
    return q == std::wstring::npos ? u : u.substr(0, q);
}

static void MakeAndSetResponse(CMdWebHostImpl* impl, ICoreWebView2WebResourceRequestedEventArgs* args,
                               const BYTE* data, size_t n, int status, const wchar_t* reason,
                               const wchar_t* headers)
{
    IStream* stream = NULL;
    if (data != NULL && n > 0)
        stream = SHCreateMemStream(data, (UINT)n);
    ComPtr<ICoreWebView2WebResourceResponse> resp;
    if (SUCCEEDED(impl->env->CreateWebResourceResponse(stream, status, reason, headers ? headers : L"", &resp)) && resp)
        args->put_Response(resp.Get());
    if (stream) stream->Release();
}

static void ServeRequest(CMdWebHostImpl* impl, ICoreWebView2WebResourceRequestedEventArgs* args)
{
    ComPtr<ICoreWebView2WebResourceRequest> req;
    if (FAILED(args->get_Request(&req)) || !req) return;
    LPWSTR uriRaw = NULL;
    req->get_Uri(&uriRaw);
    std::wstring u = uriRaw ? uriRaw : L"";
    if (uriRaw) CoTaskMemFree(uriRaw);

    const MdHtmlResult* doc = impl->doc;
    std::wstring path = PathOnly(u);

    if (path == kBase && doc != NULL)
    {
        MakeAndSetResponse(impl, args, (const BYTE*)doc->html.data(), doc->html.size(),
                           200, L"OK", L"Content-Type: text/html; charset=utf-8");
        return;
    }
    size_t pl = wcslen(kImgPrefix);
    if (doc != NULL && path.compare(0, pl, kImgPrefix) == 0)
    {
        int idx = _wtoi(path.c_str() + pl);
        if (idx >= 0 && idx < (int)doc->images.size())
        {
            const MdImageRef& ref = doc->images[idx];
            std::vector<BYTE> bytes;
            bool ok = (ref.kind == MdImageRef::Local) ? ReadFileBytes(ref.pathOrUrl, bytes)
                                                       : FetchRemote(ref.pathOrUrl, bytes);
            if (ok && !bytes.empty())
            {
                MakeAndSetResponse(impl, args, bytes.data(), bytes.size(), 200, L"OK",
                                   SniffContentType(bytes.data(), bytes.size()));
                return;
            }
        }
        MakeAndSetResponse(impl, args, NULL, 0, 404, L"Not Found", L"");
        return;
    }
    // default-deny: nothing else is ever served (invariant 3 / FR-052)
    MakeAndSetResponse(impl, args, NULL, 0, 403, L"Forbidden", L"");
}

static void ApplyControllerReady(CMdWebHostImpl* impl, ICoreWebView2Controller* ctl)
{
    impl->controller = ctl;
    if (FAILED(ctl->get_CoreWebView2(&impl->webview)) || !impl->webview)
    {
        if (impl->cb.OnInitFailed) impl->cb.OnInitFailed();
        return;
    }
    ICoreWebView2* wv = impl->webview.Get();

    // --- settings lockdown (FR-050..057) ---
    ComPtr<ICoreWebView2Settings> st;
    if (SUCCEEDED(wv->get_Settings(&st)) && st)
    {
        st->put_IsScriptEnabled(FALSE);
        st->put_AreDefaultContextMenusEnabled(FALSE);
        st->put_AreDevToolsEnabled(FALSE);
        st->put_IsStatusBarEnabled(FALSE);
        st->put_IsBuiltInErrorPageEnabled(FALSE);
        st->put_IsZoomControlEnabled(TRUE); // engine handles Ctrl+wheel / Ctrl+-+; synced via ZoomFactorChanged
        st->put_IsWebMessageEnabled(FALSE);
        st->put_AreHostObjectsAllowed(FALSE);
        ComPtr<ICoreWebView2Settings3> st3;
        if (SUCCEEDED(st.As(&st3)) && st3) st3->put_AreBrowserAcceleratorKeysEnabled(FALSE);
        ComPtr<ICoreWebView2Settings4> st4;
        if (SUCCEEDED(st.As(&st4)) && st4)
        {
            st4->put_IsGeneralAutofillEnabled(FALSE);
            st4->put_IsPasswordAutosaveEnabled(FALSE);
        }
        ComPtr<ICoreWebView2Settings5> st5;
        if (SUCCEEDED(st.As(&st5)) && st5) st5->put_IsPinchZoomEnabled(FALSE);
        ComPtr<ICoreWebView2Settings6> st6;
        if (SUCCEEDED(st.As(&st6)) && st6) st6->put_IsSwipeNavigationEnabled(FALSE);
        ComPtr<ICoreWebView2Settings8> st8;
        if (SUCCEEDED(st.As(&st8)) && st8) st8->put_IsReputationCheckingRequired(FALSE);
    }

    // --- navigation gate (cancel everything but our own document) ---
    wv->add_NavigationStarting(
        Callback<ICoreWebView2NavigationStartingEventHandler>(
            [impl](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                LPWSTR uri = NULL;
                args->get_Uri(&uri);
                std::wstring u = uri ? uri : L"";
                if (uri) CoTaskMemFree(uri);
                if (u.rfind(kBase, 0) == 0) return S_OK; // our doc + #fragments
                args->put_Cancel(TRUE);
                if (impl->cb.OnActivateLink) impl->cb.OnActivateLink(u);
                return S_OK;
            })
            .Get(),
        &impl->navTok);

    wv->add_NewWindowRequested(
        Callback<ICoreWebView2NewWindowRequestedEventHandler>(
            [impl](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT {
                args->put_Handled(TRUE);
                LPWSTR uri = NULL;
                args->get_Uri(&uri);
                std::wstring u = uri ? uri : L"";
                if (uri) CoTaskMemFree(uri);
                if (impl->cb.OnActivateLink) impl->cb.OnActivateLink(u);
                return S_OK;
            })
            .Get(),
        &impl->newWinTok);

    // --- offline content serving + default-deny net ---
    wv->add_WebResourceRequested(
        Callback<ICoreWebView2WebResourceRequestedEventHandler>(
            [impl](ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs* args) -> HRESULT {
                ServeRequest(impl, args);
                return S_OK;
            })
            .Get(),
        &impl->resTok);
    wv->AddWebResourceRequestedFilter(L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);

    wv->add_ProcessFailed(
        Callback<ICoreWebView2ProcessFailedEventHandler>(
            [impl](ICoreWebView2*, ICoreWebView2ProcessFailedEventArgs*) -> HRESULT {
                if (impl->cb.OnProcessFailed) impl->cb.OnProcessFailed();
                return S_OK;
            })
            .Get(),
        &impl->procTok);

    // --- focus the content once each navigation completes (so arrows/PgUp/PgDn
    //     scroll immediately after F3, without a mouse click) ---
    wv->add_NavigationCompleted(
        Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [impl](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs*) -> HRESULT {
                if (impl->controller)
                    impl->controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
                return S_OK;
            })
            .Get(),
        &impl->navDoneTok);

    // --- keep the persisted zoom + title in sync with engine-driven zoom
    //     (Ctrl+wheel, Ctrl+-+) ---
    ctl->add_ZoomFactorChanged(
        Callback<ICoreWebView2ZoomFactorChangedEventHandler>(
            [impl](ICoreWebView2Controller* sender, IUnknown*) -> HRESULT {
                double f = 1.0;
                if (SUCCEEDED(sender->get_ZoomFactor(&f)) && impl->cb.OnZoomChanged)
                    impl->cb.OnZoomChanged((int)(f * 100.0 + 0.5));
                return S_OK;
            })
            .Get(),
        &impl->zoomTok);

    // --- accelerator routing (focus lives inside the WebView2 HWND) ---
    ctl->add_AcceleratorKeyPressed(
        Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
            [impl](ICoreWebView2Controller*, ICoreWebView2AcceleratorKeyPressedEventArgs* args) -> HRESULT {
                COREWEBVIEW2_KEY_EVENT_KIND kind;
                if (FAILED(args->get_KeyEventKind(&kind))) return S_OK;
                if (kind != COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN &&
                    kind != COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_DOWN)
                    return S_OK;
                UINT vk = 0;
                args->get_VirtualKey(&vk);
                bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                int cmd = 0;
                if (vk == VK_F3) cmd = shift ? CM_EDIT_FINDPREV : CM_EDIT_FINDNEXT;
                else if (vk == VK_ESCAPE) cmd = CM_FILE_CLOSE;
                else if (vk == VK_F9) cmd = shift ? CM_SCHEME_PREV : CM_SCHEME_NEXT;
                else if (ctrl)
                {
                    if (vk == 'F') cmd = CM_EDIT_FIND;
                    else if (vk == 'U') cmd = CM_FILE_OPENTEXT;
                    // Ctrl+-+ / Ctrl+wheel are handled natively by the engine
                    // (IsZoomControlEnabled); we only own zoom RESET, because
                    // Ctrl+0 is a browser-accelerator key disabled by the lockdown.
                    else if (vk == '0' || vk == VK_NUMPAD0) cmd = CM_VIEW_ZOOMRESET;
                }
                if (cmd)
                {
                    args->put_Handled(TRUE);
                    PostMessage(impl->parent, WM_COMMAND, MAKEWPARAM(cmd, 0), 0);
                }
                return S_OK;
            })
            .Get(),
        &impl->accelTok);

    RECT rc;
    GetClientRect(impl->parent, &rc);
    ctl->put_Bounds(rc);
    ctl->put_ZoomFactor(impl->pendingZoom / 100.0);
    ctl->put_IsVisible(TRUE);
    ctl->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);

    impl->ready = true;
    if (impl->cb.OnReady) impl->cb.OnReady();
}

// ==========================================================================
// CMdWebHost
// ==========================================================================

CMdWebHost::CMdWebHost() : p(new CMdWebHostImpl()) {}
CMdWebHost::~CMdWebHost() { Destroy(); delete p; }

bool CMdWebHost::RuntimeAvailable()
{
    LPWSTR v = NULL;
    HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(NULL, &v);
    bool ok = SUCCEEDED(hr) && v != NULL && v[0] != 0;
    if (v) CoTaskMemFree(v);
    return ok;
}

bool CMdWebHost::Create(HWND parent, const std::wstring& userDataFolder, const Callbacks& cb)
{
    p->parent = parent;
    p->cb = cb;
    p->userDataFolder = userDataFolder;

    HRESULT hrco = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hrco)) p->comInit = true; // S_OK or S_FALSE -> balance in Destroy

    auto options = Make<CoreWebView2EnvironmentOptions>();
    options->put_AdditionalBrowserArguments(
        L"--disable-background-networking --disable-sync --disable-component-update "
        L"--disable-features=msWebOOUI,msPdfOOUI");

    CMdWebHostImpl* impl = p;
    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        NULL, userDataFolder.empty() ? NULL : userDataFolder.c_str(), options.Get(),
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [impl](HRESULT r, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(r) || !env)
                {
                    if (impl->cb.OnInitFailed) impl->cb.OnInitFailed();
                    return S_OK;
                }
                impl->env = env;
                HRESULT r2 = env->CreateCoreWebView2Controller(
                    impl->parent,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [impl](HRESULT rc, ICoreWebView2Controller* ctl) -> HRESULT {
                            if (FAILED(rc) || !ctl)
                            {
                                if (impl->cb.OnInitFailed) impl->cb.OnInitFailed();
                                return S_OK;
                            }
                            ApplyControllerReady(impl, ctl);
                            return S_OK;
                        })
                        .Get());
                if (FAILED(r2) && impl->cb.OnInitFailed) impl->cb.OnInitFailed();
                return S_OK;
            })
            .Get());

    if (FAILED(hr))
    {
        if (cb.OnInitFailed) cb.OnInitFailed();
        return false;
    }
    return true;
}

bool CMdWebHost::IsReady() const { return p->ready; }

void CMdWebHost::Resize(int cx, int cy)
{
    if (p->controller)
    {
        RECT rc = {0, 0, cx, cy};
        p->controller->put_Bounds(rc);
    }
}

void CMdWebHost::SetZoomPercent(int pct)
{
    p->pendingZoom = pct;
    if (p->controller) p->controller->put_ZoomFactor(pct / 100.0);
}

void CMdWebHost::SetDocument(const MdHtmlResult* doc, const std::wstring& docDir)
{
    p->doc = doc;
    p->docDir = docDir;
    p->docVersion++; // content changed -> next Navigate forces a full reload
}

void CMdWebHost::Navigate(const std::wstring& fragment)
{
    if (!p->webview) return;
    // The ?v=<version> query makes a term change (SetDocument bumped the version)
    // a fresh URL -> full reload with new <mark>s; a fragment-only change with the
    // same version is a same-document scroll (find next/prev).
    std::wstring url = kBase;
    url += L"?v=";
    wchar_t num[16];
    _itow_s(p->docVersion, num, 10);
    url += num;
    if (!fragment.empty()) { url += L"#"; url += fragment; }
    p->webview->Navigate(url.c_str());
}

void CMdWebHost::Focus()
{
    if (p->controller)
        p->controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
}

void CMdWebHost::Destroy()
{
    if (p->controller) { p->controller->Close(); p->controller.Reset(); }
    p->webview.Reset();
    p->env.Reset();
    p->ready = false;
    if (p->comInit) { CoUninitialize(); p->comInit = false; }
}
