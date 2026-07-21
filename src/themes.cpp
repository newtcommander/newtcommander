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

void RefreshThemeHighContrastState()
{
    HIGHCONTRAST hc;
    memset(&hc, 0, sizeof(hc));
    hc.cbSize = sizeof(hc);
    ThemeHighContrast = SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0) &&
                        (hc.dwFlags & HCF_HIGHCONTRASTON) != 0;
    ThemeHighContrastValid = TRUE;
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

    ThemeDarkSysColors[COLOR_SCROLLBAR] = RGB(45, 45, 45);
    ThemeDarkSysColors[COLOR_ACTIVECAPTION] = RGB(38, 79, 120);
    ThemeDarkSysColors[COLOR_INACTIVECAPTION] = RGB(45, 45, 45);
    ThemeDarkSysColors[COLOR_MENU] = RGB(45, 45, 45);
    ThemeDarkSysColors[COLOR_WINDOW] = RGB(32, 32, 32);
    ThemeDarkSysColors[COLOR_WINDOWFRAME] = RGB(85, 85, 85);
    ThemeDarkSysColors[COLOR_MENUTEXT] = RGB(240, 240, 240);
    ThemeDarkSysColors[COLOR_WINDOWTEXT] = RGB(240, 240, 240);
    ThemeDarkSysColors[COLOR_CAPTIONTEXT] = RGB(255, 255, 255);
    ThemeDarkSysColors[COLOR_APPWORKSPACE] = RGB(38, 38, 38);
    ThemeDarkSysColors[COLOR_HIGHLIGHT] = RGB(38, 79, 120);
    ThemeDarkSysColors[COLOR_HIGHLIGHTTEXT] = RGB(255, 255, 255);
    ThemeDarkSysColors[COLOR_BTNFACE] = RGB(45, 45, 45);
    ThemeDarkSysColors[COLOR_BTNSHADOW] = RGB(26, 26, 26);
    ThemeDarkSysColors[COLOR_GRAYTEXT] = RGB(150, 150, 150);
    ThemeDarkSysColors[COLOR_BTNTEXT] = RGB(240, 240, 240);
    ThemeDarkSysColors[COLOR_INACTIVECAPTIONTEXT] = RGB(170, 170, 170);
    ThemeDarkSysColors[COLOR_BTNHIGHLIGHT] = RGB(70, 70, 70);
    ThemeDarkSysColors[COLOR_3DDKSHADOW] = RGB(16, 16, 16);
    ThemeDarkSysColors[COLOR_3DLIGHT] = RGB(58, 58, 58);
    ThemeDarkSysColors[COLOR_INFOTEXT] = RGB(240, 240, 240);
    ThemeDarkSysColors[COLOR_INFOBK] = RGB(50, 50, 50);
    ThemeDarkSysColors[COLOR_HOTLIGHT] = RGB(102, 178, 255);
    ThemeDarkSysColors[COLOR_GRADIENTACTIVECAPTION] = RGB(38, 79, 120);
    ThemeDarkSysColors[COLOR_GRADIENTINACTIVECAPTION] = RGB(45, 45, 45);
    ThemeDarkSysColors[COLOR_MENUHILIGHT] = RGB(38, 79, 120);
    ThemeDarkSysColors[COLOR_MENUBAR] = RGB(45, 45, 45);

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

//
// ****************************************************************************
// Dialog theming
//

static BOOL CALLBACK ThemeApplyChildEnumProc(HWND hChild, LPARAM lParam)
{
    BOOL dark = (BOOL)lParam;
    char className[64];
    if (GetClassNameA(hChild, className, _countof(className)) == 0)
        return TRUE;

    if (_stricmp(className, "Button") == 0 ||
        _stricmp(className, "Edit") == 0 ||
        _stricmp(className, "ListBox") == 0 ||
        _stricmp(className, "ScrollBar") == 0)
    {
        SetWindowTheme(hChild, dark ? L"DarkMode_Explorer" : NULL, NULL);
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
        ThemeApplyToTopLevel(MainWindow->HWindow);
        // let DWM redraw the frame with the new caption color
        SetWindowPos(MainWindow->HWindow, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        RedrawWindow(MainWindow->HWindow, NULL, NULL,
                     RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }
}
