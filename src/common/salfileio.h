// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

//*****************************************************************************
//
// salfileio.h
//
// Central W-API file-I/O layer (feature 004-long-paths-unicode, R2).
//
// Every function takes UTF-8 display-form paths (no \\?\ prefix),
// converts to UTF-16, normalizes to extended-length form and calls
// the W API - so long paths and full Unicode names work regardless
// of the LongPathsEnabled registry state. GetLastError() is preserved
// from the underlying API call; unconvertible paths fail with
// ERROR_INVALID_NAME.
//
// Handle-returning wrappers register with the HANDLES tracker.
// Close find handles with HANDLES(FindClose(h)), file handles with
// HANDLES(CloseHandle(h)) as usual.
//

// enumeration: u8pattern is a directory pattern like "C:\\dir\\*";
// names come back wide in data->cFileName (transcode via SalWToU8)
HANDLE SalFindFirstFile(const char* u8pattern, WIN32_FIND_DATAW* data);
// plain ::FindNextFileW re-export for call-site symmetry
BOOL SalFindNextFile(HANDLE find, WIN32_FIND_DATAW* data);

HANDLE SalCreateFile(const char* u8path, DWORD desiredAccess, DWORD shareMode,
                     LPSECURITY_ATTRIBUTES securityAttributes, DWORD creationDisposition,
                     DWORD flagsAndAttributes, HANDLE templateFile);

// NOHANDLES variant for call sites that register the handle with
// HANDLES_ADD later (or never) - no tracker registration happens here
HANDLE SalCreateFileNH(const char* u8path, DWORD desiredAccess, DWORD shareMode,
                       LPSECURITY_ATTRIBUTES securityAttributes, DWORD creationDisposition,
                       DWORD flagsAndAttributes, HANDLE templateFile);

BOOL SalDeleteFile(const char* u8path);
BOOL SalRemoveDirectory(const char* u8path);
BOOL SalCreateDirectory(const char* u8path, LPSECURITY_ATTRIBUTES securityAttributes);
BOOL SalMoveFile(const char* u8from, const char* u8to);
BOOL SalMoveFileEx(const char* u8from, const char* u8to, DWORD flags);
BOOL SalCopyFile(const char* u8from, const char* u8to, BOOL failIfExists);

DWORD SalGetFileAttributes(const char* u8path);
BOOL SalSetFileAttributes(const char* u8path, DWORD attributes);
BOOL SalGetFileAttributesEx(const char* u8path, WIN32_FILE_ATTRIBUTE_DATA* data);
