// SPDX-FileCopyrightText: 2026 Pavel Stupka
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Lightweight precomp.h used ONLY by the dev unit-test harness so that
// sftputils.cpp (which begins with #include "precomp.h") can be compiled
// standalone, without pulling in the whole Salamander plugin SDK. The real
// plugin build uses the SDK precomp.h; this shim is copied to "precomp.h"
// in a flat temp dir by build_and_run.cmd.

#pragma once
#include <winsock2.h>
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// feature 051: keyload.cpp is compiled standalone here too (key-format fixtures
// in harness.cpp). Stub the two SDK touchpoints it needs: the UTF-8 -> UTF-16
// path helper (interface 104) and the resource-id constants used for the reject
// messages. The harness only checks WHICH reason is reported, never its text.
static inline WCHAR* SplU8ToWExtAlloc(const char* u8)
{
    if (u8 == NULL)
        return NULL;
    int n = MultiByteToWideChar(CP_UTF8, 0, u8, -1, NULL, 0);
    if (n <= 0)
        return NULL;
    WCHAR* w = (WCHAR*)malloc((size_t)n * sizeof(WCHAR));
    if (w != NULL && MultiByteToWideChar(CP_UTF8, 0, u8, -1, w, n) <= 0)
    {
        free(w);
        return NULL;
    }
    return w;
}

#define IDS_ERR_AUTHKEY 46
#define IDS_ERR_KEYPUTTY 55
#define IDS_ERR_KEYPKCS8 57
#define IDS_ERR_KEYTYPEUNSUP 58
