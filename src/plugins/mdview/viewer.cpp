// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// viewer.cpp - the mdview viewer window: RichEdit host, thread/lock plumbing,
// menu, keyboard, search, zoom, color schemes, link security gate.

#include "precomp.h"
#include "render.h"
#include "viewer.h"

CWindowQueue ViewerWindowQueue("MDView Viewers");
CThreadQueue ThreadQueue("MDView Viewers");

static HACCEL ViewerAccels = NULL;
static HMODULE RichEditLib = NULL;

#define ID_RICHEDIT 1
#define SIZE_GATE (20LL * 1024 * 1024)

// ==========================================================================
// Init / release
// ==========================================================================

BOOL InitViewer()
{
    if (!InitializeWinLib(PluginNameEN, DLLInstance))
        return FALSE;
    SetWinLibStrings(LoadStr(IDS_INVALID_NUM), LoadStr(IDS_PLUGINNAME));

    RichEditLib = LoadLibraryW(L"Msftedit.dll"); // RichEdit 4.1 (RICHEDIT50W)
    if (RichEditLib == NULL)
        return FALSE;

    ACCEL acc[] = {
        {FVIRTKEY | FCONTROL, 'F', CM_EDIT_FIND},
        {FVIRTKEY, VK_F3, CM_EDIT_FINDNEXT},
        {FVIRTKEY | FSHIFT, VK_F3, CM_EDIT_FINDPREV},
        {FVIRTKEY, VK_ESCAPE, CM_FILE_CLOSE},
        {FVIRTKEY | FCONTROL, 'U', CM_FILE_OPENTEXT},
        {FVIRTKEY | FCONTROL, VK_OEM_PLUS, CM_VIEW_ZOOMIN},
        {FVIRTKEY | FCONTROL, VK_ADD, CM_VIEW_ZOOMIN},
        {FVIRTKEY | FCONTROL, VK_OEM_MINUS, CM_VIEW_ZOOMOUT},
        {FVIRTKEY | FCONTROL, VK_SUBTRACT, CM_VIEW_ZOOMOUT},
        {FVIRTKEY | FCONTROL, '0', CM_VIEW_ZOOMRESET},
        {FVIRTKEY, VK_F9, CM_SCHEME_NEXT},
        {FVIRTKEY | FSHIFT, VK_F9, CM_SCHEME_PREV},
    };
    ViewerAccels = CreateAcceleratorTable(acc, (int)(sizeof(acc) / sizeof(acc[0])));
    return TRUE;
}

void ReleaseViewer()
{
    if (ViewerAccels != NULL) { DestroyAcceleratorTable(ViewerAccels); ViewerAccels = NULL; }
    if (RichEditLib != NULL) { FreeLibrary(RichEditLib); RichEditLib = NULL; }
    ReleaseWinLib(DLLInstance);
}

// ==========================================================================
// Viewer thread (adapted from demoview)
// ==========================================================================

class CViewerThread : public CThread
{
protected:
    char* Name;
    int Left, Top, Width, Height;
    UINT ShowCmd;
    BOOL AlwaysOnTop, ReturnLock;
    HANDLE Continue;
    HANDLE* Lock;
    BOOL* LockOwner;
    BOOL* Success;
    int EnumFilesSourceUID, EnumFilesCurrentIndex;

public:
    CViewerThread(const char* name, int left, int top, int width, int height, UINT showCmd,
                  BOOL alwaysOnTop, BOOL returnLock, HANDLE* lock, BOOL* lockOwner, HANDLE contEvent,
                  BOOL* success, int enumFilesSourceUID, int enumFilesCurrentIndex)
        : CThread("MDView Viewer")
    {
        Name = _strdup(name);
        Left = left; Top = top; Width = width; Height = height;
        ShowCmd = showCmd; AlwaysOnTop = alwaysOnTop; ReturnLock = returnLock;
        Continue = contEvent; Lock = lock; LockOwner = lockOwner; Success = success;
        EnumFilesSourceUID = enumFilesSourceUID; EnumFilesCurrentIndex = enumFilesCurrentIndex;
    }
    virtual ~CViewerThread() { free(Name); }
    virtual unsigned Body();
};

