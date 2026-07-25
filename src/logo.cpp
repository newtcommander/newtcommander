// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// CommentsTranslationProject: TRANSLATED

#include "precomp.h"
#include "mainwnd.h"
#include "plugins.h"
#include "fileswnd.h"
#include "cfgdlg.h"
#include "dialogs.h"
#include "gui.h"
#include "md5.h"

#include <uxtheme.h>
#include <vssym32.h>
#include <ppl.h>

#include "svg.h"
#include "pngimage.h"
#include "themes.h"

#include "versinfo.rh2"

// Newt Commander brand palette (feature 032, see tools/brand/README.md);
// the wordmark is drawn with GDI so no font has to be installed/shipped
#define NC_COLOR_NAVY RGB(0x0A, 0x14, 0x24)           // brand navy background
#define NC_COLOR_TEXT_DARKBG RGB(0xEA, 0xF2, 0xFB)    // "Newt" + regular text on dark background
#define NC_COLOR_ORANGE_DARKBG RGB(0xF9, 0x73, 0x16)  // "Commander" on dark background
#define NC_COLOR_MUTED_DARKBG RGB(0x8F, 0xA6, 0xC4)   // version/tagline on dark background
#define NC_COLOR_TEXT_LIGHTBG RGB(0x0A, 0x14, 0x24)   // "Newt" + regular text on light background
#define NC_COLOR_ORANGE_LIGHTBG RGB(0xEA, 0x6A, 0x0B) // "Commander" on light background
#define NC_COLOR_MUTED_LIGHTBG RGB(0x5D, 0x82, 0xB8)  // version/tagline on light background

// draws the "Newt Commander" wordmark into 'r' (left-aligned, vertically centered);
// shrinks the font until both parts fit the rect width
static void NCDrawWordmark(HDC hDC, const RECT* r, COLORREF newtClr, COLORREF commanderClr)
{
    const char* part1 = "Newt ";
    const char* part2 = "Commander";
    int rectW = r->right - r->left;
    int rectH = r->bottom - r->top;

    LOGFONT lf;
    memset(&lf, 0, sizeof(lf));
    lf.lfWeight = FW_BOLD;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
    strcpy(lf.lfFaceName, "Segoe UI");

    int oldBkMode = SetBkMode(hDC, TRANSPARENT);
    SIZE s1, s2;
    HFONT hFont = NULL;
    HFONT hOldFont = NULL;
    int height = MulDiv(rectH, 55, 100);
    for (;;)
    {
        lf.lfHeight = -height;
        hFont = HANDLES(CreateFontIndirect(&lf));
        HFONT hPrev = (HFONT)SelectObject(hDC, hFont);
        if (hOldFont == NULL)
            hOldFont = hPrev;
        GetTextExtentPoint32(hDC, part1, (int)strlen(part1), &s1);
        GetTextExtentPoint32(hDC, part2, (int)strlen(part2), &s2);
        if (s1.cx + s2.cx <= rectW || height <= 10)
            break;
        SelectObject(hDC, hOldFont);
        HANDLES(DeleteObject(hFont));
        height = MulDiv(height, 9, 10);
    }

    int y = r->top + (rectH - s1.cy) / 2;
    COLORREF oldClr = SetTextColor(hDC, newtClr);
    TextOut(hDC, r->left, y, part1, (int)strlen(part1));
    SetTextColor(hDC, commanderClr);
    TextOut(hDC, r->left + s1.cx, y, part2, (int)strlen(part2));

    SetTextColor(hDC, oldClr);
    SetBkMode(hDC, oldBkMode);
    SelectObject(hDC, hOldFont);
    HANDLES(DeleteObject(hFont));
}

void GetDlgItemRectAndDestroy(HWND hWindow, int resID, RECT* r)
{
    HWND hItem = GetDlgItem(hWindow, resID);
    if (hItem == NULL)
    {
        r->left = r->top = r->right = r->bottom = 0;
        return;
    }
    GetWindowRect(hItem, r);
    MapWindowPoints(NULL, hWindow, (POINT*)r, 2);
    DestroyWindow(hItem);
}

