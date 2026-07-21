// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

// Unit tests for the 004-long-paths-unicode foundation helpers
// (src/common/salunicode.cpp, src/common/salpath.cpp).
// Console exe; exit code = number of failed checks.

#include "precomp.h"

#include <math.h>

#include "salunicode.h"
#include "salpath.h"
#include "salfileio.h"
#include "salclip.h"
#include "themes_palette.h"

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond) \
    do \
    { \
        g_checks++; \
        if (!(cond)) \
        { \
            g_failures++; \
            printf("FAIL %s(%d): %s\n", __FILE__, __LINE__, #cond); \
        } \
    } while (0)

// UTF-8 byte sequences used below:
//   NFC c-caron (U+010D)          = C4 8D
//   NFD c + combining caron       = 63 CC 8C
//   NFC C-caron (U+010C)          = C4 8C
//   folder emoji (U+1F4C1)        = F0 9F 93 81
#define U8_C_CARON_NFC "\xC4\x8D"
#define U8_C_CARON_NFD "c\xCC\x8C"
#define U8_CAP_C_CARON_NFC "\xC4\x8C"
#define U8_FOLDER_EMOJI "\xF0\x9F\x93\x81"

static void TestConversions()
{
    // NFC round trip
    WCHAR* w = SalU8ToWAlloc(U8_C_CARON_NFC);
    CHECK(w != NULL && wcscmp(w, L"\x010D") == 0);
    free(w);

    // NFD is preserved exactly (no silent normalization)
    w = SalU8ToWAlloc(U8_C_CARON_NFD);
    CHECK(w != NULL && wcscmp(w, L"c\x030C") == 0);
    char* u8 = SalWToU8Alloc(w);
    CHECK(u8 != NULL && strcmp(u8, U8_C_CARON_NFD) == 0);
    free(u8);
    free(w);

    // non-BMP round trip (surrogate pair)
    w = SalU8ToWAlloc(U8_FOLDER_EMOJI);
    CHECK(w != NULL && wcscmp(w, L"\xD83D\xDCC1") == 0);
    u8 = SalWToU8Alloc(w);
    CHECK(u8 != NULL && strcmp(u8, U8_FOLDER_EMOJI) == 0);
    free(u8);
    free(w);

    // invalid UTF-8 fails instead of being replaced
    CHECK(SalU8ToWAlloc("\xC4") == NULL);
    CHECK(SalU8ToWAlloc("\xFF\xFE") == NULL);

    // unpaired surrogate is not representable (documented limitation)
    CHECK(SalWToU8Alloc(L"\xD83D") == NULL);

    // sized (non-null-terminated) inputs get terminated output
    WCHAR wbuf[8];
    CHECK(SalU8ToW("abcdef", 3, wbuf, 8) == 4 && wcscmp(wbuf, L"abc") == 0);
    char cbuf[8];
    CHECK(SalWToU8(L"abcdef", 3, cbuf, 8) == 4 && strcmp(cbuf, "abc") == 0);
    // exact-fit failure is detected (no silent truncation)
    CHECK(SalU8ToW("abcd", 4, wbuf, 4) == 0);

    // lossless ACP conversion: ASCII passes, emoji cannot
    char acp[16];
    CHECK(SalWToACPLossless(L"abc", -1, acp, sizeof(acp)) && strcmp(acp, "abc") == 0);
    CHECK(!SalWToACPLossless(L"\xD83D\xDCC1", -1, acp, sizeof(acp)));
}

static void TestNormalization()
{
    // NFD -> NFC composition
    WCHAR buf[8];
    CHECK(SalNormalizeNFC(L"c\x030C", -1, buf, 8) > 0 && wcscmp(buf, L"\x010D") == 0);
    // NFC input is idempotent
    CHECK(SalNormalizeNFC(L"\x010D", -1, buf, 8) > 0 && wcscmp(buf, L"\x010D") == 0);
    // ASCII passthrough
    CHECK(SalNormalizeNFC(L"abc", -1, buf, 8) > 0 && wcscmp(buf, L"abc") == 0);
    WCHAR* nfc = SalNormalizeNFCAlloc(L"c\x030C"
                                      L".txt");
    CHECK(nfc != NULL && wcscmp(nfc, L"\x010D.txt") == 0);
    free(nfc);
}

static void TestMatching()
{
    CHECK(SalIsASCII("plain.txt"));
    CHECK(!SalIsASCII(U8_C_CARON_NFC ".txt"));

    // canonical equivalence (case-sensitive)
    CHECK(SalNameEquivalent(U8_C_CARON_NFC ".txt", U8_C_CARON_NFD ".txt"));
    CHECK(SalNameEquivalent("same.txt", "same.txt"));
    CHECK(!SalNameEquivalent("a.txt", "b.txt"));
    CHECK(!SalNameEquivalent(U8_CAP_C_CARON_NFC ".txt", U8_C_CARON_NFD ".txt")); // differs in case

    // case-insensitive, form-insensitive equality (FR-008)
    CHECK(SalNameEqualCI(U8_CAP_C_CARON_NFC ".TXT", -1, U8_C_CARON_NFD ".txt", -1));
    CHECK(SalNameEqualCI("ABC", -1, "abc", -1));
    CHECK(!SalNameEqualCI("abc", -1, "abd", -1));
    CHECK(SalNameEqualCI("abc", 2, "ab", -1)); // explicit lengths

    // collation: equivalent forms compare equal, order is sign-correct
    CHECK(SalCompareNamesUTF8(U8_C_CARON_NFC, -1, U8_C_CARON_NFD, -1, FALSE) == 0);
    CHECK(SalCompareNamesUTF8("a", -1, "b", -1, FALSE) < 0);
    CHECK(SalCompareNamesUTF8("b", -1, "a", -1, FALSE) > 0);
    CHECK(SalCompareNamesUTF8("A", -1, "a", -1, TRUE) == 0);
}

static void TestPathBuf()
{
    CSalPathBuf p;
    CHECK(p.IsEmpty() && p.Length() == 0 && strcmp(p.Get(), "") == 0);

    CHECK(p.Set("C:\\dir"));
    CHECK(p.AppendComponent("sub"));
    CHECK(strcmp(p.Get(), "C:\\dir\\sub") == 0);
    CHECK(p.AppendComponent("\\slashed")); // leading separators are eaten
    CHECK(strcmp(p.Get(), "C:\\dir\\sub\\slashed") == 0);

    CHECK(p.AddBackslash() && p.AddBackslash()); // idempotent
    CHECK(strcmp(p.Get(), "C:\\dir\\sub\\slashed\\") == 0);
    p.StripBackslash();
    CHECK(strcmp(p.Get(), "C:\\dir\\sub\\slashed") == 0);

    CHECK(p.CutLastComponent() && strcmp(p.Get(), "C:\\dir\\sub") == 0);
    CHECK(p.CutLastComponent() && strcmp(p.Get(), "C:\\dir") == 0);
    CHECK(p.CutLastComponent() && strcmp(p.Get(), "C:\\") == 0);
    CHECK(!p.CutLastComponent()); // at root
    p.StripBackslash();
    CHECK(strcmp(p.Get(), "C:\\") == 0); // drive root keeps its backslash

    // UNC root protection
    CHECK(p.Set("\\\\server\\share\\dir"));
    CHECK(p.CutLastComponent() && strcmp(p.Get(), "\\\\server\\share") == 0);
    CHECK(!p.CutLastComponent()); // share is part of the root

    // growth far beyond MAX_PATH
    CHECK(p.Set("C:\\"));
    for (int i = 0; i < 200; i++)
        CHECK(p.AppendComponent("component"));
    CHECK(p.Length() > 2000);
    CHECK(p.Get()[p.Length()] == 0);

    // copy semantics
    CSalPathBuf q(p);
    CHECK(q.Length() == p.Length() && strcmp(q.Get(), p.Get()) == 0);
    CSalPathBuf r;
    r = p;
    CHECK(r.Length() == p.Length() && strcmp(r.Get(), p.Get()) == 0);
}

static void TestExtendedPaths()
{
    WCHAR* w = SalPathToWExtAlloc("C:\\dir\\file.txt");
    CHECK(w != NULL && wcscmp(w, L"\\\\?\\C:\\dir\\file.txt") == 0);
    free(w);

    // dot segments collapse, forward slashes convert
    w = SalPathToWExtAlloc("C:\\a\\b\\..\\c\\.\\d");
    CHECK(w != NULL && wcscmp(w, L"\\\\?\\C:\\a\\c\\d") == 0);
    free(w);
    w = SalPathToWExtAlloc("C:/fwd/slash");
    CHECK(w != NULL && wcscmp(w, L"\\\\?\\C:\\fwd\\slash") == 0);
    free(w);

    // UNC form
    w = SalPathToWExtAlloc("\\\\server\\share\\file");
    CHECK(w != NULL && wcscmp(w, L"\\\\?\\UNC\\server\\share\\file") == 0);
    free(w);

    // drive root
    w = SalPathToWExtAlloc("C:\\");
    CHECK(w != NULL && wcscmp(w, L"\\\\?\\C:\\") == 0);
    free(w);

    // Unicode content flows through
    w = SalPathToWExtAlloc("C:\\" U8_C_CARON_NFD "\\" U8_FOLDER_EMOJI ".txt");
    CHECK(w != NULL && wcscmp(w, L"\\\\?\\C:\\c\x030C\\\xD83D\xDCC1.txt") == 0);
    free(w);

    // climbing above the root fails
    CHECK(SalPathToWExtAlloc("C:\\a\\..\\..") == NULL);

    // feature 027 pre-scan: clean paths (skip branch) and the dirty forms it
    // must still route through canonicalization produce identical output
    w = SalPathToWExtAlloc("C:\\already\\clean\\path"); // clean -> skip branch
    CHECK(w != NULL && wcscmp(w, L"\\\\?\\C:\\already\\clean\\path") == 0);
    free(w);
    w = SalPathToWExtAlloc("C:\\trailing\\"); // trailing separator must be stripped
    CHECK(w != NULL && wcscmp(w, L"\\\\?\\C:\\trailing") == 0);
    free(w);
    w = SalPathToWExtAlloc("C:\\double\\\\sep"); // doubled separator must collapse
    CHECK(w != NULL && wcscmp(w, L"\\\\?\\C:\\double\\sep") == 0);
    free(w);
    w = SalPathToWExtAlloc("C:\\a\\.\\b"); // single-dot component must drop
    CHECK(w != NULL && wcscmp(w, L"\\\\?\\C:\\a\\b") == 0);
    free(w);
    w = SalPathToWExtAlloc("C:\\dotted.name\\file..ext"); // dots inside names are NOT components -> clean
    CHECK(w != NULL && wcscmp(w, L"\\\\?\\C:\\dotted.name\\file..ext") == 0);
    free(w);

    // already-extended input passes through
    w = SalPathToWExtAlloc("\\\\?\\C:\\x");
    CHECK(w != NULL && wcscmp(w, L"\\\\?\\C:\\x") == 0);
    free(w);

    // a long (>260) path is accepted, an absurd one (>32767) is rejected
    CSalPathBuf lp;
    CHECK(lp.Set("C:\\"));
    for (int i = 0; i < 60; i++)
        CHECK(lp.AppendComponent("component-eighteen"));
    CHECK(lp.Length() > 1000);
    w = SalPathToWExtAlloc(lp.Get());
    CHECK(w != NULL && wcsncmp(w, L"\\\\?\\C:\\", 7) == 0 && wcslen(w) > 1000);
    free(w);
    for (int i = 0; i < 1800; i++)
        lp.AppendComponent("component-eighteen");
    CHECK(SalPathToWExtAlloc(lp.Get()) == NULL);

    // relative input resolves against the current directory
    w = SalPathToWExtAlloc("relative.txt");
    CHECK(w != NULL && wcsncmp(w, L"\\\\?\\", 4) == 0 && wcsstr(w, L"relative.txt") != NULL);
    free(w);

    // display-form round trip strips the prefix
    char* u8 = SalPathFromWAlloc(L"\\\\?\\C:\\dir\\x");
    CHECK(u8 != NULL && strcmp(u8, "C:\\dir\\x") == 0);
    free(u8);
    u8 = SalPathFromWAlloc(L"\\\\?\\UNC\\server\\share\\x");
    CHECK(u8 != NULL && strcmp(u8, "\\\\server\\share\\x") == 0);
    free(u8);
}

// end-to-end: create, enumerate, rename and delete files at a path
// deeper than the legacy 260-char limit and with an NFD Unicode name
static void TestFileIO()
{
    char tmp[MAX_PATH];
    DWORD n = GetTempPathA(sizeof(tmp), tmp);
    if (n == 0 || n >= sizeof(tmp))
    {
        printf("skipping TestFileIO (no temp path)\n");
        return;
    }

    CSalPathBuf base;
    CHECK(base.Set(tmp));
    CHECK(base.AppendComponent("saltests-deep"));
    CHECK(SalCreateDirectory(base.Get(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS);
    CSalPathBuf dir(base);
    while (dir.Length() < 300) // push well past MAX_PATH
    {
        CHECK(dir.AppendComponent("component-eighteen"));
        CHECK(SalCreateDirectory(dir.Get(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS);
    }
    CHECK(dir.Length() > 300);

    // file with an NFD name at the deep path
    CSalPathBuf file(dir);
    CHECK(file.AppendComponent(U8_C_CARON_NFD "-deep.txt"));
    HANDLE h = SalCreateFile(file.Get(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL, NULL);
    CHECK(h != INVALID_HANDLE_VALUE);
    if (h != INVALID_HANDLE_VALUE)
    {
        DWORD written;
        CHECK(WriteFile(h, "data", 4, &written, NULL) && written == 4);
        CloseHandle(h);
    }

    // attributes work at depth
    CHECK(SalGetFileAttributes(file.Get()) != INVALID_FILE_ATTRIBUTES);
    WIN32_FILE_ATTRIBUTE_DATA fad;
    CHECK(SalGetFileAttributesEx(file.Get(), &fad) && fad.nFileSizeLow == 4);

    // enumeration returns the exact NFD name (no normalization)
    CSalPathBuf pattern(dir);
    CHECK(pattern.AppendComponent("*"));
    WIN32_FIND_DATAW fd;
    HANDLE find = SalFindFirstFile(pattern.Get(), &fd);
    CHECK(find != INVALID_HANDLE_VALUE);
    BOOL seen = FALSE;
    if (find != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (wcscmp(fd.cFileName, L"c\x030C-deep.txt") == 0)
                seen = TRUE;
        } while (SalFindNextFile(find, &fd));
        FindClose(find);
    }
    CHECK(seen);

    // rename + copy + delete at depth
    CSalPathBuf file2(dir);
    CHECK(file2.AppendComponent("renamed-" U8_C_CARON_NFC ".txt"));
    CHECK(SalMoveFile(file.Get(), file2.Get()));
    CSalPathBuf file3(dir);
    CHECK(file3.AppendComponent("copy.txt"));
    CHECK(SalCopyFile(file2.Get(), file3.Get(), TRUE));
    CHECK(SalDeleteFile(file2.Get()));
    CHECK(SalDeleteFile(file3.Get()));

    // tear down the deep tree
    while (dir.Length() > base.Length())
    {
        CHECK(SalRemoveDirectory(dir.Get()));
        CHECK(dir.CutLastComponent());
    }
    CHECK(SalRemoveDirectory(base.Get()));
}

// UTF-8 "ěščř" (2 bytes per char)
#define U8_ESCR "\xC4\x9B\xC5\xA1\xC4\x8D\xC5\x99"

static void TestDropFiles()
{
    // --- build a wide CF_HDROP block from two >MAX_PATH Czech-diacritics paths
    char longA[600];
    char longB[600];
    strcpy(longA, "C:\\salamander-test\\" U8_ESCR);
    while (strlen(longA) < 560)
        strcat(longA, "\\dir-" U8_ESCR);
    strcpy(longB, longA);
    strcat(longB, "\\soubor-" U8_ESCR ".txt");
    const char* paths[2] = {longA, longB};

    HGLOBAL h = SalBuildWideDropFiles(paths, 2);
    CHECK(h != NULL);
    if (h != NULL)
    {
        SIZE_T size = GlobalSize(h);
        DROPFILES* df = (DROPFILES*)GlobalLock(h);
        CHECK(df != NULL);
        if (df != NULL)
        {
            CHECK(df->fWide);
            CHECK(df->pFiles == sizeof(DROPFILES));

            // scan reports both paths and the exact longest length
            WCHAR* wideA = SalU8ToWAlloc(longA);
            WCHAR* wideB = SalU8ToWAlloc(longB);
            CHECK(wideA != NULL && wideB != NULL);
            int longest = 0;
            CHECK(SalScanDropFiles(df, size, &longest) == 2);
            if (wideA != NULL && wideB != NULL)
            {
                CHECK(longest == (int)wcslen(wideB));
                CHECK((int)wcslen(wideB) > MAX_PATH); // the scenario actually exceeds the legacy limit

                // content round-trip: both wide strings are stored verbatim
                const WCHAR* s = (const WCHAR*)((const BYTE*)df + df->pFiles);
                CHECK(wcscmp(s, wideA) == 0);
                s += wcslen(s) + 1;
                CHECK(wcscmp(s, wideB) == 0);
                s += wcslen(s) + 1;
                CHECK(*s == 0); // double-NUL terminated

                // malformed blocks are rejected, never over-read (exact content
                // size -- GlobalSize may round the allocation up)
                SIZE_T exactSize = sizeof(DROPFILES) +
                                   (wcslen(wideA) + 1 + wcslen(wideB) + 1 + 1) * sizeof(WCHAR);
                CHECK(SalScanDropFiles(df, sizeof(DROPFILES) - 1, NULL) == -1);         // truncated header
                CHECK(SalScanDropFiles(df, exactSize - 2 * sizeof(WCHAR), NULL) == -1); // missing double-NUL
            }
            free(wideA);
            free(wideB);

            GlobalUnlock(h);
        }
        GlobalFree(h);
    }

    // --- ANSI (fWide=0) blocks are scanned too (foreign legacy producers)
    {
        const char list[] = "C:\\aa\0C:\\bbb\0";
        BYTE block[sizeof(DROPFILES) + sizeof(list)];
        memset(block, 0, sizeof(block));
        DROPFILES* df = (DROPFILES*)block;
        df->pFiles = sizeof(DROPFILES);
        df->fWide = FALSE;
        memcpy(block + sizeof(DROPFILES), list, sizeof(list));
        int longest = 0;
        CHECK(SalScanDropFiles(df, sizeof(block), &longest) == 2);
        CHECK(longest == 6); // "C:\bbb"
    }

    // --- degenerate inputs
    CHECK(SalBuildWideDropFiles(NULL, 1) == NULL);
    CHECK(SalBuildWideDropFiles(paths, 0) == NULL);
    const char* invalid[1] = {"\xC4"}; // invalid UTF-8: caller must fall back to the legacy route
    CHECK(SalBuildWideDropFiles(invalid, 1) == NULL);
    CHECK(SalScanDropFiles(NULL, 1000, NULL) == -1);
}

// ---------------------------------------------------------------------------
// Feature 028: Dark theme palette tests (src/common/themes_palette.h)
// WCAG 2.x contrast: standard text >= 4.5:1, disabled/secondary >= 3:1 (SC-005)

static double SrgbChannel(int c)
{
    double s = c / 255.0;
    return s <= 0.03928 ? s / 12.92 : pow((s + 0.055) / 1.055, 2.4);
}

static double Luminance(COLORREF c)
{
    return 0.2126 * SrgbChannel(GetRValue(c)) +
           0.7152 * SrgbChannel(GetGValue(c)) +
           0.0722 * SrgbChannel(GetBValue(c));
}

static double ContrastRatio(COLORREF a, COLORREF b)
{
    double la = Luminance(a) + 0.05;
    double lb = Luminance(b) + 0.05;
    return la > lb ? la / lb : lb / la;
}

// positional views of the palette data (order = list order in the header)
enum DarkPanelIdx
{
#define TP_ENUM(name, r, g, b) DP_##name,
    THEME_DARK_PANEL_COLORS(TP_ENUM)
#undef TP_ENUM
        DP_COUNT
};

enum DarkViewerIdx
{
#define TV_ENUM(name, r, g, b) DV_##name,
    THEME_DARK_VIEWER_COLORS(TV_ENUM)
#undef TV_ENUM
        DV_COUNT
};

static void TestDarkThemePalette()
{
    // --- chrome palette: build the LUT the app uses
    COLORREF chrome[64];
    BOOL chromeSet[64] = {0};
    for (int i = 0; i < 64; i++)
        chrome[i] = 0;
#define TC_FILL(idx, r, g, b) \
    chrome[idx] = RGB(r, g, b); \
    chromeSet[idx] = TRUE;
    THEME_DARK_SYSCOLORS(TC_FILL)
#undef TC_FILL

    // every COLOR_* index the application draws with must be mapped
    // (COLOR_3DFACE==COLOR_BTNFACE and COLOR_3DSHADOW==COLOR_BTNSHADOW share values)
    const int drawnIndexes[] = {
        COLOR_WINDOW, COLOR_WINDOWTEXT, COLOR_WINDOWFRAME, COLOR_BTNFACE,
        COLOR_BTNTEXT, COLOR_BTNSHADOW, COLOR_BTNHIGHLIGHT, COLOR_3DLIGHT,
        COLOR_3DDKSHADOW, COLOR_HIGHLIGHT, COLOR_HIGHLIGHTTEXT, COLOR_GRAYTEXT,
        COLOR_HOTLIGHT, COLOR_INFOTEXT, COLOR_INFOBK, COLOR_CAPTIONTEXT,
        COLOR_ACTIVECAPTION, COLOR_INACTIVECAPTION, COLOR_INACTIVECAPTIONTEXT,
        COLOR_SCROLLBAR, COLOR_MENU, COLOR_MENUTEXT, COLOR_3DFACE, COLOR_3DSHADOW};
    for (int i = 0; i < (int)(sizeof(drawnIndexes) / sizeof(drawnIndexes[0])); i++)
        CHECK(chromeSet[drawnIndexes[i]]);

    // chrome text/background pairs (>= 4.5:1; disabled text >= 3:1)
    CHECK(ContrastRatio(chrome[COLOR_WINDOWTEXT], chrome[COLOR_WINDOW]) >= 4.5);
    CHECK(ContrastRatio(chrome[COLOR_BTNTEXT], chrome[COLOR_BTNFACE]) >= 4.5);
    CHECK(ContrastRatio(chrome[COLOR_MENUTEXT], chrome[COLOR_MENU]) >= 4.5);
    CHECK(ContrastRatio(chrome[COLOR_HIGHLIGHTTEXT], chrome[COLOR_HIGHLIGHT]) >= 4.5);
    CHECK(ContrastRatio(chrome[COLOR_INFOTEXT], chrome[COLOR_INFOBK]) >= 4.5);
    CHECK(ContrastRatio(chrome[COLOR_CAPTIONTEXT], chrome[COLOR_ACTIVECAPTION]) >= 4.5);
    CHECK(ContrastRatio(chrome[COLOR_INACTIVECAPTIONTEXT], chrome[COLOR_INACTIVECAPTION]) >= 4.5);
    CHECK(ContrastRatio(chrome[COLOR_HOTLIGHT], chrome[COLOR_WINDOW]) >= 4.5);
    CHECK(ContrastRatio(chrome[COLOR_HOTLIGHT], chrome[COLOR_BTNFACE]) >= 4.5);
    CHECK(ContrastRatio(chrome[COLOR_GRAYTEXT], chrome[COLOR_BTNFACE]) >= 3.0);
    CHECK(ContrastRatio(chrome[COLOR_GRAYTEXT], chrome[COLOR_WINDOW]) >= 3.0);

    // --- panel palette: exact index count (positional integrity vs consts.h
    // is additionally static_assert-ed inside the application build)
    CHECK(DP_COUNT == 34);
    CHECK(DV_COUNT == 4);

    COLORREF panel[DP_COUNT];
#define TP_FILL(name, r, g, b) panel[DP_##name] = RGB(r, g, b);
    THEME_DARK_PANEL_COLORS(TP_FILL)
#undef TP_FILL

    // panel item text over its backgrounds (all item states, SC-005)
    CHECK(ContrastRatio(panel[DP_ITEM_FG_NORMAL], panel[DP_ITEM_BK_NORMAL]) >= 4.5);
    CHECK(ContrastRatio(panel[DP_ITEM_FG_SELECTED], panel[DP_ITEM_BK_SELECTED]) >= 4.5);
    CHECK(ContrastRatio(panel[DP_ITEM_FG_FOCUSED], panel[DP_ITEM_BK_FOCUSED]) >= 4.5);
    CHECK(ContrastRatio(panel[DP_ITEM_FG_FOCSEL], panel[DP_ITEM_BK_FOCSEL]) >= 4.5);
    CHECK(ContrastRatio(panel[DP_ITEM_FG_HIGHLIGHT], panel[DP_ITEM_BK_HIGHLIGHT]) >= 4.5);
    CHECK(ContrastRatio(panel[DP_HOT_PANEL], panel[DP_ITEM_BK_NORMAL]) >= 4.5);
    CHECK(ContrastRatio(panel[DP_ACTIVE_CAPTION_FG], panel[DP_ACTIVE_CAPTION_BK]) >= 4.5);
    CHECK(ContrastRatio(panel[DP_INACTIVE_CAPTION_FG], panel[DP_INACTIVE_CAPTION_BK]) >= 4.5);
    CHECK(ContrastRatio(panel[DP_HOT_ACTIVE], panel[DP_ACTIVE_CAPTION_BK]) >= 4.5);
    CHECK(ContrastRatio(panel[DP_HOT_INACTIVE], panel[DP_INACTIVE_CAPTION_BK]) >= 4.5);
    CHECK(ContrastRatio(panel[DP_PROGRESS_FG_NORMAL], panel[DP_PROGRESS_BK_NORMAL]) >= 4.5);
    CHECK(ContrastRatio(panel[DP_PROGRESS_FG_SELECTED], panel[DP_PROGRESS_BK_SELECTED]) >= 4.5);

    COLORREF viewer[DV_COUNT];
#define TV_FILL(name, r, g, b) viewer[DV_##name] = RGB(r, g, b);
    THEME_DARK_VIEWER_COLORS(TV_FILL)
#undef TV_FILL
    CHECK(ContrastRatio(viewer[DV_VIEWER_FG_NORMAL], viewer[DV_VIEWER_BK_NORMAL]) >= 4.5);
    CHECK(ContrastRatio(viewer[DV_VIEWER_FG_SELECTED], viewer[DV_VIEWER_BK_SELECTED]) >= 4.5);

    // all surfaces are truly dark (backgrounds darker than mid-gray)
    CHECK(Luminance(chrome[COLOR_WINDOW]) < 0.1);
    CHECK(Luminance(chrome[COLOR_BTNFACE]) < 0.1);
    CHECK(Luminance(panel[DP_ITEM_BK_NORMAL]) < 0.1);
    CHECK(Luminance(viewer[DV_VIEWER_BK_NORMAL]) < 0.1);
}

// ---------------------------------------------------------------------------
// Feature 029: dark adaptation of toolbar glyph colors
// (ThemeDarkAdaptColor in src/common/themes_palette.h; SC-002: adapted
// neutral strokes must reach >= 3:1 contrast on the dark COLOR_BTNFACE)

static void TestDarkIconColorAdaptation()
{
    const COLORREF darkBtnFace = RGB(45, 45, 45); // THEME_DARK_SYSCOLORS COLOR_BTNFACE
    int r, g, b;

    // pure black (typical outline) becomes the lightest adapted gray
    r = g = b = 0;
    ThemeDarkAdaptColor(&r, &g, &b);
    CHECK(r == 220 && g == 220 && b == 220);

    // neutral sweep [0,140): output stays neutral, lands in (140,220],
    // is monotonically non-increasing, and clears 3:1 on the dark toolbar
    int prev = 220;
    for (int v = 0; v < 140; v++)
    {
        r = g = b = v;
        ThemeDarkAdaptColor(&r, &g, &b);
        CHECK(r == g && g == b);
        CHECK(r > 140 && r <= 220);
        CHECK(r <= prev);
        prev = r;
        CHECK(ContrastRatio(RGB(r, g, b), darkBtnFace) >= 3.0);
    }

    // neutrals at/above 140 and white are left untouched
    for (int v = 140; v <= 255; v += 5)
    {
        r = g = b = v;
        ThemeDarkAdaptColor(&r, &g, &b);
        CHECK(r == v && g == v && b == v);
    }
    r = g = b = 255;
    ThemeDarkAdaptColor(&r, &g, &b);
    CHECK(r == 255 && g == 255 && b == 255);

    // dark saturated color: max channel scales to 170, hue (ratios) kept
    r = 100, g = 0, b = 0;
    ThemeDarkAdaptColor(&r, &g, &b);
    CHECK(r == 170 && g == 0 && b == 0);
    r = 60, g = 30, b = 0; // 2:1 red:green ratio must survive
    ThemeDarkAdaptColor(&r, &g, &b);
    CHECK(r == 170 && g == 85 && b == 0);
    r = 0, g = 0, b = 100; // dark blue accent brightens toward the same hue
    ThemeDarkAdaptColor(&r, &g, &b);
    CHECK(r == 0 && g == 0 && b == 170);

    // bright saturated accents are left untouched (colored icons stay colored)
    r = 255, g = 201, b = 14; // folder yellow
    ThemeDarkAdaptColor(&r, &g, &b);
    CHECK(r == 255 && g == 201 && b == 14);
    r = 0, g = 0, b = 255;
    ThemeDarkAdaptColor(&r, &g, &b);
    CHECK(r == 0 && g == 0 && b == 255);
    r = 200, g = 60, b = 60;
    ThemeDarkAdaptColor(&r, &g, &b);
    CHECK(r == 200 && g == 60 && b == 60);

    // deterministic: same input always produces the same output
    int r2 = 17, g2 = 17, b2 = 17;
    r = 17, g = 17, b = 17;
    ThemeDarkAdaptColor(&r, &g, &b);
    ThemeDarkAdaptColor(&r2, &g2, &b2);
    CHECK(r == r2 && g == g2 && b == b2);
}

int main()
{
    TestConversions();
    TestNormalization();
    TestMatching();
    TestPathBuf();
    TestExtendedPaths();
    TestFileIO();
    TestDropFiles();
    TestDarkThemePalette();
    TestDarkIconColorAdaptation();

    printf("saltests: %d checks, %d failed\n", g_checks, g_failures);
    return g_failures;
}
