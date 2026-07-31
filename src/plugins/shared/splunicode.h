// SPDX-FileCopyrightText: 2026 Pavel Stupka
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

//*****************************************************************************
//
// splunicode.h
//
// Header-only UTF-8/UTF-16 helpers for plugins built against plugin
// interface 104 (feature 004-long-paths-unicode). Since interface 104
// every char* file name and path crossing the Salamander plugin
// interface is UTF-8; see doc/plugin-vnext-migration.md.
//
// Use these helpers to call W file APIs from a plugin:
//
//   WCHAR* w = SplU8ToWExtAlloc(u8path);          // \\?\-prefixed, for file APIs
//   HANDLE h = CreateFileW(w, ...);
//   free(w);
//
// All allocating helpers return NULL on failure; free() the results.
//

#include <windows.h>
#include <stdlib.h>
#include <string.h>

// UTF-8 -> UTF-16 (strict: fails on invalid sequences); free() the result
inline WCHAR* SplU8ToWAlloc(const char* u8)
{
    if (u8 == NULL)
        return NULL;
    int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, u8, -1, NULL, 0);
    if (len <= 0)
        return NULL;
    WCHAR* w = (WCHAR*)malloc(len * sizeof(WCHAR));
    if (w != NULL)
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, u8, -1, w, len);
    return w;
}

// UTF-16 -> UTF-8 (strict); free() the result
inline char* SplWToU8Alloc(const WCHAR* w)
{
    if (w == NULL)
        return NULL;
    int len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, w, -1, NULL, 0, NULL, NULL);
    if (len <= 0)
        return NULL;
    char* u8 = (char*)malloc(len);
    if (u8 != NULL)
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, w, -1, u8, len, NULL, NULL);
    return u8;
}

// UTF-16 -> UTF-8 into a caller buffer; returns bytes written incl. the
// terminator, 0 on failure (invalid input or buffer too small)
inline int SplWToU8(const WCHAR* w, char* buf, int bufSize)
{
    if (w == NULL || buf == NULL || bufSize <= 0)
        return 0;
    int res = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, w, -1, buf, bufSize, NULL, NULL);
    if (res <= 0 && bufSize > 0)
        buf[0] = 0;
    return res;
}

// UTF-8 -> UTF-16 into a caller buffer; returns WCHARs written incl. the
// terminator, 0 on failure
inline int SplU8ToW(const char* u8, WCHAR* buf, int bufSizeInWchars)
{
    if (u8 == NULL || buf == NULL || bufSizeInWchars <= 0)
        return 0;
    int res = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, u8, -1, buf, bufSizeInWchars);
    if (res <= 0 && bufSizeInWchars > 0)
        buf[0] = 0;
    return res;
}

// TRUE when the buffer contains only ASCII (fast-path predicate)
inline BOOL SplIsASCII(const char* s)
{
    if (s == NULL)
        return TRUE;
    for (; *s != 0; s++)
        if ((unsigned char)*s >= 0x80)
            return FALSE;
    return TRUE;
}

// UTF-8 display-form path -> heap UTF-16 extended-length path for W file
// APIs: "C:\..." -> "\\?\C:\...", "\\server\share\..." -> "\\?\UNC\...";
// already-prefixed paths pass through; relative paths are returned
// unprefixed (extended-length form requires absolute paths).
// free() the result; NULL on conversion failure.
inline WCHAR* SplU8ToWExtAlloc(const char* u8path)
{
    WCHAR* w = SplU8ToWAlloc(u8path);
    if (w == NULL)
        return NULL;
    if (wcsncmp(w, L"\\\\?\\", 4) == 0)
        return w; // already extended
    BOOL isDrive = ((w[0] >= L'A' && w[0] <= L'Z') || (w[0] >= L'a' && w[0] <= L'z')) &&
                   w[1] == L':' && w[2] == L'\\';
    BOOL isUNC = w[0] == L'\\' && w[1] == L'\\';
    if (!isDrive && !isUNC)
        return w; // relative path: cannot be \\?\-prefixed, return as-is
    const WCHAR* prefix = isDrive ? L"\\\\?\\" : L"\\\\?\\UNC\\";
    size_t skip = isDrive ? 0 : 2;
    size_t prefixLen = wcslen(prefix);
    size_t len = wcslen(w);
    WCHAR* ext = (WCHAR*)malloc((prefixLen + len - skip + 1) * sizeof(WCHAR));
    if (ext != NULL)
    {
        memcpy(ext, prefix, prefixLen * sizeof(WCHAR));
        memcpy(ext + prefixLen, w + skip, (len - skip + 1) * sizeof(WCHAR));
    }
    free(w);
    return ext;
}
