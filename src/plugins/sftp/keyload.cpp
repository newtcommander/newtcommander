// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "sftp.h"
#include "keyload.h"

CSFTPKeyFormat DetectKeyFormat(const char* keyFilePath)
{
    HANDLE h = CreateFileA(keyFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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

BOOL KeyFormatSupported(CSFTPKeyFormat fmt, int* reasonStrId)
{
    switch (fmt)
    {
    case kfPEM:
    case kfPKCS8:
    case kfOpenSSH:
        return TRUE;
    case kfPuTTY:
        // .ppk v2 is handled (US5); v3 (Argon2) is rejected with a clear message.
        // Detection of v2 vs v3 is done by the key loader; treat as supported
        // here and let the loader reject v3 by content.
        return TRUE;
    default:
        if (reasonStrId != NULL)
            *reasonStrId = IDS_ERR_AUTHKEY;
        return FALSE;
    }
}
