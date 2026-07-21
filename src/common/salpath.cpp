// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include <windows.h>

#include "salunicode.h"
#include "salpath.h"

//*****************************************************************************
//
// CSalPathBuf
//

CSalPathBuf::CSalPathBuf()
{
    Inline[0] = 0;
    Buffer = Inline;
    Len = 0;
    Capacity = sizeof(Inline);
}

CSalPathBuf::CSalPathBuf(const CSalPathBuf& other)
{
    Inline[0] = 0;
    Buffer = Inline;
    Len = 0;
    Capacity = sizeof(Inline);
    Set(other.Buffer, other.Len);
}

CSalPathBuf::~CSalPathBuf()
{
    if (Buffer != Inline)
        free(Buffer);
}

CSalPathBuf& CSalPathBuf::operator=(const CSalPathBuf& other)
{
    if (this != &other)
        Set(other.Buffer, other.Len);
    return *this;
}

void CSalPathBuf::Clear()
{
    Buffer[0] = 0;
    Len = 0;
}

BOOL CSalPathBuf::Reserve(int bytes)
{
    if (bytes <= Capacity)
        return TRUE;
    int newCap = Capacity < 260 ? 260 : Capacity;
    while (newCap < bytes)
        newCap *= 2;
    char* newBuf = (char*)malloc(newCap);
    if (newBuf == NULL)
        return FALSE;
    memcpy(newBuf, Buffer, Len + 1);
    if (Buffer != Inline)
        free(Buffer);
    Buffer = newBuf;
    Capacity = newCap;
    return TRUE;
}

BOOL CSalPathBuf::Set(const char* path)
{
    return Set(path, path == NULL ? 0 : (int)strlen(path));
}

BOOL CSalPathBuf::Set(const char* path, int len)
{
    if (path == NULL || len < 0)
        len = 0;
    if (!Reserve(len + 1))
        return FALSE;
    if (len > 0)
        memmove(Buffer, path, len);
    Buffer[len] = 0;
    Len = len;
    return TRUE;
}

BOOL CSalPathBuf::Append(const char* text)
{
    if (text == NULL)
        return TRUE;
    int add = (int)strlen(text);
    if (!Reserve(Len + add + 1))
        return FALSE;
    memcpy(Buffer + Len, text, add + 1);
    Len += add;
    return TRUE;
}

BOOL CSalPathBuf::AppendComponent(const char* name)
{
    if (name == NULL || *name == 0)
        return TRUE;
    while (*name == '\\')
        name++;
    if (!AddBackslash())
        return FALSE;
    return Append(name);
}

BOOL CSalPathBuf::AddBackslash()
{
    if (Len > 0 && Buffer[Len - 1] == '\\')
        return TRUE;
    if (!Reserve(Len + 2))
        return FALSE;
    Buffer[Len++] = '\\';
    Buffer[Len] = 0;
    return TRUE;
}

void CSalPathBuf::StripBackslash()
{
    // keep the root backslash of "C:\" and of "\\server\share\"-style roots
    if (Len > 0 && Buffer[Len - 1] == '\\')
    {
        if (Len == 3 && Buffer[1] == ':')
            return; // "C:\"
        Buffer[--Len] = 0;
    }
}

BOOL CSalPathBuf::CutLastComponent()
{
    if (Len == 0)
        return FALSE;
    int i = Len - 1;
    if (Buffer[i] == '\\')
        i--; // ignore a trailing backslash
    while (i >= 0 && Buffer[i] != '\\')
        i--;
    if (i < 0)
        return FALSE;
    // do not cut above the drive root ("C:\") or leave "\\server" alone
    if (i == 2 && Buffer[1] == ':')
        i++; // keep "C:\"
    else if (Buffer[0] == '\\' && Buffer[1] == '\\')
    {
        // UNC: refuse to cut into "\\server" (share is part of the root)
        int seps = 0;
        for (int j = 2; j < i; j++)
            if (Buffer[j] == '\\')
                seps++;
        if (seps < 1)
            return FALSE; // would cut into \\server
    }
    if (i == 0)
        return FALSE;
    Buffer[i] = 0;
    Len = i;
    return TRUE;
}

//*****************************************************************************
//
// canonicalization helpers (wide, in place)
//

// converts '/' to '\' and collapses ".", ".." and doubled separators
// beyond the root; returns FALSE when ".." would climb above the root
// feature 027 (perf): returns TRUE only when the part past the root is already
// canonical -- no '/', no '.'/'..' component, no doubled separator, no trailing
// separator -- so the (allocating, O(n)) SalCanonicalizePathW pass can be
// skipped. Long-path file operations call SalPathToWExtAlloc several times per
// file; most real paths are already clean, so this removes the dominant
// avoidable per-call cost while leaving output identical.
static BOOL SalPathIsAlreadyCanonicalW(const WCHAR* path, int rootLen)
{
    const WCHAR* in = path + rootLen;
    if (*in == 0)
        return TRUE; // just the root
    while (*in != 0)
    {
        const WCHAR* end = in;
        while (*end != 0 && *end != L'\\' && *end != L'/')
            end++;
        int compLen = (int)(end - in);
        if (compLen == 0)
            return FALSE; // doubled separator or leading separator past root
        if (compLen == 1 && in[0] == L'.')
            return FALSE; // "." component
        if (compLen == 2 && in[0] == L'.' && in[1] == L'.')
            return FALSE; // ".." component
        if (*end == L'/')
            return FALSE; // forward slash needs normalizing
        if (*end == L'\\' && *(end + 1) == 0)
            return FALSE; // trailing separator (the canonicalizer strips it)
        in = *end == 0 ? end : end + 1;
    }
    return TRUE;
}

