// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include <dwmapi.h>
#include <uxtheme.h>

#include "cfgdlg.h"
#include "usermenu.h"
#include "execute.h"
#include "plugins.h"
#include "fileswnd.h"
#include "mainwnd.h"
#include "themes.h"
#include "themes_palette.h"

#pragma comment(lib, "Dwmapi.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

// window property marking windows we have dark-themed; lets the Default
// theme stay a pure no-op for windows that were never touched
static const char* THEME_DARKENED_PROP = "SalThemeDark";

//
// ****************************************************************************
// High Contrast state
//

static BOOL ThemeHighContrast = FALSE;
static BOOL ThemeHighContrastValid = FALSE;

static void InitThemeDarkSysColors();

void RefreshThemeHighContrastState()
{
    HIGHCONTRAST hc;
    memset(&hc, 0, sizeof(hc));
    hc.cbSize = sizeof(hc);
    ThemeHighContrast = SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0) &&
                        (hc.dwFlags & HCF_HIGHCONTRASTON) != 0;
    ThemeHighContrastValid = TRUE;
    InitThemeDarkSysColors(); // eager init: the viewer thread reads the LUT
}

BOOL IsDarkThemeActive()
{
    if (Configuration.ThemeMode != THEME_MODE_DARK)
        return FALSE;
    if (!ThemeHighContrastValid)
        RefreshThemeHighContrastState();
    return !ThemeHighContrast; // High Contrast wins over the application theme
}

//
// ****************************************************************************
// Dark chrome palette (specs/028-visual-themes/data-model.md, section 5)
//

#define THEME_SYSCOLOR_COUNT 31 // COLOR_3DDKSHADOW(21)..COLOR_MENUBAR(30) fit below 31

static COLORREF ThemeDarkSysColors[THEME_SYSCOLOR_COUNT];
static BOOL ThemeDarkSysColorsValid = FALSE;

static void InitThemeDarkSysColors()
{
    int i;
    for (i = 0; i < THEME_SYSCOLOR_COUNT; i++)
        ThemeDarkSysColors[i] = CLR_INVALID; // unmapped -> GetSysColor fallback

#define THEME_SET_ENTRY(idx, r, g, b) ThemeDarkSysColors[idx] = RGB(r, g, b);
    THEME_DARK_SYSCOLORS(THEME_SET_ENTRY)
#undef THEME_SET_ENTRY

    ThemeDarkSysColorsValid = TRUE;
}

COLORREF ThemeSysColor(int index)
{
    if (!IsDarkThemeActive())
        return GetSysColor(index);
    if (!ThemeDarkSysColorsValid)
        InitThemeDarkSysColors();
    if (index >= 0 && index < THEME_SYSCOLOR_COUNT && ThemeDarkSysColors[index] != CLR_INVALID)
        return ThemeDarkSysColors[index];
    return GetSysColor(index);
}

//
// ****************************************************************************
// Brush cache (engine-owned solid brushes for the dark palette)
//

static HBRUSH ThemeDarkBrushes[THEME_SYSCOLOR_COUNT] = {0};

HBRUSH ThemeSysColorBrush(int index)
{
    if (!IsDarkThemeActive())
        return GetSysColorBrush(index);
    if (index < 0 || index >= THEME_SYSCOLOR_COUNT)
        return GetSysColorBrush(index);
    if (ThemeDarkBrushes[index] == NULL)
        ThemeDarkBrushes[index] = HANDLES(CreateSolidBrush(ThemeSysColor(index)));
    if (ThemeDarkBrushes[index] == NULL) // out of GDI resources: never return NULL
        return GetSysColorBrush(index);
    return ThemeDarkBrushes[index];
}

void ReleaseThemeGraphics()
{
    int i;
    for (i = 0; i < THEME_SYSCOLOR_COUNT; i++)
    {
        if (ThemeDarkBrushes[i] != NULL)
        {
            HANDLES(DeleteObject(ThemeDarkBrushes[i]));
            ThemeDarkBrushes[i] = NULL;
        }
    }
}

//
// ****************************************************************************
// ThemeDrawEdge
//

