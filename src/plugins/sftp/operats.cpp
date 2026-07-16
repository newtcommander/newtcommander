// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "sftp.h"
#include "sftputils.h"
#include "session.h"
#include "listing.h"
#include "dialogs.h"
#include "logs.h"
#include "operats.h"

#define XFER_BUFSIZE (256 * 1024)

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

static BOOL Utf8ToWide(const char* s, wchar_t* out, int outCount)
{
    return MultiByteToWideChar(CP_UTF8, 0, s, -1, out, outCount) > 0;
}

static BOOL WideToUtf8(const wchar_t* s, char* out, int outCount)
{
    return WideCharToMultiByte(CP_UTF8, 0, s, -1, out, outCount, NULL, NULL) > 0;
}

// builds a Windows long-path-capable wide path from a UTF-8 path
static void MakeLocalWidePath(const char* utf8Path, wchar_t* wide, int wideCount)
{
    Utf8ToWide(utf8Path, wide, wideCount);
}

struct COperationCtx
{
    HWND Parent;
    CSFTPSession* Session;
    volatile BOOL Cancelled;
    char Buffer[XFER_BUFSIZE];

    COperationCtx(HWND parent, CSFTPSession* session)
    {
        Parent = parent;
        Session = session;
        Cancelled = FALSE;
    }
    BOOL CheckCancel()
    {
        if (!Cancelled && SalamanderGeneral->GetSafeWaitWindowClosePressed())
            Cancelled = TRUE;
        return Cancelled;
    }
};

static void WaitText(const char* fmt, const char* arg)
{
    char msg[1024];
    _snprintf_s(msg, _TRUNCATE, fmt, arg);
    // reflect current file in the wait window title area via a fresh window
    SalamanderGeneral->DestroySafeWaitWindow();
    SalamanderGeneral->CreateSafeWaitWindow(msg, LoadStr(IDS_PLUGINNAME), 0, TRUE, SalamanderGeneral->GetMainWindowHWND());
}

// ---------------------------------------------------------------------------
// download
// ---------------------------------------------------------------------------