//*****************************************************************************
//
// Splash Screen
//

#define SPLASH_WIDTH_DLGUNITS 270
#define SPLASH_HEIGHT_DLGUNITS 72

CSplashScreen::CSplashScreen()
    : CDialog(HInstance, IDD_SPLASH, NULL, ooStatic)
{
    Bitmap = NULL;
    OriginalBitmap = NULL;
    HNormalFont = NULL;
    HBoldFont = NULL;
    GradientY = 0;
    Width = 0;
    Height = 0;

    // create a font
    LOGFONT lf;
    //  GetSystemGUIFont(&lf);
    //  lf.lfWeight = FW_NORMAL;

    HDC hDC2 = HANDLES(GetDC(NULL));
    lf.lfHeight = -MulDiv(8, GetDeviceCaps(hDC2, LOGPIXELSY), 72);
    HANDLES(ReleaseDC(NULL, hDC2));
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfItalic = 0;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
    strcpy(lf.lfFaceName, "MS Shell Dlg 2");

    HNormalFont = HANDLES(CreateFontIndirect(&lf));
    // create the bold variant
    lf.lfWeight = FW_BOLD;
    HBoldFont = HANDLES(CreateFontIndirect(&lf));
}

CSplashScreen::~CSplashScreen()
{
    // by this time the bitmap should be destroyed, but do it again just in case
    DestroyBitmap();
    if (HNormalFont != NULL)
        HANDLES(DeleteObject(HNormalFont));
    if (HBoldFont != NULL)
        HANDLES(DeleteObject(HBoldFont));
}

BOOL CSplashScreen::PaintText(const char* text, int x, int y, BOOL bold, COLORREF clr)
{
    HDC hDC = NULL;
    if (Bitmap != NULL)
        hDC = Bitmap->HMemDC;
    if (hDC != NULL)
    {
        RECT r;
        r.left = x;
        r.top = y;
        r.right = x + Width;
        r.bottom = y + Height;
        int oldBkMode = SetBkMode(hDC, TRANSPARENT);
        COLORREF oldTextColor = SetTextColor(hDC, clr);
        HFONT hOldFont = (HFONT)SelectObject(hDC, bold ? HBoldFont : HNormalFont);
        DrawText(hDC, text, -1, &r, DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
        SelectObject(hDC, hOldFont);
        SetTextColor(hDC, oldTextColor);
        SetBkMode(hDC, oldBkMode);
    }
    return TRUE;
}

BOOL CSplashScreen::PrepareBitmap()
{
    Bitmap = new CBitmap();
    OriginalBitmap = new CBitmap();
    if (Bitmap != NULL && OriginalBitmap != NULL)
    {
        HDC hDC = HANDLES(GetDC(NULL));
        if (!Bitmap->CreateBmp(hDC, Width, Height))
        {
            delete Bitmap;
            Bitmap = NULL;
        }
        if (!OriginalBitmap->CreateBmp(hDC, Width, Height))
        {
            delete OriginalBitmap;
            OriginalBitmap = NULL;
        }
        HANDLES(ReleaseDC(NULL, hDC));
    }
    else
        return FALSE;

    HDC hDC = Bitmap->HMemDC;

    RECT r;
    r.left = 0;
    r.top = 0;
    r.right = Width;
    r.bottom = Height;

    // the splash always uses the brand dark look (theme configuration may not be
    // loaded yet this early during startup)
    SetBkColor(hDC, NC_COLOR_NAVY);
    ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &r, "", 0, NULL);

    CSVGSprite svgGrad;
    CPngImage pngLogo; // hand-swappable PNG artwork (feature 035, src/res/logo.png)
    concurrency::parallel_invoke(
        [&]
        { svgGrad.Load(IDB_LOGO_GRAD, Width, -1, SVGSTATE_ORIGINAL); },
        [&]
        // the artwork sits above the accent line so the texts below stay clear of it
        { pngLogo.Load(IDB_LOGO_IMAGE, -1, GradientY - 12); });

    SIZE gradSize, logoSize;
    svgGrad.GetSize(&gradSize);
    pngLogo.GetSize(&logoSize);

    // thin brand accent line (blue -> orange) instead of the old full gradient area
    svgGrad.AlphaBlend(hDC, 0, GradientY, gradSize.cx, max(2, gradSize.cy), SVGSTATE_ORIGINAL);
    pngLogo.AlphaBlend(hDC, Width - logoSize.cx - 8, 6, logoSize.cx, logoSize.cy);

    // product wordmark drawn with GDI (no font dependency, see NCDrawWordmark)
    NCDrawWordmark(hDC, &OpenSalR, NC_COLOR_TEXT_DARKBG, NC_COLOR_ORANGE_DARKBG);

    // fixed texts
    PaintText(SALAMANDER_TEXT_VERSION,
              VersionR.left,
              VersionR.top,
              FALSE, NC_COLOR_MUTED_DARKBG);

    // the copyright has two authorship parts; a single line does not fit the
    // splash width, so each part gets its own line (feature 035)
    PaintText(VERSINFO_COPYRIGHT1,
              CopyrightR.left,
              CopyrightR.top,
              TRUE, NC_COLOR_TEXT_DARKBG);

    PaintText(VERSINFO_COPYRIGHT2,
              Copyright2R.left,
              Copyright2R.top,
              TRUE, NC_COLOR_TEXT_DARKBG);

    // backup of the bitmap without text
    BitBlt(OriginalBitmap->HMemDC, 0, 0, Width, Height, Bitmap->HMemDC, 0, 0, SRCCOPY);

    return TRUE;
}

