// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include <windows.h>

#include "handles.h"
#include "salunicode.h"
#include "salpath.h"
#include "salfileio.h"

// frees the converted path while keeping the API call's last error
static void SalFreeKeepLastError(WCHAR* w)
{
    DWORD err = GetLastError();
    free(w);
    SetLastError(err);
}

//*****************************************************************************
//
// SalFindFirstFile / SalFindNextFile
//

HANDLE SalFindFirstFile(const char* u8pattern, WIN32_FIND_DATAW* data)
{
    WCHAR* w = SalPathToWExtAlloc(u8pattern);
    if (w == NULL)
    {
        SetLastError(ERROR_INVALID_NAME);
        return INVALID_HANDLE_VALUE;
    }
    HANDLE h = HANDLES_Q(FindFirstFileW(w, data));
    SalFreeKeepLastError(w);
    return h;
}

BOOL SalFindNextFile(HANDLE find, WIN32_FIND_DATAW* data)
{
    return FindNextFileW(find, data);
}

void SalConvertFindDataW(const WIN32_FIND_DATAW* w, WIN32_FIND_DATA* a,
                         char* nameU8, int nameU8Size, char* dosNameU8, int dosNameU8Size)
{
    if (a != NULL)
    {
        a->dwFileAttributes = w->dwFileAttributes;
        a->ftCreationTime = w->ftCreationTime;
        a->ftLastAccessTime = w->ftLastAccessTime;
        a->ftLastWriteTime = w->ftLastWriteTime;
        a->nFileSizeHigh = w->nFileSizeHigh;
        a->nFileSizeLow = w->nFileSizeLow;
        a->dwReserved0 = w->dwReserved0;
        a->dwReserved1 = w->dwReserved1;
        a->cFileName[0] = 0;          // names are NOT kept in the legacy view,
        a->cAlternateFileName[0] = 0; // they live in nameU8/dosNameU8
    }
    if (nameU8 != NULL &&
        SalWToU8(w->cFileName, -1, nameU8, nameU8Size) == 0 &&
        WideCharToMultiByte(CP_UTF8, 0, w->cFileName, -1, nameU8, nameU8Size, NULL, NULL) == 0)
        nameU8[0] = 0;
    if (dosNameU8 != NULL &&
        SalWToU8(w->cAlternateFileName, -1, dosNameU8, dosNameU8Size) == 0 &&
        WideCharToMultiByte(CP_UTF8, 0, w->cAlternateFileName, -1, dosNameU8, dosNameU8Size, NULL, NULL) == 0)
        dosNameU8[0] = 0;
}

//*****************************************************************************
//
// SalCreateFile
//

HANDLE SalCreateFile(const char* u8path, DWORD desiredAccess, DWORD shareMode,
                     LPSECURITY_ATTRIBUTES securityAttributes, DWORD creationDisposition,
                     DWORD flagsAndAttributes, HANDLE templateFile)
{
    WCHAR* w = SalPathToWExtAlloc(u8path);
    if (w == NULL)
    {
        SetLastError(ERROR_INVALID_NAME);
        return INVALID_HANDLE_VALUE;
    }
    HANDLE h = HANDLES_Q(CreateFileW(w, desiredAccess, shareMode, securityAttributes,
                                     creationDisposition, flagsAndAttributes, templateFile));
    SalFreeKeepLastError(w);
    return h;
}

HANDLE SalCreateFileNH(const char* u8path, DWORD desiredAccess, DWORD shareMode,
                       LPSECURITY_ATTRIBUTES securityAttributes, DWORD creationDisposition,
                       DWORD flagsAndAttributes, HANDLE templateFile)
{
    WCHAR* w = SalPathToWExtAlloc(u8path);
    if (w == NULL)
    {
        SetLastError(ERROR_INVALID_NAME);
        return INVALID_HANDLE_VALUE;
    }
    HANDLE h = NOHANDLES(CreateFileW(w, desiredAccess, shareMode, securityAttributes,
                                     creationDisposition, flagsAndAttributes, templateFile));
    SalFreeKeepLastError(w);
    return h;
}

