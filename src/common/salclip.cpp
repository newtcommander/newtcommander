// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include <windows.h>
#include <shlobj.h> // DROPFILES

#include "salunicode.h"
#include "salclip.h"

HGLOBAL SalBuildWideDropFiles(const char* const* utf8Paths, int count)
{
    if (utf8Paths == NULL || count <= 0)
        return NULL;

    WCHAR** wide = (WCHAR**)calloc(count, sizeof(WCHAR*));
    if (wide == NULL)
        return NULL;

    HGLOBAL ret = NULL;
    SIZE_T totalChars = 1; // the final extra NUL of the double-NUL terminator
    int i;
    for (i = 0; i < count; i++)
    {
        wide[i] = SalU8ToWAlloc(utf8Paths[i]);
        if (wide[i] == NULL || wide[i][0] == 0)
            goto cleanup; // invalid UTF-8 or empty path: caller falls back to the legacy route
        totalChars += wcslen(wide[i]) + 1;
    }

    {
        SIZE_T size = sizeof(DROPFILES) + totalChars * sizeof(WCHAR);
        HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE | GMEM_ZEROINIT, size);
        if (h != NULL)
        {
            DROPFILES* df = (DROPFILES*)GlobalLock(h);
            if (df != NULL)
            {
                df->pFiles = sizeof(DROPFILES);
                df->fWide = TRUE;
                WCHAR* dst = (WCHAR*)((BYTE*)df + sizeof(DROPFILES));
                for (i = 0; i < count; i++)
                {
                    SIZE_T len = wcslen(wide[i]);
                    memcpy(dst, wide[i], (len + 1) * sizeof(WCHAR));
                    dst += len + 1;
                }
                *dst = 0; // double-NUL terminator (block is zero-inited, but be explicit)
                GlobalUnlock(h);
                ret = h;
            }
            else
                GlobalFree(h);
        }
    }

cleanup:
    for (i = 0; i < count; i++)
        free(wide[i]);
    free(wide);
    return ret;
}

int SalScanDropFiles(const DROPFILES* df, SIZE_T blockSize, int* longestLen)
{
    if (longestLen != NULL)
        *longestLen = 0;
    if (df == NULL || blockSize < sizeof(DROPFILES) || df->pFiles > blockSize)
        return -1;

    int count = 0;
    int longest = 0;
    if (df->fWide)
    {
        const WCHAR* s = (const WCHAR*)((const BYTE*)df + df->pFiles);
        SIZE_T remaining = (blockSize - df->pFiles) / sizeof(WCHAR);
        while (remaining > 0 && *s != 0)
        {
            SIZE_T len = 0;
            while (len < remaining && s[len] != 0)
                len++;
            if (len >= remaining)
                return -1; // unterminated string: malformed block
            count++;
            if ((int)len > longest)
                longest = (int)len;
            s += len + 1;
            remaining -= len + 1;
        }
        if (remaining == 0)
            return -1; // missing double-NUL terminator
    }
    else
    {
        const char* s = (const char*)df + df->pFiles;
        SIZE_T remaining = blockSize - df->pFiles;
        while (remaining > 0 && *s != 0)
        {
            SIZE_T len = 0;
            while (len < remaining && s[len] != 0)
                len++;
            if (len >= remaining)
                return -1;
            count++;
            if ((int)len > longest)
                longest = (int)len;
            s += len + 1;
            remaining -= len + 1;
        }
        if (remaining == 0)
            return -1;
    }
    if (longestLen != NULL)
        *longestLen = longest;
    return count;
}