void CSplashScreen::SetText(const char* text)
{
    if (Bitmap != NULL && OriginalBitmap != NULL)
    {
        // restore the background
        BitBlt(Bitmap->HMemDC, StatusR.left, StatusR.top, StatusR.right - StatusR.left, StatusR.bottom - StatusR.top, OriginalBitmap->HMemDC, StatusR.left, StatusR.top, SRCCOPY);
        PaintText(text,
                  StatusR.left, StatusR.top,
                  FALSE, RGB(255, 255, 255));

        // if visible, update the display with the change
        if (HWindow != NULL)
        {
            HDC hDC = HANDLES(GetDC(HWindow));
            BitBlt(hDC, StatusR.left, StatusR.top, StatusR.right - StatusR.left, StatusR.bottom - StatusR.top, Bitmap->HMemDC, StatusR.left, StatusR.top, SRCCOPY);
            HANDLES(ReleaseDC(HWindow, hDC));
        }
    }
}

void CSplashScreen::DestroyBitmap()
{
    if (Bitmap != NULL)
    {
        delete Bitmap;
        Bitmap = NULL;
    }
    if (OriginalBitmap != NULL)
    {
        delete OriginalBitmap;
        OriginalBitmap = NULL;
    }
}

INT_PTR
CSplashScreen::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CSplashScreen::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        RECT r;
        GetClientRect(HWindow, &r);
        Width = r.right - r.left;
        Height = r.bottom - r.top;

        GetDlgItemRectAndDestroy(HWindow, IDC_SPLASH_OPENSAL, &OpenSalR);
        GetDlgItemRectAndDestroy(HWindow, IDC_SPLASH_VERSION, &VersionR);
        GetDlgItemRectAndDestroy(HWindow, IDC_SPLASH_COPYRIGHT, &CopyrightR);
        GetDlgItemRectAndDestroy(HWindow, IDC_SPLASH_COPYRIGHT2, &Copyright2R);
        GetDlgItemRectAndDestroy(HWindow, IDC_SPLASH_STATUS, &StatusR);

        GradientY = VersionR.bottom + 5;

        TRACE_I("!!!!!!!!!!!!! BEG ");
        PrepareBitmap();
        TRACE_I("!!!!!!!!!!!!! END ");
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HANDLES(BeginPaint(HWindow, &ps));
        BitBlt(ps.hdc, 0, 0, Width, Height, Bitmap->HMemDC, 0, 0, SRCCOPY);
        HANDLES(EndPaint(HWindow, &ps));
        return FALSE;
    }

    case WM_ERASEBKGND:
    {
        return TRUE;
    }
    }

    return CDialog::DialogProc(uMsg, wParam, lParam);
}