BOOL ThemeDrawEdge(HDC hDC, RECT* rc, UINT edge, UINT flags)
{
    if (!IsDarkThemeActive())
        return DrawEdge(hDC, rc, edge, flags);

    // flat single-pixel bevel in the dark theme; raised edges get the light
    // color top/left, sunken edges get it bottom/right
    BOOL sunken = (edge & (BDR_SUNKENOUTER | BDR_SUNKENINNER)) != 0;
    COLORREF tl = sunken ? ThemeSysColor(COLOR_3DDKSHADOW) : ThemeSysColor(COLOR_3DLIGHT);
    COLORREF br = sunken ? ThemeSysColor(COLOR_3DLIGHT) : ThemeSysColor(COLOR_3DDKSHADOW);

    if (flags & BF_MIDDLE)
        FillRect(hDC, rc, ThemeSysColorBrush(COLOR_BTNFACE));

    RECT r = *rc;
    COLORREF oldBk = SetBkColor(hDC, tl);
    RECT line;
    if (flags & BF_TOP)
    {
        line = r;
        line.bottom = line.top + 1;
        ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &line, NULL, 0, NULL);
    }
    if (flags & BF_LEFT)
    {
        line = r;
        line.right = line.left + 1;
        ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &line, NULL, 0, NULL);
    }
    SetBkColor(hDC, br);
    if (flags & BF_BOTTOM)
    {
        line = r;
        line.top = line.bottom - 1;
        ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &line, NULL, 0, NULL);
    }
    if (flags & BF_RIGHT)
    {
        line = r;
        line.left = line.right - 1;
        ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &line, NULL, 0, NULL);
    }
    SetBkColor(hDC, oldBk);

    if (flags & BF_ADJUST)
    {
        if (flags & BF_LEFT)
            rc->left++;
        if (flags & BF_TOP)
            rc->top++;
        if (flags & BF_RIGHT)
            rc->right--;
        if (flags & BF_BOTTOM)
            rc->bottom--;
    }
    return TRUE;
}

//
// ****************************************************************************
// Palette repointing + theme switch
//

void UpdateCurrentColorsForTheme()
{
    if (IsDarkThemeActive())
    {
        CurrentColors = DarkColors;
        CurrentViewerColors = DarkViewerColors;
        // warm up the brushes other threads (viewer) may ask for, so the
        // lazy CreateSolidBrush always happens on the main thread
        ThemeSysColorBrush(COLOR_BTNFACE);
        ThemeSysColorBrush(COLOR_WINDOW);
    }
    else
    {
        CurrentColors = SchemeColors;
        CurrentViewerColors = ViewerColors;
    }
}