unsigned CViewerThread::Body()
{
    CALL_STACK_MESSAGE1("CViewerThread::Body()");
    CViewerWindow* window = new CViewerWindow(EnumFilesSourceUID, EnumFilesCurrentIndex);
    if (window != NULL)
    {
        if (ReturnLock) { *Lock = window->GetLock(); *LockOwner = TRUE; }
        if (!ReturnLock || *Lock != NULL)
        {
            if (g_savePos && g_wndPlacement.length != 0)
            {
                WINDOWPLACEMENT place = g_wndPlacement;
                RECT monitorRect, workRect;
                SalamanderGeneral->MultiMonGetClipRectByRect(&place.rcNormalPosition, &workRect, &monitorRect);
                OffsetRect(&place.rcNormalPosition, workRect.left - monitorRect.left, workRect.top - monitorRect.top);
                SalamanderGeneral->MultiMonEnsureRectVisible(&place.rcNormalPosition, TRUE);
                Left = place.rcNormalPosition.left; Top = place.rcNormalPosition.top;
                Width = place.rcNormalPosition.right - place.rcNormalPosition.left;
                Height = place.rcNormalPosition.bottom - place.rcNormalPosition.top;
                ShowCmd = place.showCmd;
            }
            if (window->CreateEx(AlwaysOnTop ? WS_EX_TOPMOST : 0, CWINDOW_CLASSNAME2, "Markdown Viewer",
                                 WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, Left, Top, Width, Height,
                                 NULL, NULL, DLLInstance, window) != NULL)
            {
                ShowWindow(window->HWindow, ShowCmd);
                SetForegroundWindow(window->HWindow);
                UpdateWindow(window->HWindow);
                *Success = TRUE;
            }
            else if (ReturnLock && *Lock != NULL)
                HANDLES(CloseHandle(*Lock));
        }
    }

    BOOL openFile = *Success && Name != NULL;
    SetEvent(Continue);
    Continue = NULL; Lock = NULL; LockOwner = NULL; Success = NULL;

    if (openFile)
    {
        window->OpenFile(Name, FALSE);
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            if (!TranslateAccelerator(window->HWindow, ViewerAccels, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    if (window != NULL)
        delete window;
    return 0;
}

static void SpawnViewer(const char* utf8, int enumUID, int enumIdx,
                        BOOL returnLock, HANDLE* lock, BOOL* lockOwner)
{
    HANDLE contEvent = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
    if (contEvent == NULL)
        return;
    BOOL success = FALSE;
    CViewerThread* t = new CViewerThread(utf8, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                         SW_SHOWNORMAL, FALSE, returnLock, lock, lockOwner, contEvent,
                                         &success, enumUID, enumIdx);
    if (t != NULL)
    {
        if (t->Create(ThreadQueue) != NULL)
            WaitForSingleObject(contEvent, INFINITE);
        else
            delete t;
    }
    HANDLES(CloseHandle(contEvent));
}

// ==========================================================================
// CPluginInterfaceForViewer
// ==========================================================================

BOOL WINAPI CPluginInterfaceForViewer::CanViewFile(const char* name)
{
    WCHAR* wp = SplU8ToWExtAlloc(name);
    if (wp == NULL)
        return TRUE; // let ViewFile try
    HANDLE h = CreateFileW(wp, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    free(wp);
    if (h == INVALID_HANDLE_VALUE)
        return FALSE; // cascade to the internal text viewer
    BYTE buf[512]; DWORD rd = 0;
    ReadFile(h, buf, sizeof(buf), &rd, NULL);
    CloseHandle(h);
    bool bom = rd >= 2 && ((buf[0] == 0xFF && buf[1] == 0xFE) || (buf[0] == 0xFE && buf[1] == 0xFF));
    if (!bom)
        for (DWORD i = 0; i < rd; i++)
            if (buf[i] == 0)
                return FALSE; // binary -> let the text/hex viewer handle it
    return TRUE;
}

BOOL WINAPI CPluginInterfaceForViewer::ViewFile(const char* name, int left, int top, int width, int height,
                                                UINT showCmd, BOOL alwaysOnTop, BOOL returnLock, HANDLE* lock,
                                                BOOL* lockOwner, CSalamanderPluginViewerData* viewerData,
                                                int enumFilesSourceUID, int enumFilesCurrentIndex)
{
    HANDLE contEvent = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
    if (contEvent == NULL)
        return FALSE;
    BOOL success = FALSE;
    CViewerThread* t = new CViewerThread(name, left, top, width, height, showCmd, alwaysOnTop, returnLock,
                                         lock, lockOwner, contEvent, &success, enumFilesSourceUID,
                                         enumFilesCurrentIndex);
    if (t != NULL)
    {
        if (t->Create(ThreadQueue) != NULL)
            WaitForSingleObject(contEvent, INFINITE);
        else
            delete t;
    }
    HANDLES(CloseHandle(contEvent));
    return success;
}

// ==========================================================================
// Find dialog
// ==========================================================================

static INT_PTR CALLBACK FindDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);
        if (lParam)
            SetDlgItemTextW(hDlg, IDC_FIND_TEXT, (const WCHAR*)lParam);
        SetFocus(GetDlgItem(hDlg, IDC_FIND_TEXT));
        return FALSE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            WCHAR* buf = (WCHAR*)GetWindowLongPtr(hDlg, DWLP_USER);
            if (buf)
                GetDlgItemTextW(hDlg, IDC_FIND_TEXT, buf, 256);
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) { EndDialog(hDlg, IDCANCEL); return TRUE; }
        break;
    }
    return FALSE;
}

