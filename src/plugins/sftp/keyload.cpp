// SPDX-FileCopyrightText: 2026 Pavel Stupka
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include <wincrypt.h> // CryptStringToBinaryA: base64 body of an OpenSSH container
#include "sftp.h"
#include "keyload.h"

// Reads the head of a key file. Returns the number of bytes read (0 on error).
// The buffer is always NUL-terminated. CF-9: the key path is UTF-8, so open via
// the W API (the ANSI one mangles non-ACP paths).
static DWORD ReadKeyHead(const char* keyFilePath, char* buf, DWORD bufSize)
{
    if (bufSize == 0)
        return 0;
    buf[0] = 0;
    WCHAR* w = SplU8ToWExtAlloc(keyFilePath);
    if (w == NULL)
        return 0;
    HANDLE h = CreateFileW(w, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    free(w);
    if (h == INVALID_HANDLE_VALUE)
        return 0;
    DWORD read = 0;
    ReadFile(h, buf, bufSize - 1, &read, NULL);
    CloseHandle(h);
    buf[read] = 0;
    return read;
}

CSFTPKeyFormat DetectKeyFormat(const char* keyFilePath)
{
    char head[256];
    if (ReadKeyHead(keyFilePath, head, sizeof(head)) == 0)
        return kfUnknown;

    CSFTPKeyFormat fmt = kfUnknown;
    if (strstr(head, "PuTTY-User-Key-File") != NULL)
        fmt = kfPuTTY;
    else if (strstr(head, "BEGIN OPENSSH PRIVATE KEY") != NULL)
        fmt = kfOpenSSH;
    else if (strstr(head, "BEGIN ENCRYPTED PRIVATE KEY") != NULL ||
             strstr(head, "BEGIN PRIVATE KEY") != NULL)
        fmt = kfPKCS8;
    else if (strstr(head, "BEGIN RSA PRIVATE KEY") != NULL ||
             strstr(head, "BEGIN EC PRIVATE KEY") != NULL ||
             strstr(head, "BEGIN DSA PRIVATE KEY") != NULL)
        fmt = kfPEM;

    SecureZeroMemory(head, sizeof(head)); // CF-14: key bytes must not linger
    return fmt;
}

// Decodes the base64 body of an OpenSSH-container key. 'out' receives raw bytes;
// only the first bytes are needed (the algorithm name sits in the first record).
static DWORD DecodeOpenSSHHead(const char* keyFilePath, unsigned char* out, DWORD outSize)
{
    char text[2048];
    DWORD n = ReadKeyHead(keyFilePath, text, sizeof(text));
    if (n == 0)
        return 0;

    const char* begin = strstr(text, "-----BEGIN OPENSSH PRIVATE KEY-----");
    if (begin == NULL)
    {
        SecureZeroMemory(text, sizeof(text));
        return 0;
    }
    const char* body = strchr(begin, '\n');
    if (body == NULL)
    {
        SecureZeroMemory(text, sizeof(text));
        return 0;
    }
    body++;

    // Take whole base64 lines only; a partial trailing line would not decode.
    char b64[2048];
    DWORD b64Len = 0;
    for (const char* p = body; *p != 0 && b64Len + 1 < sizeof(b64); p++)
    {
        if (*p == '-')
            break; // reached the footer
        if (*p == '\r' || *p == '\n')
            continue;
        b64[b64Len++] = *p;
    }
    b64Len -= b64Len % 4; // drop an incomplete quantum from the truncated read
    b64[b64Len] = 0;

    DWORD decoded = outSize;
    BOOL ok = b64Len > 0 && CryptStringToBinaryA(b64, b64Len, CRYPT_STRING_BASE64,
                                                 out, &decoded, NULL, NULL);
    SecureZeroMemory(text, sizeof(text));
    SecureZeroMemory(b64, sizeof(b64));
    return ok ? decoded : 0;
}

// Reads a 4-byte big-endian length-prefixed string from 'data' at '*off'.
static BOOL GetSSHString(const unsigned char* data, DWORD dataLen, DWORD* off,
                         char* out, int outSize)
{
    if (*off + 4 > dataLen)
        return FALSE;
    DWORD len = ((DWORD)data[*off] << 24) | ((DWORD)data[*off + 1] << 16) |
                ((DWORD)data[*off + 2] << 8) | (DWORD)data[*off + 3];
    *off += 4;
    if (len > dataLen - *off)
        return FALSE;
    if (out != NULL)
    {
        DWORD copy = len < (DWORD)outSize - 1 ? len : (DWORD)outSize - 1;
        memcpy(out, data + *off, copy);
        out[copy] = 0;
    }
    *off += len;
    return TRUE;
}

// Parses the OpenSSH container header: magic, ciphername, kdfname, kdfoptions,
// key count, first public key (whose first field is the algorithm name).
static BOOL ParseOpenSSHHeader(const char* keyFilePath, char* cipherOut, int cipherSize,
                               char* typeOut, int typeSize)
{
    static const char magic[] = "openssh-key-v1";
    unsigned char raw[1024];
    DWORD n = DecodeOpenSSHHead(keyFilePath, raw, sizeof(raw));
    if (n < sizeof(magic) || memcmp(raw, magic, sizeof(magic)) != 0)
        return FALSE;

    DWORD off = sizeof(magic); // includes the trailing NUL of the magic
    char cipher[64] = "";
    if (!GetSSHString(raw, n, &off, cipher, sizeof(cipher)))
        return FALSE;
    if (cipherOut != NULL)
        lstrcpynA(cipherOut, cipher, cipherSize);

    if (typeOut != NULL)
    {
        typeOut[0] = 0;
        char kdf[64];
        if (!GetSSHString(raw, n, &off, kdf, sizeof(kdf)) || // kdfname
            !GetSSHString(raw, n, &off, NULL, 0))            // kdfoptions
            return FALSE;
        if (off + 4 > n)
            return FALSE;
        off += 4; // number of keys
        // first public key blob; its own first field is the algorithm name
        if (off + 4 > n)
            return FALSE;
        off += 4; // blob length
        if (!GetSSHString(raw, n, &off, typeOut, typeSize))
            return FALSE;
    }
    SecureZeroMemory(raw, sizeof(raw));
    return TRUE;
}

BOOL ReadOpenSSHKeyType(const char* keyFilePath, char* typeOut, int typeOutSize)
{
    if (typeOut == NULL || typeOutSize <= 0)
        return FALSE;
    typeOut[0] = 0;
    return ParseOpenSSHHeader(keyFilePath, NULL, 0, typeOut, typeOutSize) && typeOut[0] != 0;
}

BOOL KeyFileLooksEncrypted(const char* keyFilePath)
{
    char head[1024];
    if (ReadKeyHead(keyFilePath, head, sizeof(head)) == 0)
        return FALSE;
    // classic PEM encryption markers and encrypted PKCS#8
    BOOL enc = strstr(head, "ENCRYPTED") != NULL || strstr(head, "Proc-Type:") != NULL ||
               strstr(head, "DEK-Info:") != NULL;
    BOOL openSSH = strstr(head, "BEGIN OPENSSH PRIVATE KEY") != NULL;
    SecureZeroMemory(head, sizeof(head));
    if (enc)
        return TRUE;
    if (openSSH)
    {
        // feature 051: the container's cipher name is inside the base64 body,
        // so header sniffing alone used to miss encrypted OpenSSH keys.
        char cipher[64] = "";
        if (ParseOpenSSHHeader(keyFilePath, cipher, sizeof(cipher), NULL, 0) &&
            cipher[0] != 0 && strcmp(cipher, "none") != 0)
            return TRUE;
    }
    return FALSE;
}

BOOL KeyFormatSupported(CSFTPKeyFormat fmt, int* reasonStrId)
{
    switch (fmt)
    {
    case kfPEM:
        // classic PEM, encrypted or not (the WinCNG memory loader decrypts
        // Proc-Type/DEK-Info keys since feature 051)
        return TRUE;
    case kfOpenSSH:
        // openssh-key-v1 container holding RSA or ECDSA; the algorithm itself is
        // checked by KeyFileSupported (ed25519 is not available on WinCNG)
        return TRUE;
    case kfPKCS8:
        // feature 051: the WinCNG key loader has no PKCS#8 path, so accepting
        // these here only deferred the failure to a cryptic low-level error.
        if (reasonStrId != NULL)
            *reasonStrId = IDS_ERR_KEYPKCS8;
        return FALSE;
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

BOOL KeyFileSupported(const char* keyFilePath, int* reasonStrId,
                      char* typeOut, int typeOutSize)
{
    if (typeOut != NULL && typeOutSize > 0)
        typeOut[0] = 0;

    CSFTPKeyFormat fmt = DetectKeyFormat(keyFilePath);
    if (!KeyFormatSupported(fmt, reasonStrId))
        return FALSE;

    if (fmt == kfOpenSSH)
    {
        // FR-007: reject an algorithm this build cannot use before connecting.
        char type[64] = "";
        if (ReadOpenSSHKeyType(keyFilePath, type, sizeof(type)) &&
            strcmp(type, "ssh-rsa") != 0 && strncmp(type, "ecdsa-sha2-", 11) != 0)
        {
            if (reasonStrId != NULL)
                *reasonStrId = IDS_ERR_KEYTYPEUNSUP;
            if (typeOut != NULL && typeOutSize > 0)
                lstrcpynA(typeOut, type, typeOutSize);
            return FALSE;
        }
    }
    return TRUE;
}