void ThemeApplyToTopLevel(HWND hWindow)
{
    if (hWindow == NULL)
        return;
    BOOL dark = IsDarkThemeActive();
    if (!dark && GetPropA(hWindow, THEME_DARKENED_PROP) == NULL)
        return; // never touched by us: keep the Default theme a pure no-op

    BOOL value = dark;
    DwmSetWindowAttribute(hWindow, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    if (dark)
        SetPropA(hWindow, THEME_DARKENED_PROP, (HANDLE)1);
    else
        RemovePropA(hWindow, THEME_DARKENED_PROP);
}

void ThemeUpdateWindowClassBackground(HWND hWindow, int lightSysColor)
{
    if (hWindow == NULL)
        return;
    if (IsDarkThemeActive())
        SetClassLongPtr(hWindow, GCLP_HBRBACKGROUND, (LONG_PTR)ThemeSysColorBrush(lightSysColor));
    else
        SetClassLongPtr(hWindow, GCLP_HBRBACKGROUND, (LONG_PTR)(HBRUSH)(UINT_PTR)(lightSysColor + 1));
}

void ThemeUpdateRebarStyle(HWND hRebar)
{
    if (hRebar == NULL)
        return;
    DWORD style = (DWORD)GetWindowLongPtr(hRebar, GWL_STYLE);
    DWORD newStyle;
    if (IsDarkThemeActive())
        newStyle = style & ~(WS_BORDER | RBS_BANDBORDERS);
    else
        newStyle = style | WS_BORDER | RBS_BANDBORDERS;
    if (newStyle != style)
    {
        SetWindowLongPtr(hRebar, GWL_STYLE, newStyle);
        SetWindowPos(hRebar, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        InvalidateRect(hRebar, NULL, TRUE);
    }
}

//
// ****************************************************************************
// Dialog theming
//

// Classic (unthemed) text controls render a DISABLED label with a two-pass
// etched emboss: COLOR_3DHILIGHT offset by 1px under COLOR_GRAYTEXT. On a
// dark background the near-white highlight makes the text look corroded.
// This subclass repaints the disabled label flat (theme gray on theme
// background); enabled controls and the Default theme pass through.
static LRESULT CALLBACK ThemeFlatDisabledTextSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                                          LPARAM lParam, UINT_PTR uIdSubclass,
                                                          DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        if (!IsDarkThemeActive() || IsWindowEnabled(hWnd))
            break; // normal drawing

        DWORD style = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
        DWORD type = style & SS_TYPEMASK;
        if (type != SS_LEFT && type != SS_CENTER && type != SS_RIGHT &&
            type != SS_SIMPLE && type != SS_LEFTNOWORDWRAP)
            break; // icons/bitmaps/frames/owner-draw keep their own drawing

        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hWnd, &ps);
        RECT r;
        GetClientRect(hWnd, &r);
        FillRect(hDC, &r, ThemeSysColorBrush(COLOR_BTNFACE));
        WCHAR text[512];
        int len = GetWindowTextW(hWnd, text, _countof(text));
        if (len > 0)
        {
            HFONT hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
            HFONT hOldFont = hFont != NULL ? (HFONT)SelectObject(hDC, hFont) : NULL;
            int oldBkMode = SetBkMode(hDC, TRANSPARENT);
            COLORREF oldClr = SetTextColor(hDC, ThemeSysColor(COLOR_GRAYTEXT));
            UINT dt = DT_EXPANDTABS;
            if (type == SS_CENTER)
                dt |= DT_CENTER;
            else if (type == SS_RIGHT)
                dt |= DT_RIGHT;
            if (type == SS_SIMPLE || type == SS_LEFTNOWORDWRAP)
                dt |= DT_SINGLELINE;
            else
                dt |= DT_WORDBREAK;
            if (style & SS_NOPREFIX)
                dt |= DT_NOPREFIX;
            if (style & SS_CENTERIMAGE)
                dt |= DT_VCENTER | DT_SINGLELINE;
            DrawTextW(hDC, text, len, &r, dt);
            SetTextColor(hDC, oldClr);
            SetBkMode(hDC, oldBkMode);
            if (hOldFont != NULL)
                SelectObject(hDC, hOldFont);
        }
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_NCDESTROY:
    {
        RemoveWindowSubclass(hWnd, ThemeFlatDisabledTextSubclassProc, 2);
        break;
    }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static BOOL CALLBACK ThemeApplyChildEnumProc(HWND hChild, LPARAM lParam)
{
    BOOL dark = (BOOL)lParam;
    char className[64];
    if (GetClassNameA(hChild, className, _countof(className)) == 0)
        return TRUE;

    if (_stricmp(className, "Button") == 0)
    {
        // the DarkMode_Explorer Button theme renders push buttons and
        // checkboxes with light label text, but radio buttons and group
        // boxes keep the theme's black text (unreadable on the dark
        // background) - strip those to classic drawing, which honors the
        // WM_CTLCOLORSTATIC text color from ThemeHandleCtlColor
        DWORD type = (DWORD)GetWindowLongPtr(hChild, GWL_STYLE) & BS_TYPEMASK;
        if (type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON || type == BS_GROUPBOX)
            SetWindowTheme(hChild, dark ? L"" : NULL, dark ? L"" : NULL);
        else
            SetWindowTheme(hChild, dark ? L"DarkMode_Explorer" : NULL, NULL);
    }
    else if (_stricmp(className, "Edit") == 0 ||
             _stricmp(className, "ListBox") == 0 ||
             _stricmp(className, "ScrollBar") == 0)
    {
        SetWindowTheme(hChild, dark ? L"DarkMode_Explorer" : NULL, NULL);
    }
    else if (_stricmp(className, "Static") == 0)
    {
        // flat repaint of disabled labels (see the subclass above); the
        // subclass is inert while the control is enabled or Default is active
        if (dark)
            SetWindowSubclass(hChild, ThemeFlatDisabledTextSubclassProc, 2, 0);
        else
            RemoveWindowSubclass(hChild, ThemeFlatDisabledTextSubclassProc, 2);
    }
    else if (_stricmp(className, "ComboBox") == 0 ||
             _stricmp(className, "ComboBoxEx32") == 0)
    {
        SetWindowTheme(hChild, dark ? L"DarkMode_CFD" : NULL, NULL);
    }
    else if (_stricmp(className, WC_LISTVIEWA) == 0)
    {
        SetWindowTheme(hChild, dark ? L"DarkMode_Explorer" : NULL, NULL);
        COLORREF bk = dark ? ThemeSysColor(COLOR_WINDOW) : GetSysColor(COLOR_WINDOW);
        COLORREF tx = dark ? ThemeSysColor(COLOR_WINDOWTEXT) : GetSysColor(COLOR_WINDOWTEXT);
        ListView_SetBkColor(hChild, bk);
        ListView_SetTextBkColor(hChild, bk);
        ListView_SetTextColor(hChild, tx);
        HWND hHeader = ListView_GetHeader(hChild);
        if (hHeader != NULL)
            SetWindowTheme(hHeader, dark ? L"DarkMode_ItemsView" : NULL, NULL);
    }
    else if (_stricmp(className, WC_TREEVIEWA) == 0)
    {
        SetWindowTheme(hChild, dark ? L"DarkMode_Explorer" : L"Explorer", NULL);
        TreeView_SetBkColor(hChild, dark ? ThemeSysColor(COLOR_WINDOW) : (COLORREF)-1);
        TreeView_SetTextColor(hChild, dark ? ThemeSysColor(COLOR_WINDOWTEXT) : (COLORREF)-1);
    }
    else if (_stricmp(className, WC_HEADERA) == 0)
    {
        SetWindowTheme(hChild, dark ? L"DarkMode_ItemsView" : NULL, NULL);
    }
    else
    {
        // custom windows with standard (non-client) scrollbars - panels,
        // viewers, edit-list boxes: the DarkMode_Explorer theme darkens the
        // scrollbars without touching the self-painted client area
        DWORD style = (DWORD)GetWindowLongPtr(hChild, GWL_STYLE);
        if (style & (WS_VSCROLL | WS_HSCROLL))
            SetWindowTheme(hChild, dark ? L"DarkMode_Explorer" : NULL, NULL);
    }
    InvalidateRect(hChild, NULL, TRUE);
    return TRUE;
}

void ThemeApplyToDialog(HWND hDialog)
{
    if (hDialog == NULL)
        return;
    BOOL dark = IsDarkThemeActive();
    if (!dark && GetPropA(hDialog, THEME_DARKENED_PROP) == NULL)
        return; // Default theme + never darkened: strict no-op (passthrough)

    ThemeApplyToTopLevel(hDialog);
    EnumChildWindows(hDialog, ThemeApplyChildEnumProc, (LPARAM)dark);
    if (dark)
        SetPropA(hDialog, THEME_DARKENED_PROP, (HANDLE)1);
    else
        RemovePropA(hDialog, THEME_DARKENED_PROP);
    InvalidateRect(hDialog, NULL, TRUE);
}

BOOL ThemeHandleCtlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, INT_PTR* result)
{
    if (!IsDarkThemeActive())
        return FALSE;

    HDC hDC = (HDC)wParam;
    switch (uMsg)
    {
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    {
        SetTextColor(hDC, ThemeSysColor(COLOR_BTNTEXT));
        SetBkColor(hDC, ThemeSysColor(COLOR_BTNFACE));
        *result = (INT_PTR)ThemeSysColorBrush(COLOR_BTNFACE);
        return TRUE;
    }

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    {
        SetTextColor(hDC, ThemeSysColor(COLOR_WINDOWTEXT));
        SetBkColor(hDC, ThemeSysColor(COLOR_WINDOW));
        *result = (INT_PTR)ThemeSysColorBrush(COLOR_WINDOW);
        return TRUE;
    }

    case WM_CTLCOLORSCROLLBAR:
    {
        *result = (INT_PTR)ThemeSysColorBrush(COLOR_BTNFACE);
        return TRUE;
    }
    }
    return FALSE;
}