static BOOL SalCanonicalizePathW(WCHAR* path, int rootLen)
{
    for (WCHAR* s = path; *s != 0; s++)
        if (*s == L'/')
            *s = L'\\';

    WCHAR* out = path + rootLen; // write cursor, never before the root
    WCHAR* in = path + rootLen;
    while (*in != 0)
    {
        // 'in' stands at the start of a component (past root or a '\')
        WCHAR* end = in;
        while (*end != 0 && *end != L'\\')
            end++;
        int compLen = (int)(end - in);
        if (compLen == 0)
        {
            // doubled backslash: skip
        }
        else if (compLen == 1 && in[0] == L'.')
        {
            // "." component: skip
        }
        else if (compLen == 2 && in[0] == L'.' && in[1] == L'.')
        {
            // ".." component: remove the previous component from 'out'
            if (out == path + rootLen)
                return FALSE; // climbing above the root
            out--;            // step onto the separator written before the component
            while (out > path + rootLen && out[-1] != L'\\')
                out--;
        }
        else
        {
            memmove(out, in, compLen * sizeof(WCHAR));
            out += compLen;
            if (*end == L'\\')
                *out++ = L'\\';
        }
        in = *end == 0 ? end : end + 1;
    }
    // strip a trailing separator the loop may have kept (not on the root)
    if (out > path + rootLen && out[-1] == L'\\')
        out--;
    *out = 0;
    return TRUE;
}

//*****************************************************************************
//
// SalPathToWExtAlloc
//

WCHAR* SalPathToWExtAlloc(const char* u8path)
{
    if (u8path == NULL || *u8path == 0)
        return NULL;

    WCHAR* w = SalU8ToWAlloc(u8path, -1);
    if (w == NULL)
        return NULL;

    // already extended: pass through untouched (caller knows what it does)
    if (wcsncmp(w, L"\\\\?\\", 4) == 0)
    {
        if (wcslen(w) > SAL_MAX_PATH_W)
        {
            free(w);
            return NULL;
        }
        return w;
    }

    // resolve relative forms against the current directory (the process
    // current directory cannot exceed MAX_PATH, so this stays small)
    BOOL isDrive = ((w[0] >= L'A' && w[0] <= L'Z') || (w[0] >= L'a' && w[0] <= L'z')) &&
                   w[1] == L':' && (w[2] == L'\\' || w[2] == L'/');
    BOOL isUNC = (w[0] == L'\\' || w[0] == L'/') && (w[1] == L'\\' || w[1] == L'/');
    if (!isDrive && !isUNC)
    {
        WCHAR full[2 * MAX_PATH];
        DWORD res = GetFullPathNameW(w, _countof(full), full, NULL);
        free(w);
        if (res == 0 || res >= _countof(full))
            return NULL;
        w = _wcsdup(full);
        if (w == NULL)
            return NULL;
        isDrive = w[1] == L':';
        isUNC = !isDrive;
    }

    size_t len = wcslen(w);
    int rootLen; // length of the immutable root part ("C:\" / "\\server\share\")
    if (isDrive)
        rootLen = 3;
    else
    {
        // root = \\server\share\ (or as much of it as exists)
        int seps = 0;
        int i = 2;
        while (w[i] != 0)
        {
            if (w[i] == L'\\' || w[i] == L'/')
            {
                seps++;
                if (seps == 2)
                {
                    i++;
                    break;
                }
            }
            i++;
        }
        rootLen = i;
    }
    if ((int)len < rootLen)
    {
        free(w);
        return NULL;
    }

    if (!SalPathIsAlreadyCanonicalW(w, rootLen)) // feature 027: skip the pass for already-clean paths
    {
        if (!SalCanonicalizePathW(w, rootLen))
        {
            free(w);
            return NULL;
        }
        len = wcslen(w);
    }

    // build the extended form
    const WCHAR* prefix = isDrive ? L"\\\\?\\" : L"\\\\?\\UNC\\";
    size_t skip = isDrive ? 0 : 2; // UNC: drop the leading "\\"
    size_t prefixLen = wcslen(prefix);
    size_t totalLen = prefixLen + (len - skip);
    if (totalLen > SAL_MAX_PATH_W)
    {
        free(w);
        return NULL;
    }
    WCHAR* ext = (WCHAR*)malloc((totalLen + 1) * sizeof(WCHAR));
    if (ext == NULL)
    {
        free(w);
        return NULL;
    }
    memcpy(ext, prefix, prefixLen * sizeof(WCHAR));
    memcpy(ext + prefixLen, w + skip, (len - skip + 1) * sizeof(WCHAR));
    free(w);
    return ext;
}

//*****************************************************************************
//
// SalPathFromWAlloc
//

char* SalPathFromWAlloc(const WCHAR* wpath)
{
    if (wpath == NULL)
        return NULL;
    if (wcsncmp(wpath, L"\\\\?\\UNC\\", 8) == 0)
    {
        // \\?\UNC\server\share -> \\server\share
        size_t len = wcslen(wpath) - 8;
        WCHAR* tmp = (WCHAR*)malloc((len + 3) * sizeof(WCHAR));
        if (tmp == NULL)
            return NULL;
        tmp[0] = L'\\';
        tmp[1] = L'\\';
        memcpy(tmp + 2, wpath + 8, (len + 1) * sizeof(WCHAR));
        char* res = SalWToU8Alloc(tmp, -1);
        free(tmp);
        return res;
    }
    if (wcsncmp(wpath, L"\\\\?\\", 4) == 0)
        wpath += 4;
    return SalWToU8Alloc(wpath, -1);
}