CSplashScreen SplashScreen;

BOOL SplashScreenOpen()
{
    if (SplashScreen.HWindow != NULL)
    {
        TRACE_E("SplashScreenOpen(): splash screen already exists!");
        return FALSE;
    }

    SplashScreen.Create();

    if (SplashScreen.HWindow != NULL)
    {
        MultiMonCenterWindow(SplashScreen.HWindow, NULL, FALSE);
        ShowWindow(SplashScreen.HWindow, SW_SHOWNOACTIVATE);
        UpdateWindow(SplashScreen.HWindow);
        return TRUE;
    }
    else
    {
        SplashScreen.DestroyBitmap();
        return FALSE;
    }
}

void SplashScreenCloseIfExist()
{
    if (SplashScreen.HWindow != NULL)
    {
        DestroyWindow(SplashScreen.HWindow);
        SplashScreen.DestroyBitmap();
    }
}

BOOL ExistSplashScreen()
{
    return (SplashScreen.HWindow != NULL);
}

void IfExistSetSplashScreenText(const char* text)
{
    if (SplashScreen.HWindow != NULL)
        SplashScreen.SetText(text);
}

HWND GetSplashScreenHandle()
{
    return SplashScreen.HWindow;
}

//
// ****************************************************************************
// CAboutDialog
//

CAboutDialog::CAboutDialog(HWND parent)
    : CCommonDialog(HLanguage, IDD_ABOUT, parent)
{
    // must match the dialog background painted in AboutAndEvalDlgCreateBkgnd
    HGradientBkBrush = HANDLES(CreateSolidBrush(IsDarkThemeActive() ? NC_COLOR_NAVY : RGB(255, 255, 255)));
    BackgroundBitmap = NULL;
}

CAboutDialog::~CAboutDialog()
{
    if (BackgroundBitmap != NULL)
        delete BackgroundBitmap;
    HANDLES(DeleteObject(HGradientBkBrush));
}

HDWP OffsetControl(HWND hWindow, HDWP hdwp, int id, int yOffset)
{
    HWND hCtrl = GetDlgItem(hWindow, id);
    RECT r;
    GetWindowRect(hCtrl, &r);
    ScreenToClient(hWindow, (LPPOINT)&r);

    hdwp = HANDLES(DeferWindowPos(hdwp, hCtrl, NULL, r.left, r.top + yOffset, 0, 0, SWP_NOSIZE | SWP_NOZORDER));
    return hdwp;
}

CBitmap*
AboutAndEvalDlgCreateBkgnd(HWND hWindow)
{
    RECT opensalR;
    RECT logoR;
    RECT gradR;
    GetDlgItemRectAndDestroy(hWindow, IDC_ABOUT_OPENSAL, &opensalR);
    GetDlgItemRectAndDestroy(hWindow, IDC_ABOUT_LOGO, &logoR);
    GetDlgItemRectAndDestroy(hWindow, IDC_ABOUT_BOTTOM, &gradR);

    RECT r;
    GetClientRect(hWindow, &r);

    CBitmap* bitmap = new CBitmap();
    HDC hDC = HANDLES(GetDC(NULL));
    if (!bitmap->CreateBmp(hDC, r.right - r.left, r.bottom - r.top))
    {
        delete bitmap;
        bitmap = NULL;
    }
    HANDLES(ReleaseDC(NULL, hDC));
    if (bitmap == NULL)
        return NULL;

    hDC = bitmap->HMemDC;

    // theme-aware background (feature 032: About follows the application theme)
    BOOL dark = IsDarkThemeActive();
    SetBkColor(hDC, dark ? NC_COLOR_NAVY : RGB(255, 255, 255));
    ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &r, "", 0, NULL);

    CSVGSprite svgGrad;
    CPngImage pngLogo; // hand-swappable PNG artwork (feature 035, src/res/logo.png)
    concurrency::parallel_invoke(
        [&]
        { svgGrad.Load(IDB_ABOUT_GRAD, r.right - r.left, -1, SVGSTATE_ORIGINAL); },
        [&]
        { pngLogo.Load(IDB_LOGO_IMAGE, logoR.right - logoR.left, logoR.bottom - logoR.top); });

    SIZE gradSize, logoSize;
    svgGrad.GetSize(&gradSize);
    pngLogo.GetSize(&logoSize);

    // thin brand accent line (blue -> orange) at the position of the old gradient area
    svgGrad.AlphaBlend(hDC, 0, gradR.top, gradSize.cx, max(2, gradSize.cy), SVGSTATE_ORIGINAL);
    pngLogo.AlphaBlend(hDC, r.right - r.left - logoSize.cx, 0, logoSize.cx, logoSize.cy);

    // product wordmark drawn with GDI (no font dependency)
    NCDrawWordmark(hDC, &opensalR,
                   dark ? NC_COLOR_TEXT_DARKBG : NC_COLOR_TEXT_LIGHTBG,
                   dark ? NC_COLOR_ORANGE_DARKBG : NC_COLOR_ORANGE_LIGHTBG);

    return bitmap;
}