// ==========================================================================
// CViewerWindow
// ==========================================================================

CViewerWindow::CViewerWindow(int enumFilesSourceUID, int enumFilesCurrentIndex) : CWindow(ooStatic)
{
    Lock = NULL;
    Name = NULL;
    HRich = NULL;
    HSchemeMenu = NULL;
    Theme = MdThemeById(g_scheme);
    if (Theme == NULL) Theme = MdThemeDefault(false);
    Encoding = MDENC_UTF8;
    FindText[0] = 0;
    EnumFilesSourceUID = enumFilesSourceUID;
    EnumFilesCurrentIndex = enumFilesCurrentIndex;
}

CViewerWindow::~CViewerWindow()
{
    free(Name);
}

HANDLE CViewerWindow::GetLock()
{
    if (Lock == NULL)
        Lock = NOHANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
    return Lock;
}

const MdTheme* CViewerWindow::EffectiveTheme()
{
    if (g_followSys)
    {
        BOOL light = TRUE;
        HKEY k;
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
                          L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                          0, KEY_READ, &k) == ERROR_SUCCESS)
        {
            DWORD v = 1, sz = sizeof(v), type = 0;
            if (RegQueryValueExW(k, L"AppsUseLightTheme", NULL, &type, (LPBYTE)&v, &sz) == ERROR_SUCCESS)
                light = (v != 0);
            RegCloseKey(k);
        }
        const MdTheme* t = MdThemeById(light ? g_schemeLight : g_schemeDark);
        if (t) return t;
    }
    const MdTheme* t = MdThemeById(g_scheme);
    return t ? t : MdThemeDefault(false);
}

