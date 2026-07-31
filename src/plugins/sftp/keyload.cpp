// SPDX-FileCopyrightText: 2026 Pavel Stupka
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "sftp.h"
#include "keyload.h"

CSFTPKeyFormat DetectKeyFormat(const char* keyFilePath)
{
    // CF-9: the key path is UTF-8; open via the W API (the ANSI one mangles
    // non-ACP paths and would misdetect the format as kfUnknown).
    WCHAR* w = SplU8ToWExtAlloc(keyFilePath);
    if (w == NULL)
        return kfUnknown;
    HANDLE h = CreateFileW(w, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    free(w);
    if (h == INVALID_HANDLE_VALUE)
        return kfUnknown;

    char head[256];
    DWORD read = 0;
    ReadFile(h, head, sizeof(head) - 1, &read, NULL);
    CloseHandle(h);
    head[read] = 0;

    if (strstr(head, "PuTTY-User-Key-File") != NULL)
        return kfPuTTY;
    if (strstr(head, "BEGIN OPENSSH PRIVATE KEY") != NULL)
        return kfOpenSSH;
    if (strstr(head, "BEGIN ENCRYPTED PRIVATE KEY") != NULL ||
        strstr(head, "BEGIN PRIVATE KEY") != NULL)
        return kfPKCS8;
    if (strstr(head, "BEGIN RSA PRIVATE KEY") != NULL ||
        strstr(head, "BEGIN EC PRIVATE KEY") != NULL ||
        strstr(head, "BEGIN DSA PRIVATE KEY") != NULL)
        return kfPEM;
    return kfUnknown;
}

BOOL KeyFileLooksEncrypted(const char* keyFilePath)
{
    WCHAR* w = SplU8ToWExtAlloc(keyFilePath);
    if (w == NULL)
        return FALSE;
    HANDLE h = CreateFileW(w, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    free(w);
    if (h == INVALID_HANDLE_VALUE)
        return FALSE;
    char head[1024];
    DWORD read = 0;
    ReadFile(h, head, sizeof(head) - 1, &read, NULL);
    CloseHandle(h);
    head[read] = 0;
    // classic PEM encryption markers and encrypted PKCS#8
    return strstr(head, "ENCRYPTED") != NULL || strstr(head, "Proc-Type:") != NULL ||
           strstr(head, "DEK-Info:") != NULL;
}

BOOL KeyFormatSupported(CSFTPKeyFormat fmt, int* reasonStrId)
{
    switch (fmt)
    {
    case kfPEM:
    case kfPKCS8:
    case kfOpenSSH:
        return TRUE;
    case kfPuTTY:
        // CF-10: libssh2 cannot parse PuTTY .ppk keys and the plugin has no .ppk
        // loader, so accepting them here only defers the failure to a cryptic
        // low-level error. Reject up front with a clear "convert to OpenSSH" hint.
        if (reasonStrId != NULL)
            *reasonStrId = IDS_ERR_KEYPUTTY;
        return FALSE;
    default:
        if (reasonStrId != NULL)
            *reasonStrId = IDS_ERR_AUTHKEY;
        return FALSE;
    }
}