void AboutAndEvalDlgPaintBkgnd(HWND hWindow, HDC hDC, CBitmap* bitmap)
{
    BitBlt(hDC, 0, 0, bitmap->GetWidth(), bitmap->GetHeight(), bitmap->HMemDC, 0, 0, SRCCOPY);
}

INT_PTR
CAboutDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CAboutDialog::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        CHyperLink* hl;

        SetDlgItemText(HWindow, IDS_ABOUT_SALAMANDER, SALAMANDER_TEXT_VERSION);
        new CStaticText(HWindow, IDS_ABOUT_SALAMANDER, STF_BOLD);
        //      new CStaticText(HWindow, IDS_ABOUT_FIRM, STF_BOLD);

        hl = new CHyperLink(HWindow, IDC_ABOUT_WWW);
        if (hl != NULL)
        {
            const char* url = "https://newtcommander.org";
            SetDlgItemText(HWindow, IDC_ABOUT_WWW, url + 8);
            hl->SetActionOpen(url);
        }

        BackgroundBitmap = AboutAndEvalDlgCreateBkgnd(HWindow);
        break;
    }

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;
        int resID = GetWindowLong(hwndStatic, GWL_ID);
        BOOL dark = IsDarkThemeActive();
        COLORREF textClr = dark ? NC_COLOR_TEXT_DARKBG : RGB(70, 70, 70);
        switch (resID)
        {
        case IDC_STATIC_6:
        case IDC_STATIC_7:
        case IDC_STATIC_8:
            textClr = dark ? NC_COLOR_MUTED_DARKBG : RGB(128, 128, 128);
            break;
        }
        SetTextColor(hdcStatic, textClr);
        SetBkColor(hdcStatic, dark ? NC_COLOR_NAVY : RGB(255, 255, 255));
        return (BOOL)(UINT_PTR)GetStockObject(NULL_BRUSH);
    }

    case WM_CTLCOLORBTN:
    {
        if (IsAppThemed())
        {
            // without this workaround there's a gray frame around the OK button
            if (WindowsVistaAndLater)
                return (BOOL)(UINT_PTR)HGradientBkBrush;
            else
                return (BOOL)(UINT_PTR)GetStockObject(NULL_BRUSH); // under XP this still worked fine
        }
        break;
    }

    case WM_ERASEBKGND:
    {
        HDC hDC = (HDC)wParam;
        AboutAndEvalDlgPaintBkgnd(HWindow, hDC, BackgroundBitmap);
        return TRUE;
    }

    case WM_NCHITTEST:
    {
        SetWindowLongPtr(HWindow, DWLP_MSGRESULT, HTCAPTION);
        return HTCAPTION;
    }
    }
    return CCommonDialog::DialogProc(uMsg, wParam, lParam);
}
