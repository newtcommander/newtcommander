// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

// Unit tests for the 004-long-paths-unicode foundation helpers
// (src/common/salunicode.cpp, src/common/salpath.cpp).
// Console exe; exit code = number of failed checks.

#include "precomp.h"

#include "salunicode.h"
#include "salpath.h"
#include "salfileio.h"
#include "salclip.h"

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
    WCHAR* nfc = SalNormalizeNFCAlloc(L"c\x030C" L".txt");
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

int main()
{
    TestConversions();
    TestNormalization();
    TestMatching();
    TestPathBuf();
    TestExtendedPaths();
    TestFileIO();
    TestDropFiles();

    printf("saltests: %d checks, %d failed\n", g_checks, g_failures);
    return g_failures;
}