static BOOL DownloadOneFile(COperationCtx* ctx, const char* remotePath, const char* localPath,
                            unsigned __int64 remoteSize)
{
    wchar_t wlocal[4096];
    MakeLocalWidePath(localPath, wlocal, 4096);

    // resume check (FR-011): existing smaller local file
    unsigned __int64 startOffset = 0;
    BOOL append = FALSE;
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExW(wlocal, GetFileExInfoStandard, &fad))
    {
        unsigned __int64 localSize = ((unsigned __int64)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
        if (localSize > 0 && localSize < remoteSize && (int)localSize >= Config.ResumeMinFileSize)
        {
            int r = ShowResumePrompt(ctx->Parent, PosixBaseName(remotePath));
            if (r == IDYES)
            {
                startOffset = localSize;
                append = TRUE;
            }
            else if (r == IDCANCEL)
                return TRUE; // skip this file
        }
    }

    LIBSSH2_SFTP_HANDLE* h = ctx->Session->OpenRead(remotePath);
    if (h == NULL)
    {
        char msg[1200];
        _snprintf_s(msg, _TRUNCATE, LoadStr(IDS_ERR_DOWNLOAD), remotePath, ctx->Session->GetLastErrorText());
        SalamanderGeneral->SalMessageBox(ctx->Parent, msg, LoadStr(IDS_SFTPERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
        Logs.Append(ctx->Session->GetLogUID(), msg);
        return FALSE;
    }

    HANDLE lf = CreateFileW(wlocal, GENERIC_WRITE, 0, NULL,
                            append ? OPEN_EXISTING : CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (lf == INVALID_HANDLE_VALUE)
    {
        ctx->Session->CloseHandle(h);
        char msg[1200];
        _snprintf_s(msg, _TRUNCATE, LoadStr(IDS_ERR_OPENLOCAL), localPath);
        SalamanderGeneral->SalMessageBox(ctx->Parent, msg, LoadStr(IDS_SFTPERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }
    if (append)
    {
        LARGE_INTEGER li;
        li.QuadPart = startOffset;
        SetFilePointerEx(lf, li, NULL, FILE_BEGIN);
        ctx->Session->SeekWrite(h, startOffset);
    }

    BOOL ok = TRUE;
    for (;;)
    {
        if (ctx->CheckCancel())
        {
            ok = FALSE;
            break;
        }
        __int64 n = ctx->Session->Read(h, ctx->Buffer, XFER_BUFSIZE);
        if (n < 0)
        {
            ok = FALSE;
            SalamanderGeneral->SalMessageBox(ctx->Parent, LoadStr(IDS_ERR_READREMOTE),
                                             LoadStr(IDS_SFTPERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
            break;
        }
        if (n == 0)
            break;
        DWORD written = 0;
        if (!WriteFile(lf, ctx->Buffer, (DWORD)n, &written, NULL) || written != (DWORD)n)
        {
            ok = FALSE;
            SalamanderGeneral->SalMessageBox(ctx->Parent, LoadStr(IDS_ERR_DISKFULL),
                                             LoadStr(IDS_SFTPERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
            break;
        }
    }
    CloseHandle(lf);
    ctx->Session->CloseHandle(h);
    return ok;
}

static BOOL DownloadRecursive(COperationCtx* ctx, const char* remotePath, const char* localPath,
                              unsigned long mode, BOOL isLink, unsigned __int64 size);

static BOOL DownloadDir(COperationCtx* ctx, const char* remoteDir, const char* localDir)
{
    wchar_t wdir[4096];
    MakeLocalWidePath(localDir, wdir, 4096);
    CreateDirectoryW(wdir, NULL); // ignore "already exists"

    TIndirectArray<CSFTPDirEntry> entries(64, 64);
    if (!ctx->Session->ListDir(remoteDir, &entries, &ctx->Cancelled))
    {
        char msg[1200];
        _snprintf_s(msg, _TRUNCATE, LoadStr(IDS_ERR_LISTDIR), remoteDir, ctx->Session->GetLastErrorText());
        SalamanderGeneral->SalMessageBox(ctx->Parent, msg, LoadStr(IDS_SFTPERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    BOOL ok = TRUE;
    for (int i = 0; i < entries.Count && !ctx->CheckCancel(); i++)
    {
        CSFTPDirEntry* e = entries[i];
        char rpath[4096], lpath[4096];
        PosixPathAppend(remoteDir, e->Name, rpath, sizeof(rpath));
        _snprintf_s(lpath, _TRUNCATE, "%s\\%s", localDir, e->Name);
        BOOL isLink = e->HasMode && SFTP_S_ISLNK(e->Mode);
        if (!DownloadRecursive(ctx, rpath, lpath, e->Mode, isLink, e->Size))
            ok = FALSE;
    }
    return ok;
}

static BOOL DownloadRecursive(COperationCtx* ctx, const char* remotePath, const char* localPath,
                              unsigned long mode, BOOL isLink, unsigned __int64 size)
{
    if (ctx->CheckCancel())
        return FALSE;

    if (isLink)
    {
        // resolve the target type: file links are followed (content), directory
        // links are skipped and reported (clarification #4)
        LIBSSH2_SFTP_ATTRIBUTES attrs;
        if (ctx->Session->Stat(remotePath, TRUE, &attrs) && (attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) &&
            SFTP_S_ISDIR(attrs.permissions))
        {
            Logs.AppendFmt(ctx->Session->GetLogUID(), LoadStr(IDS_SYMLINK_DIR_SKIPPED), remotePath);
            return TRUE;
        }
        // treat as a regular file (open follows the link)
        WaitText("Downloading %s ...", PosixBaseName(remotePath));
        return DownloadOneFile(ctx, remotePath, localPath, size);
    }

    if (SFTP_S_ISDIR(mode))
        return DownloadDir(ctx, remotePath, localPath);

    WaitText("Downloading %s ...", PosixBaseName(remotePath));
    return DownloadOneFile(ctx, remotePath, localPath, size);
}

BOOL SFTPDownloadFromPanel(HWND parent, CSFTPSession* session, int panel,
                           const char* remoteDir, const char* targetLocalPath, BOOL move)
{
    COperationCtx ctx(parent, session);
    SalamanderGeneral->CreateSafeWaitWindow(LoadStr(IDS_PLUGINNAME), LoadStr(IDS_PLUGINNAME), 500, TRUE,
                                            SalamanderGeneral->GetMainWindowHWND());

    BOOL ok = TRUE;
    int index = 0;
    BOOL isDir = FALSE;
    const CFileData* fd = SalamanderGeneral->GetPanelSelectedItem(panel, &index, &isDir);
    if (fd == NULL)
        fd = SalamanderGeneral->GetPanelFocusedItem(panel, &isDir);

    // collect names first (selection pointers stay valid within the main thread,
    // but we avoid holding them across the transfer)
    TIndirectArray<CSFTPDirEntry> items(16, 16);
    index = 0;
    for (;;)
    {
        const CFileData* it = SalamanderGeneral->GetPanelSelectedItem(panel, &index, &isDir);
        if (it == NULL)
            break;
        CSFTPDirEntry* e = new CSFTPDirEntry;
        e->Name = _strdup(it->Name);
        CSFTPItemData* d = (CSFTPItemData*)it->PluginData;
        if (d != NULL)
        {
            e->Mode = d->Mode;
            e->HasMode = d->HasMode;
        }
        e->Size = it->Size.Value;
        items.Add(e);
    }
    if (items.Count == 0)
    {
        const CFileData* it = SalamanderGeneral->GetPanelFocusedItem(panel, &isDir);
        if (it != NULL && strcmp(it->Name, "..") != 0)
        {
            CSFTPDirEntry* e = new CSFTPDirEntry;
            e->Name = _strdup(it->Name);
            CSFTPItemData* d = (CSFTPItemData*)it->PluginData;
            if (d != NULL) { e->Mode = d->Mode; e->HasMode = d->HasMode; }
            e->Size = it->Size.Value;
            items.Add(e);
        }
    }

    for (int i = 0; i < items.Count && !ctx.CheckCancel(); i++)
    {
        CSFTPDirEntry* e = items[i];
        char rpath[4096], lpath[4096];
        PosixPathAppend(remoteDir, e->Name, rpath, sizeof(rpath));
        _snprintf_s(lpath, _TRUNCATE, "%s\\%s", targetLocalPath, e->Name);
        BOOL isLink = e->HasMode && SFTP_S_ISLNK(e->Mode);
        if (!DownloadRecursive(&ctx, rpath, lpath, e->Mode, isLink, e->Size))
            ok = FALSE;
        else if (move && ok)
        {
            // delete source after a successful move of a top-level item
            if (e->HasMode && SFTP_S_ISDIR(e->Mode))
                session->Rmdir(rpath);
            else
                session->Unlink(rpath);
        }
    }

    SalamanderGeneral->DestroySafeWaitWindow();
    return ok && !ctx.Cancelled;
}

// ---------------------------------------------------------------------------
// upload
// ---------------------------------------------------------------------------

static BOOL UploadOneFile(COperationCtx* ctx, const char* localPath, const char* remotePath)
{
    wchar_t wlocal[4096];
    MakeLocalWidePath(localPath, wlocal, 4096);
    HANDLE lf = CreateFileW(wlocal, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, NULL);
    if (lf == INVALID_HANDLE_VALUE)
    {
        char msg[1200];
        _snprintf_s(msg, _TRUNCATE, LoadStr(IDS_ERR_OPENLOCAL), localPath);
        SalamanderGeneral->SalMessageBox(ctx->Parent, msg, LoadStr(IDS_SFTPERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    LIBSSH2_SFTP_HANDLE* h = ctx->Session->OpenWrite(remotePath, 0644, FALSE);
    if (h == NULL)
    {
        CloseHandle(lf);
        char msg[1200];
        _snprintf_s(msg, _TRUNCATE, LoadStr(IDS_ERR_UPLOAD), remotePath, ctx->Session->GetLastErrorText());
        SalamanderGeneral->SalMessageBox(ctx->Parent, msg, LoadStr(IDS_SFTPERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    BOOL ok = TRUE;
    for (;;)
    {
        if (ctx->CheckCancel())
        {
            ok = FALSE;
            break;
        }
        DWORD read = 0;
        if (!ReadFile(lf, ctx->Buffer, XFER_BUFSIZE, &read, NULL))
        {
            ok = FALSE;
            break;
        }
        if (read == 0)
            break;
        // libssh2_sftp_write may accept a partial buffer; loop until all written
        int off = 0;
        while (off < (int)read)
        {
            __int64 w = ctx->Session->Write(h, ctx->Buffer + off, (int)read - off);
            if (w < 0)
            {
                ok = FALSE;
                SalamanderGeneral->SalMessageBox(ctx->Parent, LoadStr(IDS_ERR_WRITEREMOTE),
                                                 LoadStr(IDS_SFTPERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
                break;
            }
            off += (int)w;
        }
        if (!ok)
            break;
    }
    ctx->Session->CloseHandle(h);
    CloseHandle(lf);
    return ok;
}

static BOOL UploadDirRecursive(COperationCtx* ctx, const char* localDir, const char* remoteDir)
{
    ctx->Session->Mkdir(remoteDir, 0755); // ignore "already exists"

    wchar_t wpattern[4096];
    char pattern[4096];
    _snprintf_s(pattern, _TRUNCATE, "%s\\*", localDir);
    MakeLocalWidePath(pattern, wpattern, 4096);

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(wpattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return TRUE;

    BOOL ok = TRUE;
    do
    {
        if (ctx->CheckCancel())
        {
            ok = FALSE;
            break;
        }
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
            continue;
        char name[1024];
        WideToUtf8(fd.cFileName, name, sizeof(name));
        char lpath[4096], rpath[4096];
        _snprintf_s(lpath, _TRUNCATE, "%s\\%s", localDir, name);
        PosixPathAppend(remoteDir, name, rpath, sizeof(rpath));
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (!UploadDirRecursive(ctx, lpath, rpath))
                ok = FALSE;
        }
        else
        {
            WaitText("Uploading %s ...", name);
            if (!UploadOneFile(ctx, lpath, rpath))
                ok = FALSE;
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
    return ok;
}

BOOL SFTPUploadToFS(HWND parent, CSFTPSession* session, const char* sourcePath,
                    SalEnumSelection2 next, void* nextParam, const char* targetRemoteDir, BOOL move)
{
    COperationCtx ctx(parent, session);
    SalamanderGeneral->CreateSafeWaitWindow(LoadStr(IDS_PLUGINNAME), LoadStr(IDS_PLUGINNAME), 500, TRUE,
                                            SalamanderGeneral->GetMainWindowHWND());
    BOOL ok = TRUE;

    next(NULL, -1, NULL, NULL, NULL, NULL, NULL, nextParam, NULL); // reset enumeration
    BOOL isDir = FALSE;
    int err = 0;
    const char* name;
    while ((name = next(parent, 0, NULL, &isDir, NULL, NULL, NULL, nextParam, &err)) != NULL)
    {
        if (ctx.CheckCancel())
        {
            ok = FALSE;
            break;
        }
        char lpath[4096], rpath[4096];
        _snprintf_s(lpath, _TRUNCATE, "%s%s", sourcePath, name);
        PosixPathAppend(targetRemoteDir, PosixBaseName(name), rpath, sizeof(rpath));
        if (isDir)
        {
            if (!UploadDirRecursive(&ctx, lpath, rpath))
                ok = FALSE;
        }
        else
        {
            WaitText("Uploading %s ...", name);
            if (!UploadOneFile(&ctx, lpath, rpath))
                ok = FALSE;
        }
    }
    if (err == SALENUM_CANCEL)
        ok = FALSE;

    SalamanderGeneral->DestroySafeWaitWindow();
    return ok && !ctx.Cancelled;
}

// ---------------------------------------------------------------------------
// delete
// ---------------------------------------------------------------------------

static BOOL DeleteRecursive(COperationCtx* ctx, const char* remotePath, unsigned long mode, BOOL isLink)
{
    if (ctx->CheckCancel())
        return FALSE;

    if (!isLink && SFTP_S_ISDIR(mode))
    {
        TIndirectArray<CSFTPDirEntry> entries(64, 64);
        if (ctx->Session->ListDir(remotePath, &entries, &ctx->Cancelled))
        {
            for (int i = 0; i < entries.Count && !ctx->CheckCancel(); i++)
            {
                CSFTPDirEntry* e = entries[i];
                char child[4096];
                PosixPathAppend(remotePath, e->Name, child, sizeof(child));
                BOOL childLink = e->HasMode && SFTP_S_ISLNK(e->Mode);
                DeleteRecursive(ctx, child, e->Mode, childLink);
            }
        }
        if (!ctx->Session->Rmdir(remotePath))
        {
            char msg[1200];
            _snprintf_s(msg, _TRUNCATE, LoadStr(IDS_ERR_RMDIR), remotePath, ctx->Session->GetLastErrorText());
            SalamanderGeneral->SalMessageBox(ctx->Parent, msg, LoadStr(IDS_SFTPERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
            return FALSE;
        }
    }
    else
    {
        if (!ctx->Session->Unlink(remotePath))
        {
            char msg[1200];
            _snprintf_s(msg, _TRUNCATE, LoadStr(IDS_ERR_DELETE), remotePath, ctx->Session->GetLastErrorText());
            SalamanderGeneral->SalMessageBox(ctx->Parent, msg, LoadStr(IDS_SFTPERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
            return FALSE;
        }
    }
    return TRUE;
}

BOOL SFTPDeleteFromPanel(HWND parent, CSFTPSession* session, int panel, const char* remoteDir)
{
    COperationCtx ctx(parent, session);
    SalamanderGeneral->CreateSafeWaitWindow(LoadStr(IDS_PLUGINNAME), LoadStr(IDS_PLUGINNAME), 500, TRUE,
                                            SalamanderGeneral->GetMainWindowHWND());
    BOOL ok = TRUE;

    TIndirectArray<CSFTPDirEntry> items(16, 16);
    int index = 0;
    BOOL isDir = FALSE;
    for (;;)
    {
        const CFileData* it = SalamanderGeneral->GetPanelSelectedItem(panel, &index, &isDir);
        if (it == NULL)
            break;
        CSFTPDirEntry* e = new CSFTPDirEntry;
        e->Name = _strdup(it->Name);
        CSFTPItemData* d = (CSFTPItemData*)it->PluginData;
        if (d != NULL) { e->Mode = d->Mode; e->HasMode = d->HasMode; }
        items.Add(e);
    }
    if (items.Count == 0)
    {
        const CFileData* it = SalamanderGeneral->GetPanelFocusedItem(panel, &isDir);
        if (it != NULL && strcmp(it->Name, "..") != 0)
        {
            CSFTPDirEntry* e = new CSFTPDirEntry;
            e->Name = _strdup(it->Name);
            CSFTPItemData* d = (CSFTPItemData*)it->PluginData;
            if (d != NULL) { e->Mode = d->Mode; e->HasMode = d->HasMode; }
            items.Add(e);
        }
    }

    for (int i = 0; i < items.Count && !ctx.CheckCancel(); i++)
    {
        CSFTPDirEntry* e = items[i];
        char rpath[4096];
        PosixPathAppend(remoteDir, e->Name, rpath, sizeof(rpath));
        WaitText("Deleting %s ...", e->Name);
        BOOL isLink = e->HasMode && SFTP_S_ISLNK(e->Mode);
        if (!DeleteRecursive(&ctx, rpath, e->Mode, isLink))
            ok = FALSE;
    }

    SalamanderGeneral->DestroySafeWaitWindow();
    return ok && !ctx.Cancelled;
}

// ---------------------------------------------------------------------------
// chmod
// ---------------------------------------------------------------------------

static void ChmodRecursive(COperationCtx* ctx, const char* remotePath, unsigned long mode,
                           BOOL isDir, BOOL recurse)
{
    if (ctx->CheckCancel())
        return;
    if (!ctx->Session->Chmod(remotePath, mode))
    {
        char msg[1200];
        _snprintf_s(msg, _TRUNCATE, LoadStr(IDS_ERR_CHMOD), remotePath, ctx->Session->GetLastErrorText());
        Logs.Append(ctx->Session->GetLogUID(), msg);
    }
    if (isDir && recurse)
    {
        TIndirectArray<CSFTPDirEntry> entries(64, 64);
        if (ctx->Session->ListDir(remotePath, &entries, &ctx->Cancelled))
        {
            for (int i = 0; i < entries.Count && !ctx->CheckCancel(); i++)
            {
                CSFTPDirEntry* e = entries[i];
                if (e->HasMode && SFTP_S_ISLNK(e->Mode))
                    continue; // do not chmod through symlinks
                char child[4096];
                PosixPathAppend(remotePath, e->Name, child, sizeof(child));
                ChmodRecursive(ctx, child, mode, SFTP_S_ISDIR(e->Mode), recurse);
            }
        }
    }
}

BOOL SFTPChangeAttrsFromPanel(HWND parent, CSFTPSession* session, int panel, const char* remoteDir)
{
    // seed from the focused item
    BOOL isDir = FALSE;
    const CFileData* focus = SalamanderGeneral->GetPanelFocusedItem(panel, &isDir);
    unsigned long mode = 0644;
    char label[256];
    label[0] = 0;
    if (focus != NULL)
    {
        CSFTPItemData* d = (CSFTPItemData*)focus->PluginData;
        if (d != NULL && d->HasMode)
            mode = d->Mode & 07777;
        lstrcpynA(label, focus->Name, sizeof(label));
    }

    int selFiles = 0, selDirs = 0;
    SalamanderGeneral->GetPanelSelection(panel, &selFiles, &selDirs);
    BOOL multiple = (selFiles + selDirs) > 1;
    if (multiple)
        _snprintf_s(label, _TRUNCATE, "%d file(s), %d dir(s)", selFiles, selDirs);

    BOOL recurse = FALSE, setTime = FALSE;
    __int64 mtime = 0;
    if (!ShowChmodDialog(parent, label, multiple, &mode, &recurse, &setTime, &mtime))
        return FALSE;

    COperationCtx ctx(parent, session);
    SalamanderGeneral->CreateSafeWaitWindow(LoadStr(IDS_PLUGINNAME), LoadStr(IDS_PLUGINNAME), 500, TRUE,
                                            SalamanderGeneral->GetMainWindowHWND());

    // build the working set
    TIndirectArray<CSFTPDirEntry> items(16, 16);
    int index = 0;
    BOOL d2 = FALSE;
    for (;;)
    {
        const CFileData* it = SalamanderGeneral->GetPanelSelectedItem(panel, &index, &d2);
        if (it == NULL)
            break;
        CSFTPDirEntry* e = new CSFTPDirEntry;
        e->Name = _strdup(it->Name);
        CSFTPItemData* dd = (CSFTPItemData*)it->PluginData;
        if (dd != NULL) { e->Mode = dd->Mode; e->HasMode = dd->HasMode; }
        items.Add(e);
    }
    if (items.Count == 0 && focus != NULL && strcmp(focus->Name, "..") != 0)
    {
        CSFTPDirEntry* e = new CSFTPDirEntry;
        e->Name = _strdup(focus->Name);
        CSFTPItemData* dd = (CSFTPItemData*)focus->PluginData;
        if (dd != NULL) { e->Mode = dd->Mode; e->HasMode = dd->HasMode; }
        items.Add(e);
    }

    for (int i = 0; i < items.Count && !ctx.CheckCancel(); i++)
    {
        CSFTPDirEntry* e = items[i];
        char rpath[4096];
        PosixPathAppend(remoteDir, e->Name, rpath, sizeof(rpath));
        BOOL itemIsDir = e->HasMode && SFTP_S_ISDIR(e->Mode);
        ChmodRecursive(&ctx, rpath, mode, itemIsDir, recurse);
        if (setTime)
            session->SetMTime(rpath, mtime);
    }

    SalamanderGeneral->DestroySafeWaitWindow();
    return TRUE;
}