//
// ****************************************************************************
// ThemeAdjustBitmapForDarkMode
//

void ThemeAdjustBitmapForDarkMode(HBITMAP hBitmap, COLORREF transparent)
{
    if (!IsDarkThemeActive() || hBitmap == NULL)
        return;

    BITMAP bmp;
    if (GetObject(hBitmap, sizeof(bmp), &bmp) == 0 || bmp.bmWidth <= 0 || bmp.bmHeight <= 0)
        return;

    int width = bmp.bmWidth;
    int height = bmp.bmHeight;
    DWORD* pixels = (DWORD*)malloc((size_t)width * height * sizeof(DWORD));
    if (pixels == NULL)
        return;

    BITMAPINFO bi;
    memset(&bi, 0, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = -height; // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    HDC hDC = HANDLES(GetDC(NULL));
    if (GetDIBits(hDC, hBitmap, 0, height, pixels, &bi, DIB_RGB_COLORS) == height)
    {
        int count = width * height;
        int i;

        // legacy 24bpp sources come back with a zero alpha byte everywhere;
        // PNG (alpha) sources have at least one non-zero alpha byte
        BOOL hasAlpha = FALSE;
        for (i = 0; i < count; i++)
        {
            if ((pixels[i] & 0xFF000000) != 0)
            {
                hasAlpha = TRUE;
                break;
            }
        }

        for (i = 0; i < count; i++)
        {
            DWORD px = pixels[i];
            int a = (int)(px >> 24);
            if (hasAlpha && a == 0)
                continue; // fully transparent
            int r = (int)((px >> 16) & 0xFF);
            int g = (int)((px >> 8) & 0xFF);
            int b = (int)(px & 0xFF);
            if (!hasAlpha && RGB(r, g, b) == transparent)
                continue; // mask key color must survive untouched

            // un-premultiply partially transparent PNG pixels
            if (hasAlpha && a > 0 && a < 255)
            {
                r = min(255, MulDiv(r, 255, a));
                g = min(255, MulDiv(g, 255, a));
                b = min(255, MulDiv(b, 255, a));
            }

            // feature 029: shared per-color rules (also applied to SVG toolbar
            // glyphs in svg.cpp and unit-tested from saltests)
            ThemeDarkAdaptColor(&r, &g, &b);

            if (hasAlpha && a > 0 && a < 255)
            {
                r = MulDiv(r, a, 255);
                g = MulDiv(g, a, 255);
                b = MulDiv(b, a, 255);
            }
            pixels[i] = ((DWORD)a << 24) | ((DWORD)r << 16) | ((DWORD)g << 8) | (DWORD)b;
        }
        SetDIBits(hDC, hBitmap, 0, height, pixels, &bi, DIB_RGB_COLORS);
    }
    HANDLES(ReleaseDC(NULL, hDC));
    free(pixels);
}

//
// ****************************************************************************
// Property-sheet frame subclass
//

static LRESULT CALLBACK ThemePropSheetFrameSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                                        LPARAM lParam, UINT_PTR uIdSubclass,
                                                        DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORSCROLLBAR:
    {
        INT_PTR result;
        if (ThemeHandleCtlColor(uMsg, wParam, lParam, &result))
            return result;
        break;
    }

    case WM_ERASEBKGND:
    {
        if (IsDarkThemeActive())
        {
            RECT r;
            GetClientRect(hWnd, &r);
            FillRect((HDC)wParam, &r, ThemeSysColorBrush(COLOR_BTNFACE));
            return TRUE;
        }
        break;
    }

    case WM_NCDESTROY:
    {
        RemoveWindowSubclass(hWnd, ThemePropSheetFrameSubclassProc, 1);
        break;
    }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void ThemeSubclassPropSheetFrame(HWND hFrame)
{
    if (hFrame == NULL)
        return;
    if (!IsDarkThemeActive() && GetPropA(hFrame, THEME_DARKENED_PROP) == NULL)
        return;                                                       // strict Default-theme passthrough
    SetWindowSubclass(hFrame, ThemePropSheetFrameSubclassProc, 1, 0); // idempotent
    ThemeApplyToDialog(hFrame);                                       // DWM title bar + tree/buttons/tab children
}

//
// ****************************************************************************
// SetThemeMode
//

void SetThemeMode(DWORD mode)
{
    CALL_STACK_MESSAGE2("SetThemeMode(%u)", mode);
    if (mode != THEME_MODE_DARK)
        mode = THEME_MODE_DEFAULT; // forward compatibility: unknown -> Default
    if ((DWORD)Configuration.ThemeMode == mode)
        return;

    Configuration.ThemeMode = mode;
    RefreshThemeHighContrastState();
    UpdateCurrentColorsForTheme();

    if (MainWindow != NULL && MainWindow->HWindow != NULL)
        ThemeUpdateWindowClassBackground(MainWindow->HWindow, COLOR_WINDOW);

    // the standard color-change pipeline: rebuilds brushes/pens/toolbar
    // bitmaps and notifies panels, Find windows, viewers and plugins
    ColorsChanged(TRUE, FALSE, TRUE);

    if (MainWindow != NULL && MainWindow->HWindow != NULL)
    {
        // rebar band grippers are classic-drawn (light-only): re-insert the
        // bands so the gripper style matches the new theme
        if (MainWindow->HTopRebar != NULL)
            MainWindow->RebuildRebarBands();
        // DWM title bar + per-child theming (scrollbars, combos, ...)
        ThemeApplyToDialog(MainWindow->HWindow);
        // let DWM redraw the frame with the new caption color
        SetWindowPos(MainWindow->HWindow, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        RedrawWindow(MainWindow->HWindow, NULL, NULL,
                     RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }
}
