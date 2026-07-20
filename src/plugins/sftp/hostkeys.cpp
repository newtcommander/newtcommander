// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "sftp.h"
#include "hostkeys.h"

CSFTPHostKeyList HostKeys;

CSFTPHostKey::CSFTPHostKey()
{
    Host = NULL;
    Port = 22;
    KeyType = NULL;
    KeyBlobB64 = NULL;
    FingerprintSHA256 = NULL;
    Added.dwLowDateTime = Added.dwHighDateTime = 0;
}

CSFTPHostKey::~CSFTPHostKey()
{
    if (Host != NULL)
        free(Host);
    if (KeyType != NULL)
        free(KeyType);
    if (KeyBlobB64 != NULL)
        free(KeyBlobB64);
    if (FingerprintSHA256 != NULL)
        free(FingerprintSHA256);
}

static void NormalizeHost(const char* host, char* out, int outSize)
{
    int i = 0;
    for (; host[i] != 0 && i < outSize - 1; i++)
        out[i] = (char)tolower((unsigned char)host[i]);
    out[i] = 0;
}

CSFTPHostKey* CSFTPHostKeyList::Find(const char* host, int port)
{
    char norm[256];
    NormalizeHost(host, norm, sizeof(norm));
    for (int i = 0; i < Count; i++)
    {
        CSFTPHostKey* k = At(i);
        if (k->Port == port && k->Host != NULL && strcmp(k->Host, norm) == 0)
            return k;
    }
    return NULL;
}

CSFTPHostKeyMatch CSFTPHostKeyList::Match(const char* host, int port, const char* keyBlobB64,
                                          char* storedFp, int storedFpSize)
{
    CSFTPHostKey* k = Find(host, port);
    if (k == NULL)
        return hkmUnknown;
    if (k->KeyBlobB64 != NULL && keyBlobB64 != NULL && strcmp(k->KeyBlobB64, keyBlobB64) == 0)
        return hkmMatch;
    if (storedFp != NULL && k->FingerprintSHA256 != NULL)
        lstrcpynA(storedFp, k->FingerprintSHA256, storedFpSize);
    return hkmMismatch;
}

BOOL CSFTPHostKeyList::Trust(const char* host, int port, const char* keyType,
                             const char* keyBlobB64, const char* fingerprint)
{
    char norm[256];
    NormalizeHost(host, norm, sizeof(norm));

    CSFTPHostKey* k = Find(host, port);
    BOOL isNew = FALSE;
    if (k == NULL)
    {
        k = new CSFTPHostKey;
        if (k == NULL)
            return FALSE;
        k->Host = _strdup(norm);
        k->Port = port;
        isNew = TRUE;
    }
    // replace key material
    if (k->KeyType != NULL)
        free(k->KeyType);
    if (k->KeyBlobB64 != NULL)
        free(k->KeyBlobB64);
    if (k->FingerprintSHA256 != NULL)
        free(k->FingerprintSHA256);
    k->KeyType = _strdup(keyType != NULL ? keyType : "");
    k->KeyBlobB64 = _strdup(keyBlobB64 != NULL ? keyBlobB64 : "");
    k->FingerprintSHA256 = _strdup(fingerprint != NULL ? fingerprint : "");
    // CF-19: never keep a half-populated entry on OOM - a NULL host/blob would be
    // unmatchable or persisted broken. Fail closed (Match already treats a NULL
    // stored blob as a mismatch, so the next connect re-verifies).
    if ((isNew && k->Host == NULL) || k->KeyType == NULL || k->KeyBlobB64 == NULL ||
        k->FingerprintSHA256 == NULL)
    {
        if (isNew)
            delete k; // not yet added to the list
        return FALSE;
    }
    GetSystemTimeAsFileTime(&k->Added);

    if (isNew)
    {
        Add(k);
        if (!IsGood())
        {
            ResetState();
            delete k;
            return FALSE;
        }
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// Base64 + fingerprint formatting
// ---------------------------------------------------------------------------

static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* Base64Encode(const unsigned char* data, int len)
{
    int outLen = ((len + 2) / 3) * 4;
    char* out = (char*)malloc(outLen + 1);
    if (out == NULL)
        return NULL;
    int o = 0;
    for (int i = 0; i < len; i += 3)
    {
        unsigned long n = (unsigned long)data[i] << 16;
        if (i + 1 < len)
            n |= (unsigned long)data[i + 1] << 8;
        if (i + 2 < len)
            n |= data[i + 2];
        out[o++] = B64[(n >> 18) & 0x3F];
        out[o++] = B64[(n >> 12) & 0x3F];
        out[o++] = (i + 1 < len) ? B64[(n >> 6) & 0x3F] : '=';
        out[o++] = (i + 2 < len) ? B64[n & 0x3F] : '=';
    }
    out[o] = 0;
    return out;
}

void FormatSHA256Fingerprint(const unsigned char* hash, char* buf, int bufSize)
{
    // OpenSSH form: "SHA256:" + base64(hash) without trailing '=' padding
    char* b64 = Base64Encode(hash, 32);
    if (b64 == NULL)
    {
        lstrcpynA(buf, "SHA256:?", bufSize);
        return;
    }
    // strip '=' padding
    int l = (int)strlen(b64);
    while (l > 0 && b64[l - 1] == '=')
        b64[--l] = 0;
    _snprintf_s(buf, bufSize, _TRUNCATE, "SHA256:%s", b64);
    free(b64);
}

void FormatMD5Fingerprint(const unsigned char* hash, char* buf, int bufSize)
{
    char* p = buf;
    int remaining = bufSize;
    for (int i = 0; i < 16 && remaining > 3; i++)
    {
        int n = _snprintf_s(p, remaining, _TRUNCATE, i == 0 ? "%02x" : ":%02x", hash[i]);
        if (n <= 0)
            break;
        p += n;
        remaining -= n;
    }
}