//*****************************************************************************
//
// single-path operations
//

typedef BOOL(WINAPI* FSalPathOpW)(LPCWSTR);

static BOOL SalPathOp(const char* u8path, FSalPathOpW op)
{
    WCHAR* w = SalPathToWExtAlloc(u8path);
    if (w == NULL)
    {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }
    BOOL ret = op(w);
    SalFreeKeepLastError(w);
    return ret;
}

BOOL SalDeleteFile(const char* u8path)
{
    return SalPathOp(u8path, DeleteFileW);
}

BOOL SalRemoveDirectory(const char* u8path)
{
    return SalPathOp(u8path, RemoveDirectoryW);
}

BOOL SalCreateDirectory(const char* u8path, LPSECURITY_ATTRIBUTES securityAttributes)
{
    WCHAR* w = SalPathToWExtAlloc(u8path);
    if (w == NULL)
    {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }
    BOOL ret = CreateDirectoryW(w, securityAttributes);
    SalFreeKeepLastError(w);
    return ret;
}

//*****************************************************************************
//
// two-path operations
//

BOOL SalMoveFile(const char* u8from, const char* u8to)
{
    if (!SalMoveFileEx(u8from, u8to, 0))
    {
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED)
        { // Novell issue: MoveFile fails for files with the read-only attribute
            DWORD attr = SalGetFileAttributes(u8from);
            if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_READONLY))
            {
                SalSetFileAttributes(u8from, FILE_ATTRIBUTE_ARCHIVE);
                if (SalMoveFileEx(u8from, u8to, 0))
                {
                    SalSetFileAttributes(u8to, attr);
                    return TRUE;
                }
                err = GetLastError();
                SalSetFileAttributes(u8from, attr);
            }
            SetLastError(err);
        }
        return FALSE;
    }
    return TRUE;
}

BOOL SalMoveFileEx(const char* u8from, const char* u8to, DWORD flags)
{
    WCHAR* wf = SalPathToWExtAlloc(u8from);
    WCHAR* wt = SalPathToWExtAlloc(u8to);
    BOOL ret = FALSE;
    if (wf == NULL || wt == NULL)
        SetLastError(ERROR_INVALID_NAME);
    else
        ret = flags == 0 ? MoveFileW(wf, wt) : MoveFileExW(wf, wt, flags);
    DWORD err = GetLastError();
    free(wf);
    free(wt);
    SetLastError(err);
    return ret;
}

BOOL SalCopyFile(const char* u8from, const char* u8to, BOOL failIfExists)
{
    WCHAR* wf = SalPathToWExtAlloc(u8from);
    WCHAR* wt = SalPathToWExtAlloc(u8to);
    BOOL ret = FALSE;
    if (wf == NULL || wt == NULL)
        SetLastError(ERROR_INVALID_NAME);
    else
        ret = CopyFileW(wf, wt, failIfExists);
    DWORD err = GetLastError();
    free(wf);
    free(wt);
    SetLastError(err);
    return ret;
}

//*****************************************************************************
//
// attributes
//

DWORD SalGetFileAttributes(const char* u8path)
{
    WCHAR* w = SalPathToWExtAlloc(u8path);
    if (w == NULL)
    {
        SetLastError(ERROR_INVALID_NAME);
        return INVALID_FILE_ATTRIBUTES;
    }
    DWORD ret = GetFileAttributesW(w);
    SalFreeKeepLastError(w);
    return ret;
}

BOOL SalSetFileAttributes(const char* u8path, DWORD attributes)
{
    WCHAR* w = SalPathToWExtAlloc(u8path);
    if (w == NULL)
    {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }
    BOOL ret = SetFileAttributesW(w, attributes);
    SalFreeKeepLastError(w);
    return ret;
}

BOOL SalGetFileAttributesEx(const char* u8path, WIN32_FILE_ATTRIBUTE_DATA* data)
{
    WCHAR* w = SalPathToWExtAlloc(u8path);
    if (w == NULL)
    {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }
    BOOL ret = GetFileAttributesExW(w, GetFileExInfoStandard, data);
    SalFreeKeepLastError(w);
    return ret;
}
