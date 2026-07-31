// SPDX-FileCopyrightText: 2026 Pavel Stupka
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <libssh2.h>
#include <libssh2_sftp.h>

class CSFTPHostKeyList;

// one entry returned by a directory listing
struct CSFTPDirEntry
{
    char* Name;              // heap UTF-8 name (single path component)
    unsigned long Mode;      // permissions incl. type bits (0 if unknown)
    unsigned __int64 Size;
    unsigned long Uid;
    unsigned long Gid;
    __int64 Mtime;           // seconds since Unix epoch
    char* Owner;             // heap owner name or NULL (numeric fallback)
    char* Group;             // heap group name or NULL
    BOOL HasMode;
    BOOL HasSize;
    BOOL HasMtime;

    CSFTPDirEntry();
    ~CSFTPDirEntry();
};

// result of the host-key verification step
enum CSFTPHostKeyResult
{
    hkOk = 0,        // key trusted (known match or user accepted)
    hkAborted = 1,   // user aborted / declined
    hkError = 2,     // could not obtain key
};

// how the connection ended up (for error reporting, FR-024)
enum CSFTPConnectResult
{
    crOk = 0,
    crResolveFailed,
    crConnectFailed,
    crHandshakeFailed,
    crHostKeyRejected,
    crAuthFailed,
    crAuthPassword,
    crAuthKey,
    crKeyUnlock,
    crKeyFileMissing,
    crNoAuthMethod,
    crSFTPInitFailed,
    crCancelled,
    crTimeout,
};

// parameters needed to (re)establish a connection
struct CSFTPConnectParams
{
    char Host[256];
    int Port;
    char User[256];
    int AuthMethod;       // CSFTPAuthMethod
    char Password[512];   // decrypted, in-memory only
    char KeyFile[MAX_PATH];
    char Passphrase[512]; // decrypted, in-memory only
    BOOL UseCompression;
    int ConnectTimeoutSec;
    int OperationTimeoutSec;
    int KeepAliveSec;

    CSFTPConnectParams() { memset(this, 0, sizeof(*this)); Port = 22; }
    void WipeSecrets()
    {
        SecureZeroMemory(Password, sizeof(Password));
        SecureZeroMemory(Passphrase, sizeof(Passphrase));
    }
};

// ****************************************************************************
// CSFTPSession - wraps one SSH transport + SFTP subsystem (blocking mode).
// One session belongs to one panel FS or one operation worker; not shared
// across threads.
// ****************************************************************************

class CSFTPSession
{
protected:
    SOCKET Socket;
    LIBSSH2_SESSION* Ssh;
    LIBSSH2_SFTP* Sftp;
    BOOL Connected;

    CSFTPConnectParams Params;
    int LogUID; // session log id, -1 = none

    char LastErrorText[512]; // human-readable last error

public:
    CSFTPSession();
    ~CSFTPSession();

    void SetLogUID(int uid) { LogUID = uid; }
    int GetLogUID() const { return LogUID; }

    BOOL IsConnected() const { return Connected; }
    const char* GetLastErrorText() const { return LastErrorText; }

    // Establishes the transport, verifies the host key (via 'trustStore' and,
    // when interaction is needed, prompting through 'parent'), authenticates,
    // and starts the SFTP subsystem. 'result' receives a precise outcome.
    // Returns TRUE only on a fully usable SFTP session.
    BOOL Connect(HWND parent, const CSFTPConnectParams* params,
                 CSFTPHostKeyList* trustStore, CSFTPConnectResult* result);

    // Re-establishes a dropped session using the stored params (FR-023).
    BOOL Reconnect(HWND parent, CSFTPHostKeyList* trustStore, CSFTPConnectResult* result);

    void Disconnect();

    // Resolves the server's home / a relative path to an absolute one.
    BOOL RealPath(const char* path, char* absPath, int absPathSize);

    // Lists 'path'. Appends heap-owned CSFTPDirEntry* to 'entries'. 'cancel'
    // (if not NULL) is polled to allow the caller to abort a huge listing.
    BOOL ListDir(const char* path, TIndirectArray<CSFTPDirEntry>* entries,
                 volatile BOOL* cancel);

    // stat/lstat: fills a single entry's attributes.
    BOOL Stat(const char* path, BOOL followSymlink, LIBSSH2_SFTP_ATTRIBUTES* attrs);
    BOOL ReadLink(const char* path, char* target, int targetSize);

    BOOL Mkdir(const char* path, long mode);
    BOOL Rmdir(const char* path);
    BOOL Unlink(const char* path);
    BOOL Rename(const char* from, const char* to);
    BOOL Symlink(const char* target, const char* linkPath);
    BOOL Chmod(const char* path, unsigned long mode);
    // feature 018: change owner (uid) and/or group (gid); an unset field is kept
    BOOL Chown(const char* path, unsigned long uid, unsigned long gid, BOOL setUid, BOOL setGid);
    BOOL SetMTime(const char* path, __int64 mtime);

    // File transfer primitives (blocking). Handles are opaque.
    LIBSSH2_SFTP_HANDLE* OpenRead(const char* path);
    LIBSSH2_SFTP_HANDLE* OpenWrite(const char* path, long mode, BOOL append);
    void SeekWrite(LIBSSH2_SFTP_HANDLE* h, unsigned __int64 offset);
    __int64 Read(LIBSSH2_SFTP_HANDLE* h, char* buf, int len);
    __int64 Write(LIBSSH2_SFTP_HANDLE* h, const char* buf, int len);
    void CloseHandle(LIBSSH2_SFTP_HANDLE* h);

    // Sends a keepalive if due; returns seconds until the next one is needed.
    void Keepalive(int* secondsToNext);

protected:
    BOOL OpenSocket(HWND parent, CSFTPConnectResult* result);
    BOOL VerifyHostKey(HWND parent, CSFTPHostKeyList* trustStore, CSFTPConnectResult* result);
    BOOL Authenticate(HWND parent, CSFTPConnectResult* result);
    void SetLastErrorFromSsh(const char* prefix);
    void Log(const char* text);
    void LogFmt(const char* fmt, ...);
};

// process-wide libssh2 init / shutdown (called from the plugin entry/release)
BOOL InitSSH();
void ReleaseSSH();