void CViewerWindow::BuildMenu()
{
    HMENU bar = CreateMenu();

    HMENU file = CreatePopupMenu();
    AppendMenuA(file, MF_STRING, CM_FILE_OPENTEXT, LoadStr(IDS_MENU_FILE_OPENTEXT));
    AppendMenuA(file, MF_SEPARATOR, 0, NULL);
    AppendMenuA(file, MF_STRING, CM_FILE_CLOSE, LoadStr(IDS_MENU_FILE_CLOSE));
    AppendMenuA(bar, MF_POPUP, (UINT_PTR)file, LoadStr(IDS_MENU_FILE));

    HMENU edit = CreatePopupMenu();
    AppendMenuA(edit, MF_STRING, CM_EDIT_COPY, LoadStr(IDS_MENU_EDIT_COPY));
    AppendMenuA(edit, MF_STRING, CM_EDIT_SELALL, LoadStr(IDS_MENU_EDIT_SELALL));
    AppendMenuA(edit, MF_SEPARATOR, 0, NULL);
    AppendMenuA(edit, MF_STRING, CM_EDIT_FIND, LoadStr(IDS_MENU_EDIT_FIND));
    AppendMenuA(edit, MF_STRING, CM_EDIT_FINDNEXT, LoadStr(IDS_MENU_EDIT_FINDNEXT));
    AppendMenuA(edit, MF_STRING, CM_EDIT_FINDPREV, LoadStr(IDS_MENU_EDIT_FINDPREV));
    AppendMenuA(bar, MF_POPUP, (UINT_PTR)edit, LoadStr(IDS_MENU_EDIT));

    HMENU view = CreatePopupMenu();
    HSchemeMenu = CreatePopupMenu();
    for (int i = 0; i < MdThemeCount; i++)
        AppendMenuA(HSchemeMenu, MF_STRING, CM_SCHEME_FIRST + i, LoadStr(MdThemes[i].nameStrId));
    AppendMenuA(HSchemeMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(HSchemeMenu, MF_STRING, CM_VIEW_FOLLOWSYS, LoadStr(IDS_MENU_VIEW_FOLLOWSYS));
    AppendMenuA(view, MF_POPUP, (UINT_PTR)HSchemeMenu, LoadStr(IDS_MENU_VIEW_SCHEME));
    AppendMenuA(view, MF_SEPARATOR, 0, NULL);
    AppendMenuA(view, MF_STRING, CM_VIEW_ZOOMIN, LoadStr(IDS_MENU_VIEW_ZOOMIN));
    AppendMenuA(view, MF_STRING, CM_VIEW_ZOOMOUT, LoadStr(IDS_MENU_VIEW_ZOOMOUT));
    AppendMenuA(view, MF_STRING, CM_VIEW_ZOOMRESET, LoadStr(IDS_MENU_VIEW_ZOOMRESET));
    AppendMenuA(bar, MF_POPUP, (UINT_PTR)view, LoadStr(IDS_MENU_VIEW));

    HMENU help = CreatePopupMenu();
    AppendMenuA(help, MF_STRING, CM_HELP_ABOUT, LoadStr(IDS_MENU_HELP_ABOUT));
    AppendMenuA(bar, MF_POPUP, (UINT_PTR)help, LoadStr(IDS_MENU_HELP));

    SetMenu(HWindow, bar);
    RefreshSchemeChecks();
}

void CViewerWindow::RefreshSchemeChecks()
{
    if (HSchemeMenu == NULL) return;
    const MdTheme* t = MdThemeById(g_scheme);
    int idx = t ? MdThemeIndex(t) : 0;
    CheckMenuRadioItem(HSchemeMenu, CM_SCHEME_FIRST, CM_SCHEME_FIRST + MdThemeCount - 1,
                       CM_SCHEME_FIRST + idx, MF_BYCOMMAND);
    CheckMenuItem(HSchemeMenu, CM_VIEW_FOLLOWSYS, MF_BYCOMMAND | (g_followSys ? MF_CHECKED : MF_UNCHECKED));
}

void CViewerWindow::UpdateTitle()
{
    std::wstring title = FilePathW;
    if (!title.empty()) title += L" - ";
    title += L"Markdown Viewer";
    if (Encoding == MDENC_ANSI) title += L" [ANSI]";
    else if (Encoding == MDENC_UTF16LE || Encoding == MDENC_UTF16BE) title += L" [UTF-16]";
    SetWindowTextW(HWindow, title.c_str());
}

static DWORD CALLBACK StreamInCb(DWORD_PTR cookie, LPBYTE buff, LONG cb, LONG* pcb)
{
    std::pair<const std::string*, size_t>* c = (std::pair<const std::string*, size_t>*)cookie;
    size_t rem = c->first->size() - c->second;
    LONG n = (LONG)(rem < (size_t)cb ? rem : (size_t)cb);
    if (n > 0) memcpy(buff, c->first->data() + c->second, n);
    c->second += n;
    *pcb = n;
    return 0;
}

void CViewerWindow::RenderDocument()
{
    Theme = EffectiveTheme();

    std::wstring text;
    bool loaded = false;
    std::vector<BYTE> bytes;

    WCHAR* wext = Name ? SplU8ToWExtAlloc(Name) : NULL;
    if (wext != NULL)
    {
        HANDLE h = CreateFileW(wext, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        free(wext);
        if (h != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER sz;
            if (GetFileSizeEx(h, &sz))
            {
                if (sz.QuadPart > SIZE_GATE)
                {
                    CloseHandle(h);
                    if (SalamanderGeneral->SalMessageBox(HWindow, LoadStr(IDS_TOO_LARGE),
                                                         LoadStr(IDS_WINDOW_TITLE),
                                                         MB_YESNO | MB_ICONQUESTION) == IDYES)
                        OpenAsText();
                    text = L"# " + std::wstring(L"Too large\n\nThis document exceeds the render limit.");
                    MdRenderMarkdown(text, *Theme, DocDir, Render);
                    goto stream;
                }
                DWORD n = (DWORD)sz.QuadPart;
                bytes.resize(n);
                DWORD rd = 0;
                if (ReadFile(h, n ? &bytes[0] : NULL, n, &rd, NULL)) { bytes.resize(rd); loaded = true; }
            }
            CloseHandle(h);
        }
    }

    if (!loaded)
    {
        text = L"# Cannot open\n\n";
        MdRenderMarkdown(text, *Theme, DocDir, Render);
        goto stream;
    }

    Encoding = MdDetectDecode(bytes.data(), bytes.size(), text);
    if (Encoding == MDENC_BINARY)
    {
        OpenAsText();
        text = L"# Not a text file\n\nOpened in the text viewer instead.";
    }
    MdRenderMarkdown(text, *Theme, DocDir, Render);

stream:
    SendMessage(HRich, WM_SETREDRAW, FALSE, 0);
    SendMessage(HRich, EM_SETREADONLY, FALSE, 0);
    {
        std::pair<const std::string*, size_t> ctx(&Render.rtf, 0);
        EDITSTREAM es;
        es.dwCookie = (DWORD_PTR)&ctx;
        es.dwError = 0;
        es.pfnCallback = StreamInCb;
        SendMessageW(HRich, EM_STREAMIN, SF_RTF, (LPARAM)&es);
    }
    // background per scheme
    SendMessage(HRich, EM_SETBKGNDCOLOR, 0, (LPARAM)Theme->docBg);

    // clickable links: apply CFE_LINK to each recorded range (capped)
    {
        SendMessage(HRich, EM_SETEVENTMASK, 0, ENM_LINK);
        CHARFORMAT2W cf;
        ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_LINK;
        cf.dwEffects = CFE_LINK;
        size_t limit = Render.linkRanges.size();
        if (limit > 5000) limit = 5000;
        for (size_t i = 0; i < limit; i++)
        {
            CHARRANGE cr;
            cr.cpMin = Render.linkRanges[i].cpMin;
            cr.cpMax = Render.linkRanges[i].cpMax;
            SendMessage(HRich, EM_EXSETSEL, 0, (LPARAM)&cr);
            SendMessageW(HRich, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
        }
    }

    SendMessage(HRich, EM_SETREADONLY, TRUE, 0);
    { CHARRANGE cr = {0, 0}; SendMessage(HRich, EM_EXSETSEL, 0, (LPARAM)&cr); }
    SendMessage(HRich, EM_SETZOOM, g_zoom, 100);
    SendMessage(HRich, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(HRich, NULL, TRUE);
    UpdateTitle();
}

void CViewerWindow::SetZoom(int pct)
{
    if (pct < 50) pct = 50;
    if (pct > 300) pct = 300;
    g_zoom = pct;
    SendMessage(HRich, EM_SETZOOM, g_zoom, 100);
}

void CViewerWindow::SelectScheme(int idx)
{
    if (idx < 0 || idx >= MdThemeCount) return;
    lstrcpynA(g_scheme, MdThemes[idx].id, sizeof(g_scheme));
    if (MdThemes[idx].dark) lstrcpynA(g_schemeDark, MdThemes[idx].id, sizeof(g_schemeDark));
    else lstrcpynA(g_schemeLight, MdThemes[idx].id, sizeof(g_schemeLight));
    RefreshSchemeChecks();
    // preserve scroll position
    LRESULT firstLine = SendMessage(HRich, EM_GETFIRSTVISIBLELINE, 0, 0);
    RenderDocument();
    LRESULT charIdx = SendMessage(HRich, EM_LINEINDEX, firstLine, 0);
    if (charIdx >= 0)
    {
        CHARRANGE cr = {(LONG)charIdx, (LONG)charIdx};
        SendMessage(HRich, EM_EXSETSEL, 0, (LPARAM)&cr);
        SendMessage(HRich, EM_SCROLLCARET, 0, 0);
    }
}

void CViewerWindow::CycleScheme(int dir)
{
    const MdTheme* t = MdThemeById(g_scheme);
    int idx = t ? MdThemeIndex(t) : 0;
    idx = (idx + dir + MdThemeCount) % MdThemeCount;
    SelectScheme(idx);
}

void CViewerWindow::DoFind(BOOL forward, BOOL prompt)
{
    if (prompt || FindText[0] == 0)
    {
        if (DialogBoxParamW(HLanguage, MAKEINTRESOURCEW(IDD_FIND), HWindow, FindDlgProc,
                            (LPARAM)FindText) != IDOK)
            return;
        if (FindText[0] == 0) return;
    }

    CHARRANGE sel;
    SendMessage(HRich, EM_EXGETSEL, 0, (LPARAM)&sel);
    FINDTEXTEXW ft;
    ZeroMemory(&ft, sizeof(ft));
    ft.lpstrText = FindText;
    DWORD flags = forward ? FR_DOWN : 0;
    if (forward) { ft.chrg.cpMin = sel.cpMax; ft.chrg.cpMax = -1; }
    else { ft.chrg.cpMin = sel.cpMin; ft.chrg.cpMax = 0; }

    LRESULT pos = SendMessageW(HRich, EM_FINDTEXTEXW, flags, (LPARAM)&ft);
    if (pos < 0)
    {
        // wrap once
        if (forward) { ft.chrg.cpMin = 0; ft.chrg.cpMax = -1; }
        else { ft.chrg.cpMin = -1; ft.chrg.cpMax = 0; }
        pos = SendMessageW(HRich, EM_FINDTEXTEXW, flags, (LPARAM)&ft);
    }
    if (pos >= 0)
    {
        SendMessage(HRich, EM_EXSETSEL, 0, (LPARAM)&ft.chrgText);
        SendMessage(HRich, EM_SCROLLCARET, 0, 0);
    }
    else
        SalamanderGeneral->SalMessageBox(HWindow, LoadStr(IDS_NOT_FOUND), LoadStr(IDS_WINDOW_TITLE),
                                         MB_OK | MB_ICONINFORMATION);
}

void CViewerWindow::OpenAsText()
{
    if (Name == NULL) return;
    CSalamanderPluginInternalViewerData data;
    ZeroMemory(&data, sizeof(data));
    data.Size = sizeof(data);
    data.FileName = Name;
    data.Mode = 0;
    data.Caption = NULL;
    data.WholeCaption = FALSE;
    int err = 0;
    SalamanderGeneral->ViewFileInPluginViewer(NULL, &data, FALSE, NULL, Name, err);
}

void CViewerWindow::ActivateLinkByCp(long cp)
{
    int linkId = -1;
    for (const MdLinkRange& r : Render.linkRanges)
        if (cp >= r.cpMin && cp < r.cpMax) { linkId = r.linkId; break; }
    if (linkId < 0 || linkId >= (int)Render.links.size()) return;
    const MdLinkEntry& e = Render.links[linkId];

    // internal anchor -> scroll
    if (e.internalAnchor)
    {
        for (const MdAnchorEntry& a : Render.anchors)
            if (a.slug == e.url)
            {
                CHARRANGE cr = {a.charPos, a.charPos};
                SendMessage(HRich, EM_EXSETSEL, 0, (LPARAM)&cr);
                SendMessage(HRich, EM_SCROLLCARET, 0, 0);
                return;
            }
        return; // missing anchor: no-op (FR-012)
    }

    // scheme allowlist
    std::wstring url = e.url;
    size_t colon = url.find(L':');
    bool hasScheme = false;
    std::wstring scheme;
    if (colon != std::wstring::npos)
    {
        scheme = url.substr(0, colon);
        hasScheme = !scheme.empty();
        for (wchar_t& ch : scheme) ch = (wchar_t)towlower(ch);
        for (wchar_t ch : scheme)
            if (!iswalpha(ch)) { hasScheme = false; break; }
    }

    if (hasScheme)
    {
        if (scheme == L"http" || scheme == L"https" || scheme == L"mailto" || scheme == L"ftp")
            ShellExecuteW(HWindow, L"open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
        else
            SalamanderGeneral->SalMessageBox(HWindow, LoadStr(IDS_LINK_BLOCKED),
                                             LoadStr(IDS_WINDOW_TITLE), MB_OK | MB_ICONWARNING);
        return;
    }

    // relative local link: open .md/.markdown in a new mdview window; others = blocked
    std::wstring lower = url;
    for (wchar_t& ch : lower) ch = (wchar_t)towlower(ch);
    bool isMd = (lower.size() > 3 && lower.substr(lower.size() - 3) == L".md") ||
                (lower.size() > 9 && lower.substr(lower.size() - 9) == L".markdown");
    // strip a #fragment
    size_t hash = url.find(L'#');
    if (hash != std::wstring::npos) url = url.substr(0, hash);
    if (isMd && !DocDir.empty() && url.find(L':') == std::wstring::npos &&
        url.substr(0, 2) != L"\\\\")
    {
        std::wstring full = DocDir;
        if (!full.empty() && full.back() != L'\\') full += L'\\';
        for (wchar_t& ch : url) if (ch == L'/') ch = L'\\';
        full += url;
        char* u8 = SplWToU8Alloc(full.c_str());
        if (u8 != NULL) { SpawnViewer(u8, -1, -1, FALSE, NULL, NULL); free(u8); }
        return;
    }
    SalamanderGeneral->SalMessageBox(HWindow, LoadStr(IDS_LINK_BLOCKED),
                                     LoadStr(IDS_WINDOW_TITLE), MB_OK | MB_ICONWARNING);
}

void CViewerWindow::OpenFile(const char* name, BOOL setLock)
{
    if (setLock && Lock != NULL)
    {
        SetEvent(Lock);
        Lock = NULL;
    }
    free(Name);
    Name = _strdup(name);

    // display path + directory
    WCHAR* wp = SplU8ToWAlloc(name);
    if (wp != NULL)
    {
        FilePathW = wp;
        std::wstring d = wp;
        size_t slash = d.find_last_of(L"\\/");
        DocDir = (slash != std::wstring::npos) ? d.substr(0, slash) : L"";
        free(wp);
    }
    RenderDocument();
}

LRESULT CViewerWindow::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        HRich = CreateWindowExW(0, L"RICHEDIT50W", L"",
                                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_NOHIDESEL,
                                0, 0, 0, 0, HWindow, (HMENU)ID_RICHEDIT, DLLInstance, NULL);
        if (HRich == NULL)
            return -1;
        SendMessageW(HRich, EM_AUTOURLDETECT, FALSE, 0);
        SendMessage(HRich, EM_SETEVENTMASK, 0, ENM_LINK);
        SendMessage(HRich, EM_EXLIMITTEXT, 0, 0x7FFFFFFF);
        BuildMenu();
        ViewerWindowQueue.Add(new CWindowQueueItem(HWindow));
        break;
    }

    case WM_SIZE:
    {
        if (HRich != NULL)
        {
            RECT r;
            GetClientRect(HWindow, &r);
            MoveWindow(HRich, 0, 0, r.right, r.bottom, TRUE);
        }
        break;
    }

    case WM_SETFOCUS:
        if (HRich != NULL) SetFocus(HRich);
        break;

    case WM_NOTIFY:
    {
        NMHDR* nh = (NMHDR*)lParam;
        if (nh != NULL && nh->idFrom == ID_RICHEDIT && nh->code == EN_LINK)
        {
            ENLINK* el = (ENLINK*)lParam;
            if (el->msg == WM_LBUTTONUP)
            {
                ActivateLinkByCp(el->chrg.cpMin);
                return TRUE;
            }
        }
        break;
    }

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        if (id >= CM_SCHEME_FIRST && id < CM_SCHEME_FIRST + MdThemeCount)
        {
            SelectScheme(id - CM_SCHEME_FIRST);
            return 0;
        }
        switch (id)
        {
        case CM_FILE_CLOSE: DestroyWindow(HWindow); return 0;
        case CM_FILE_OPENTEXT: OpenAsText(); return 0;
        case CM_EDIT_COPY: SendMessage(HRich, WM_COPY, 0, 0); return 0;
        case CM_EDIT_SELALL: { CHARRANGE cr = {0, -1}; SendMessage(HRich, EM_EXSETSEL, 0, (LPARAM)&cr); return 0; }
        case CM_EDIT_FIND: DoFind(TRUE, TRUE); return 0;
        case CM_EDIT_FINDNEXT: DoFind(TRUE, FALSE); return 0;
        case CM_EDIT_FINDPREV: DoFind(FALSE, FALSE); return 0;
        case CM_VIEW_FOLLOWSYS: g_followSys = !g_followSys; RefreshSchemeChecks(); RenderDocument(); return 0;
        case CM_VIEW_ZOOMIN: SetZoom(g_zoom + 10); return 0;
        case CM_VIEW_ZOOMOUT: SetZoom(g_zoom - 10); return 0;
        case CM_VIEW_ZOOMRESET: SetZoom(100); return 0;
        case CM_SCHEME_NEXT: CycleScheme(1); return 0;
        case CM_SCHEME_PREV: CycleScheme(-1); return 0;
        case CM_HELP_ABOUT: OnAbout(HWindow); return 0;
        }
        break;
    }

    case WM_MOUSEWHEEL:
    {
        if (GetKeyState(VK_CONTROL) & 0x8000)
        {
            short delta = GET_WHEEL_DELTA_WPARAM(wParam);
            SetZoom(g_zoom + (delta > 0 ? 10 : -10));
            return 0;
        }
        break;
    }

    case WM_USER_VIEWERCFGCHNG:
        RefreshSchemeChecks();
        RenderDocument();
        return 0;

    case WM_DESTROY:
    {
        if (g_savePos)
        {
            g_wndPlacement.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(HWindow, &g_wndPlacement);
        }
        ViewerWindowQueue.Remove(HWindow);
        PostQuitMessage(0);
        break;
    }
    }
    return CWindow::WindowProc(uMsg, wParam, lParam);
}
