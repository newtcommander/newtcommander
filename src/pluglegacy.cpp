// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "pluglegacy.h"

BOOL PlugLegacyU8ToACP(const char* u8, char* buf, int bufSize)
{
    if (buf == NULL || bufSize <= 0)
        return FALSE;
    buf[0] = 0;
    if (u8 == NULL || *u8 == 0)
        return TRUE;
    WCHAR* w = SalU8ToWAlloc(u8);
    if (w == NULL)
        return FALSE; // not valid UTF-8: refuse rather than guess
    BOOL ret = SalWToACPLossless(w, -1, buf, bufSize);
    free(w);
    return ret;
}

BOOL PlugLegacyACPToU8(const char* acp, char* buf, int bufSize)
{
    if (buf == NULL || bufSize <= 0)
        return FALSE;
    buf[0] = 0;
    if (acp == NULL || *acp == 0)
        return TRUE;
    int wlen = MultiByteToWideChar(CP_ACP, 0, acp, -1, NULL, 0);
    if (wlen <= 0)
        return FALSE;
    WCHAR* w = (WCHAR*)malloc(wlen * sizeof(WCHAR));
    if (w == NULL)
        return FALSE;
    MultiByteToWideChar(CP_ACP, 0, acp, -1, w, wlen);
    BOOL ret = SalWToU8(w, -1, buf, bufSize) != 0;
    free(w);
    return ret;
}

char* PlugLegacyACPToU8Alloc(const char* acp)
{
    if (acp == NULL)
        return NULL;
    int wlen = MultiByteToWideChar(CP_ACP, 0, acp, -1, NULL, 0);
    if (wlen <= 0)
        return NULL;
    WCHAR* w = (WCHAR*)malloc(wlen * sizeof(WCHAR));
    if (w == NULL)
        return NULL;
    MultiByteToWideChar(CP_ACP, 0, acp, -1, w, wlen);
    char* u8 = SalWToU8Alloc(w, -1);
    free(w);
    return u8;
}
