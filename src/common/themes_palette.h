// SPDX-FileCopyrightText: 2026 Pavel Stupka
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// Feature 028: single source of truth for the Dark theme palettes.
// Pure data - needs only <windows.h> (COLOR_* indexes). Consumed by:
//   - src/themes.cpp    dark chrome palette (ThemeSysColor LUT)
//   - src/salamdr1.cpp  DarkColors / DarkViewerColors panel arrays
//   - src/saltests      coverage + WCAG contrast unit tests
//
// Design targets (specs/028-visual-themes, SC-005): standard text >= 4.5:1
// contrast against its background, disabled/secondary text >= 3:1.
// Feature 049: input/content surfaces (COLOR_WINDOW) sit LIGHTER than the
// dialog face (COLOR_BTNFACE) - the Windows 11 dark convention; saltests
// enforces the ordering.

// ---------------------------------------------------------------------------
// Dark chrome palette: ENTRY(sysColorIndex, r, g, b)
// Replaces GetSysColor results at draw time when the Dark theme is active.

#define THEME_DARK_SYSCOLORS(ENTRY) \
    ENTRY(COLOR_SCROLLBAR, 45, 45, 45) \
    ENTRY(COLOR_ACTIVECAPTION, 38, 79, 120) \
    ENTRY(COLOR_INACTIVECAPTION, 45, 45, 45) \
    ENTRY(COLOR_MENU, 45, 45, 45) \
    ENTRY(COLOR_WINDOW, 56, 56, 56) \
    ENTRY(COLOR_WINDOWFRAME, 85, 85, 85) \
    ENTRY(COLOR_MENUTEXT, 240, 240, 240) \
    ENTRY(COLOR_WINDOWTEXT, 240, 240, 240) \
    ENTRY(COLOR_CAPTIONTEXT, 255, 255, 255) \
    ENTRY(COLOR_APPWORKSPACE, 38, 38, 38) \
    ENTRY(COLOR_HIGHLIGHT, 38, 79, 120) \
    ENTRY(COLOR_HIGHLIGHTTEXT, 255, 255, 255) \
    ENTRY(COLOR_BTNFACE, 45, 45, 45) \
    ENTRY(COLOR_BTNSHADOW, 26, 26, 26) \
    ENTRY(COLOR_GRAYTEXT, 150, 150, 150) \
    ENTRY(COLOR_BTNTEXT, 240, 240, 240) \
    ENTRY(COLOR_INACTIVECAPTIONTEXT, 170, 170, 170) \
    ENTRY(COLOR_BTNHIGHLIGHT, 70, 70, 70) \
    ENTRY(COLOR_3DDKSHADOW, 16, 16, 16) \
    ENTRY(COLOR_3DLIGHT, 58, 58, 58) \
    ENTRY(COLOR_INFOTEXT, 240, 240, 240) \
    ENTRY(COLOR_INFOBK, 50, 50, 50) \
    ENTRY(COLOR_HOTLIGHT, 102, 178, 255) \
    ENTRY(COLOR_GRADIENTACTIVECAPTION, 38, 79, 120) \
    ENTRY(COLOR_GRADIENTINACTIVECAPTION, 45, 45, 45) \
    ENTRY(COLOR_MENUHILIGHT, 38, 79, 120) \
    ENTRY(COLOR_MENUBAR, 45, 45, 45)

// ---------------------------------------------------------------------------
// Dark panel scheme: ENTRY(indexName, r, g, b)
// ORDER MUST MATCH the CurrentColors index defines in src/consts.h
// (FOCUS_ACTIVE_NORMAL = 0 ... THUMBNAIL_FRAME_FOCSEL = 33); salamdr1.cpp
// verifies the order with a static_assert. All entries are explicit RGB with
// flag 0 (never SCF_DEFAULT - UpdateDefaultColors must not rewrite them).

#define THEME_DARK_PANEL_COLORS(ENTRY) \
    ENTRY(FOCUS_ACTIVE_NORMAL, 240, 240, 240) \
    ENTRY(FOCUS_ACTIVE_SELECTED, 255, 160, 160) \
    ENTRY(FOCUS_FG_INACTIVE_NORMAL, 128, 128, 128) \
    ENTRY(FOCUS_FG_INACTIVE_SELECTED, 200, 120, 120) \
    ENTRY(FOCUS_BK_INACTIVE_NORMAL, 32, 32, 32) \
    ENTRY(FOCUS_BK_INACTIVE_SELECTED, 32, 32, 32) \
    ENTRY(ITEM_FG_NORMAL, 240, 240, 240) \
    ENTRY(ITEM_FG_SELECTED, 255, 110, 110) \
    ENTRY(ITEM_FG_FOCUSED, 255, 255, 255) \
    ENTRY(ITEM_FG_FOCSEL, 255, 128, 128) \
    ENTRY(ITEM_FG_HIGHLIGHT, 240, 240, 240) \
    ENTRY(ITEM_BK_NORMAL, 32, 32, 32) \
    ENTRY(ITEM_BK_SELECTED, 32, 32, 32) \
    ENTRY(ITEM_BK_FOCUSED, 58, 58, 58) \
    ENTRY(ITEM_BK_FOCSEL, 58, 58, 58) \
    ENTRY(ITEM_BK_HIGHLIGHT, 48, 48, 48) \
    ENTRY(ICON_BLEND_SELECTED, 255, 128, 128) \
    ENTRY(ICON_BLEND_FOCUSED, 128, 128, 128) \
    ENTRY(ICON_BLEND_FOCSEL, 255, 96, 96) \
    ENTRY(PROGRESS_FG_NORMAL, 130, 180, 255) \
    ENTRY(PROGRESS_FG_SELECTED, 255, 255, 255) \
    ENTRY(PROGRESS_BK_NORMAL, 32, 32, 32) \
    ENTRY(PROGRESS_BK_SELECTED, 38, 79, 120) \
    ENTRY(HOT_PANEL, 102, 178, 255) \
    ENTRY(HOT_ACTIVE, 180, 210, 255) \
    ENTRY(HOT_INACTIVE, 160, 190, 230) \
    ENTRY(ACTIVE_CAPTION_FG, 255, 255, 255) \
    ENTRY(ACTIVE_CAPTION_BK, 38, 79, 120) \
    ENTRY(INACTIVE_CAPTION_FG, 170, 170, 170) \
    ENTRY(INACTIVE_CAPTION_BK, 45, 45, 45) \
    ENTRY(THUMBNAIL_FRAME_NORMAL, 96, 96, 96) \
    ENTRY(THUMBNAIL_FRAME_FOCUSED, 240, 240, 240) \
    ENTRY(THUMBNAIL_FRAME_SELECTED, 255, 110, 110) \
    ENTRY(THUMBNAIL_FRAME_FOCSEL, 200, 80, 80)

// Dark viewer scheme: ENTRY(indexName, r, g, b); order = VIEWER_FG_NORMAL,
// VIEWER_BK_NORMAL, VIEWER_FG_SELECTED, VIEWER_BK_SELECTED (src/consts.h)
#define THEME_DARK_VIEWER_COLORS(ENTRY) \
    ENTRY(VIEWER_FG_NORMAL, 220, 220, 220) \
    ENTRY(VIEWER_BK_NORMAL, 30, 30, 30) \
    ENTRY(VIEWER_FG_SELECTED, 255, 255, 255) \
    ENTRY(VIEWER_BK_SELECTED, 38, 79, 120)

// ---------------------------------------------------------------------------
// Feature 029: dark adaptation of a single light-surface glyph color.
// Pure math shared by ThemeAdjustBitmapForDarkMode (src/themes.cpp, legacy
// raster glyphs) and the SVG toolbar glyph recolor (src/svg.cpp), and
// unit-tested from src/saltests. Rules (identical to the 028 bitmap
// transform, MulDiv-style round-to-nearest arithmetic):
//   - neutral pixel (max-min < 32) darker than 140: invert its darkness so
//     black outlines become the lightest; [0,140) maps monotonically onto
//     (140,220]
//   - saturated color with max channel < 120: brighten toward the same hue
//     (max channel scales to 170)
//   - anything already light/bright is left unchanged

inline int ThemeDarkAdaptMulDivRound(int n, int num, int den)
{
    return (n * num + den / 2) / den;
}

inline void ThemeDarkAdaptColor(int* r, int* g, int* b)
{
    int maxc = *r > *g ? (*r > *b ? *r : *b) : (*g > *b ? *g : *b);
    int minc = *r < *g ? (*r < *b ? *r : *b) : (*g < *b ? *g : *b);
    if (maxc - minc < 32)
    {
        if (maxc < 140)
        {
            int v = 220 - ThemeDarkAdaptMulDivRound(maxc, 80, 140);
            *r = *g = *b = v;
        }
    }
    else if (maxc < 120)
    {
        // maxc >= 32 here (maxc - minc >= 32), no division by zero
        int c = ThemeDarkAdaptMulDivRound(*r, 170, maxc);
        *r = c > 255 ? 255 : c;
        c = ThemeDarkAdaptMulDivRound(*g, 170, maxc);
        *g = c > 255 ? 255 : c;
        c = ThemeDarkAdaptMulDivRound(*b, 170, maxc);
        *b = c > 255 ? 255 : c;
    }
}
