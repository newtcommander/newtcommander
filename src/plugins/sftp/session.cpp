// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "sftp.h"
#include "sftputils.h"
#include "logs.h"
#include "hostkeys.h"
#include "dialogs.h"
#include "session.h"

// ---------------------------------------------------------------------------
// process-wide libssh2 lifecycle
// ---------------------------------------------------------------------------

static BOOL SSHInitialized = FALSE;
static BOOL WinsockInitialized = FALSE;

BOOL InitSSH()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return FALSE;
    WinsockInitialized = TRUE;

    if (libssh2_init(0) != 0)
    {
        WSACleanup();
        WinsockInitialized = FALSE;
        return FALSE;
    }
    SSHInitialized = TRUE;
    return TRUE;
}

void ReleaseSSH()
{
    if (SSHInitialized)
    {
        libssh2_exit();
        SSHInitialized = FALSE;
    }
    if (WinsockInitialized)
    {
        WSACleanup();
        WinsockInitialized = FALSE;
    }
}

// ---------------------------------------------------------------------------
// CSFTPDirEntry
// ---------------------------------------------------------------------------

CSFTPDirEntry::CSFTPDirEntry()
{
    Name = NULL;
    Mode = 0;
    Size = 0;
    Uid = Gid = 0;
    Mtime = 0;
    Owner = NULL;
    Group = NULL;
    HasMode = HasSize = HasMtime = FALSE;
}

CSFTPDirEntry::~CSFTPDirEntry()
{
    if (Name != NULL)
        free(Name);
    if (Owner != NULL)
        free(Owner);
    if (Group != NULL)
        free(Group);
}

// ---------------------------------------------------------------------------
// keyboard-interactive single-prompt password callback (clarification #1)
// ---------------------------------------------------------------------------

static __declspec(thread) const char* g_kbdPassword = NULL;

extern "C" void kbd_callback(const char* name, int name_len,
                             const char* instruction, int instruction_len,
                             int num_prompts,
                             const LIBSSH2_USERAUTH_KBDINT_PROMPT* prompts,
                             LIBSSH2_USERAUTH_KBDINT_RESPONSE* responses,
                             void** abstract)
{
    (void)name; (void)name_len; (void)instruction; (void)instruction_len; (void)prompts; (void)abstract;
    // answer every prompt with the stored password; the common case is a single
    // "Password:" prompt on servers that only offer keyboard-interactive
    for (int i = 0; i < num_prompts; i++)
    {
        const char* pwd = g_kbdPassword != NULL ? g_kbdPassword : "";
        size_t len = strlen(pwd);
        responses[i].text = (char*)malloc(len + 1);
        if (responses[i].text != NULL)
        {
            memcpy(responses[i].text, pwd, len + 1);
            responses[i].length = (unsigned int)len;
        }
        else
            responses[i].length = 0;
    }
}

// ---------------------------------------------------------------------------
// CSFTPSession
// ---------------------------------------------------------------------------

CSFTPSession::CSFTPSession()
{
    Socket = INVALID_SOCKET;
    Ssh = NULL;
    Sftp = NULL;
    Connected = FALSE;
    LogUID = -1;
    LastErrorText[0] = 0;
}

CSFTPSession::~CSFTPSession()
{
    Disconnect();
    Params.WipeSecrets();
}

void CSFTPSession::Log(const char* text)
{
    if (LogUID >= 0)
        Logs.Append(LogUID, text);
}

void CSFTPSession::LogFmt(const char* fmt, ...)
{
    if (LogUID < 0)
        return;
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    _vsnprintf_s(buf, _TRUNCATE, fmt, args);
    va_end(args);
    Log(buf);
}

void CSFTPSession::SetLastErrorFromSsh(const char* prefix)
{
    char* msg = NULL;
    int len = 0;
    if (Ssh != NULL)
        libssh2_session_last_error(Ssh, &msg, &len, 0);
    _snprintf_s(LastErrorText, _TRUNCATE, "%s%s%s", prefix != NULL ? prefix : "",
                (prefix != NULL && prefix[0]) ? ": " : "",
                msg != NULL ? msg : "unknown error");
}

BOOL CSFTPSession::OpenSocket(HWND parent, CSFTPConnectResult* result)
{
    (void)parent;
    char portStr[16];
    _snprintf_s(portStr, _TRUNCATE, "%d", Params.Port);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* ai = NULL;
    if (getaddrinfo(Params.Host, portStr, &hints, &ai) != 0 || ai == NULL)
    {
        _snprintf_s(LastErrorText, _TRUNCATE, LoadStr(IDS_ERR_RESOLVE), Params.Host);
        if (result != NULL)
            *result = crResolveFailed;
        return FALSE;
    }

    SOCKET s = INVALID_SOCKET;
    for (struct addrinfo* p = ai; p != NULL; p = p->ai_next)
    {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s == INVALID_SOCKET)
            continue;

        // apply a connect timeout via non-blocking connect + select
        u_long nonblock = 1;
        ioctlsocket(s, FIONBIO, &nonblock);
        int cr = connect(s, p->ai_addr, (int)p->ai_addrlen);
        if (cr == 0)
        {
            // connected immediately
        }
        else if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(s, &wfds);
            struct timeval tv;
            tv.tv_sec = Params.ConnectTimeoutSec > 0 ? Params.ConnectTimeoutSec : 20;
            tv.tv_usec = 0;
            int sel = select(0, NULL, &wfds, NULL, &tv);
            if (sel <= 0)
            {
                closesocket(s);
                s = INVALID_SOCKET;
                continue;
            }
            int soErr = 0;
            int soLen = sizeof(soErr);
            getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&soErr, &soLen);
            if (soErr != 0)
            {
                closesocket(s);
                s = INVALID_SOCKET;
                continue;
            }
        }
        else
        {
            closesocket(s);
            s = INVALID_SOCKET;
            continue;
        }
        // back to blocking mode for libssh2 blocking API
        nonblock = 0;
        ioctlsocket(s, FIONBIO, &nonblock);
        break;
    }
    freeaddrinfo(ai);

    if (s == INVALID_SOCKET)
    {
        _snprintf_s(LastErrorText, _TRUNCATE, LoadStr(IDS_ERR_CONNECT), Params.Host, Params.Port);
        if (result != NULL)
            *result = crConnectFailed;
        return FALSE;
    }
    Socket = s;
    return TRUE;
}

BOOL CSFTPSession::VerifyHostKey(HWND parent, CSFTPHostKeyList* trustStore, CSFTPConnectResult* result)
{
    size_t keyLen = 0;
    int keyType = 0;
    const char* keyBlob = libssh2_session_hostkey(Ssh, &keyLen, &keyType);
    if (keyBlob == NULL || keyLen == 0)
    {
        lstrcpynA(LastErrorText, "Server did not present a host key.", sizeof(LastErrorText));
        if (result != NULL)
            *result = crHostKeyRejected;
        return FALSE;
    }

    const char* typeName = "ssh-unknown";
    switch (keyType)
    {
    case LIBSSH2_HOSTKEY_TYPE_RSA: typeName = "ssh-rsa"; break;
    case LIBSSH2_HOSTKEY_TYPE_DSS: typeName = "ssh-dss"; break;
    case LIBSSH2_HOSTKEY_TYPE_ECDSA_256: typeName = "ecdsa-sha2-nistp256"; break;
    case LIBSSH2_HOSTKEY_TYPE_ECDSA_384: typeName = "ecdsa-sha2-nistp384"; break;
    case LIBSSH2_HOSTKEY_TYPE_ECDSA_521: typeName = "ecdsa-sha2-nistp521"; break;
    case LIBSSH2_HOSTKEY_TYPE_ED25519: typeName = "ssh-ed25519"; break;
    }

    char* keyB64 = Base64Encode((const unsigned char*)keyBlob, (int)keyLen);
    if (keyB64 == NULL)
    {
        if (result != NULL)
            *result = crHostKeyRejected;
        return FALSE;
    }

    char fingerprint[128];
    const char* sha = libssh2_hostkey_hash(Ssh, LIBSSH2_HOSTKEY_HASH_SHA256);
    if (sha != NULL)
        FormatSHA256Fingerprint((const unsigned char*)sha, fingerprint, sizeof(fingerprint));
    else
        lstrcpynA(fingerprint, "SHA256:?", sizeof(fingerprint));

    char storedFp[128];
    storedFp[0] = 0;
    CSFTPHostKeyMatch m = trustStore->Match(Params.Host, Params.Port, keyB64, storedFp, sizeof(storedFp));

    BOOL ok = FALSE;
    if (m == hkmMatch)
    {
        LogFmt("Host key verified (%s %s).", typeName, fingerprint);
        ok = TRUE;
    }
    else
    {
        int choice = ShowHostKeyDialog(parent, m == hkmMismatch, Params.Host, Params.Port,
                                       typeName, fingerprint, m == hkmMismatch ? storedFp : NULL);
        if (choice == IDB_HOSTKEY_TRUST)
        {
            trustStore->Trust(Params.Host, Params.Port, typeName, keyB64, fingerprint);
            LogFmt("Host key %s (%s) accepted and stored.", typeName, fingerprint);
            ok = TRUE;
        }
        else if (choice == IDB_HOSTKEY_ONCE)
        {
            LogFmt("Host key %s (%s) accepted for this session only.", typeName, fingerprint);
            ok = TRUE;
        }
        else
        {
            Log("Host key rejected by user - connection aborted.");
            lstrcpynA(LastErrorText, LoadStr(IDS_ERR_HOSTKEYDECLINED), sizeof(LastErrorText));
            if (result != NULL)
                *result = crHostKeyRejected;
        }
    }
    free(keyB64);
    return ok;
}

BOOL CSFTPSession::Authenticate(HWND parent, CSFTPConnectResult* result)
{
    // discover which methods the server offers
    char* authList = libssh2_userauth_list(Ssh, Params.User, (unsigned int)strlen(Params.User));
    BOOL hasPassword = authList != NULL && strstr(authList, "password") != NULL;
    BOOL hasKbd = authList != NULL && strstr(authList, "keyboard-interactive") != NULL;
    BOOL hasPublicKey = authList != NULL && strstr(authList, "publickey") != NULL;

    if (Params.AuthMethod == saPrivateKey)
    {
        if (!hasPublicKey && authList != NULL)
        {
            lstrcpynA(LastErrorText, LoadStr(IDS_ERR_NOAUTHMETHOD), sizeof(LastErrorText));
            if (result != NULL)
                *result = crNoAuthMethod;
            return FALSE;
        }
        if (GetFileAttributesA(Params.KeyFile) == INVALID_FILE_ATTRIBUTES)
        {
            _snprintf_s(LastErrorText, _TRUNCATE, LoadStr(IDS_ERR_KEYFILEMISSING), Params.KeyFile);
            if (result != NULL)
                *result = crKeyFileMissing;
            return FALSE;
        }
        // Public key auth from file: libssh2 derives the public key from the
        // private key when the pubkey path is NULL.
        int rc = libssh2_userauth_publickey_fromfile_ex(
            Ssh, Params.User, (unsigned int)strlen(Params.User), NULL, Params.KeyFile,
            Params.Passphrase[0] ? Params.Passphrase : NULL);
        if (rc == 0)
        {
            Log("Authenticated with private key.");
            return TRUE;
        }
        // distinguish "key could not be unlocked" from "server rejected key"
        char* msg = NULL;
        int len = 0;
        libssh2_session_last_error(Ssh, &msg, &len, 0);
        if (rc == LIBSSH2_ERROR_FILE ||
            (msg != NULL && (strstr(msg, "passphrase") != NULL || strstr(msg, "decrypt") != NULL ||
                             strstr(msg, "unable to") != NULL)))
        {
            lstrcpynA(LastErrorText, LoadStr(IDS_ERR_KEYUNLOCK), sizeof(LastErrorText));
            if (result != NULL)
                *result = crKeyUnlock;
        }
        else
        {
            lstrcpynA(LastErrorText, LoadStr(IDS_ERR_AUTHKEY), sizeof(LastErrorText));
            if (result != NULL)
                *result = crAuthKey;
        }
        return FALSE;
    }

    // password method: try plain password, then single-prompt keyboard-interactive
    if (hasPassword || authList == NULL)
    {
        int rc = libssh2_userauth_password_ex(Ssh, Params.User, (unsigned int)strlen(Params.User),
                                              Params.Password, (unsigned int)strlen(Params.Password), NULL);
        if (rc == 0)
        {
            Log("Authenticated with password.");
            return TRUE;
        }
    }
    if (hasKbd)
    {
        g_kbdPassword = Params.Password;
        int rc = libssh2_userauth_keyboard_interactive_ex(Ssh, Params.User,
                                                          (unsigned int)strlen(Params.User), &kbd_callback);
        g_kbdPassword = NULL;
        if (rc == 0)
        {
            Log("Authenticated with keyboard-interactive.");
            return TRUE;
        }
    }

    lstrcpynA(LastErrorText, LoadStr(IDS_ERR_AUTHPASSWORD), sizeof(LastErrorText));
    if (result != NULL)
        *result = crAuthPassword;
    return FALSE;
}

BOOL CSFTPSession::Connect(HWND parent, const CSFTPConnectParams* params,
                           CSFTPHostKeyList* trustStore, CSFTPConnectResult* result)
{
    if (result != NULL)
        *result = crOk;

    if (params != &Params)
        Params = *params;

    LogFmt("Connecting to %s:%d as %s ...", Params.Host, Params.Port, Params.User);

    if (!OpenSocket(parent, result))
    {
        Log(LastErrorText);
        return FALSE;
    }

    Ssh = libssh2_session_init_ex(NULL, NULL, NULL, this);
    if (Ssh == NULL)
    {
        lstrcpynA(LastErrorText, "Cannot create SSH session.", sizeof(LastErrorText));
        Disconnect();
        return FALSE;
    }
    libssh2_session_set_blocking(Ssh, 1);
    long timeoutMs = (Params.OperationTimeoutSec > 0 ? Params.OperationTimeoutSec : 30) * 1000L;
    libssh2_session_set_timeout(Ssh, timeoutMs);

    if (libssh2_session_handshake(Ssh, Socket) != 0)
    {
        SetLastErrorFromSsh(LoadStr(IDS_ERR_HANDSHAKE));
        Log(LastErrorText);
        if (result != NULL)
            *result = crHandshakeFailed;
        Disconnect();
        return FALSE;
    }

    if (!VerifyHostKey(parent, trustStore, result))
    {
        Log(LastErrorText);
        Disconnect();
        return FALSE;
    }

    if (!Authenticate(parent, result))
    {
        Log(LastErrorText);
        Disconnect();
        return FALSE;
    }

    Sftp = libssh2_sftp_init(Ssh);
    if (Sftp == NULL)
    {
        SetLastErrorFromSsh(LoadStr(IDS_ERR_SFTPINIT));
        Log(LastErrorText);
        if (result != NULL)
            *result = crSFTPInitFailed;
        Disconnect();
        return FALSE;
    }

    if (Params.KeepAliveSec > 0)
        libssh2_keepalive_config(Ssh, 1, Params.KeepAliveSec);

    Connected = TRUE;
    Log("SFTP session established.");
    return TRUE;
}

BOOL CSFTPSession::Reconnect(HWND parent, CSFTPHostKeyList* trustStore, CSFTPConnectResult* result)
{
    Disconnect();
    LogFmt(LoadStr(IDS_RECONNECTING), Params.Host);
    return Connect(parent, &Params, trustStore, result);
}

void CSFTPSession::Disconnect()
{
    if (Sftp != NULL)
    {
        libssh2_sftp_shutdown(Sftp);
        Sftp = NULL;
    }
    if (Ssh != NULL)
    {
        libssh2_session_disconnect(Ssh, "Bye");
        libssh2_session_free(Ssh);
        Ssh = NULL;
    }
    if (Socket != INVALID_SOCKET)
    {
        closesocket(Socket);
        Socket = INVALID_SOCKET;
    }
    Connected = FALSE;
}

BOOL CSFTPSession::RealPath(const char* path, char* absPath, int absPathSize)
{
    if (Sftp == NULL)
        return FALSE;
    int rc = libssh2_sftp_realpath(Sftp, path, absPath, absPathSize);
    if (rc <= 0)
    {
        // fall back to the requested path
        lstrcpynA(absPath, (path != NULL && path[0]) ? path : "/", absPathSize);
        return path != NULL && path[0] != 0;
    }
    if (rc < absPathSize)
        absPath[rc] = 0;
    return TRUE;
}

BOOL CSFTPSession::ListDir(const char* path, TIndirectArray<CSFTPDirEntry>* entries, volatile BOOL* cancel)
{
    if (Sftp == NULL)
        return FALSE;

    LIBSSH2_SFTP_HANDLE* dir = libssh2_sftp_opendir(Sftp, path);
    if (dir == NULL)
    {
        SetLastErrorFromSsh("opendir");
        return FALSE;
    }

    char nameBuf[2048];
    char longEntry[2048];
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    for (;;)
    {
        if (cancel != NULL && *cancel)
            break;
        memset(&attrs, 0, sizeof(attrs));
        int rc = libssh2_sftp_readdir_ex(dir, nameBuf, sizeof(nameBuf) - 1, longEntry, sizeof(longEntry) - 1, &attrs);
        if (rc <= 0)
            break;
        nameBuf[rc] = 0;

        if (IsDotOrDotDot(nameBuf))
            continue;

        CSFTPDirEntry* e = new CSFTPDirEntry;
        if (e == NULL)
            break;
        e->Name = _strdup(nameBuf);
        if (attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS)
        {
            e->Mode = attrs.permissions;
            e->HasMode = TRUE;
        }
        if (attrs.flags & LIBSSH2_SFTP_ATTR_SIZE)
        {
            e->Size = attrs.filesize;
            e->HasSize = TRUE;
        }
        if (attrs.flags & LIBSSH2_SFTP_ATTR_UIDGID)
        {
            e->Uid = attrs.uid;
            e->Gid = attrs.gid;
        }
        if (attrs.flags & LIBSSH2_SFTP_ATTR_ACMODTIME)
        {
            e->Mtime = (__int64)attrs.mtime;
            e->HasMtime = TRUE;
        }
        // parse owner/group text from the long listing entry (v3 style:
        // "perms links owner group size date name")
        if (longEntry[0] != 0)
        {
            char* tok = longEntry;
            char* fields[8];
            int nf = 0;
            while (nf < 8)
            {
                while (*tok == ' ' || *tok == '\t')
                    tok++;
                if (*tok == 0)
                    break;
                fields[nf++] = tok;
                while (*tok != 0 && *tok != ' ' && *tok != '\t')
                    tok++;
                if (*tok != 0)
                    *tok++ = 0;
            }
            if (nf >= 4)
            {
                e->Owner = _strdup(fields[2]);
                e->Group = _strdup(fields[3]);
            }
        }
        entries->Add(e);
        if (!entries->IsGood())
        {
            entries->ResetState();
            delete e;
            break;
        }
    }
    libssh2_sftp_closedir(dir);
    return TRUE;
}

BOOL CSFTPSession::Stat(const char* path, BOOL followSymlink, LIBSSH2_SFTP_ATTRIBUTES* attrs)
{
    if (Sftp == NULL)
        return FALSE;
    memset(attrs, 0, sizeof(*attrs));
    int rc = libssh2_sftp_stat_ex(Sftp, path, (unsigned int)strlen(path),
                                  followSymlink ? LIBSSH2_SFTP_STAT : LIBSSH2_SFTP_LSTAT, attrs);
    return rc == 0;
}

BOOL CSFTPSession::ReadLink(const char* path, char* target, int targetSize)
{
    if (Sftp == NULL)
        return FALSE;
    int rc = libssh2_sftp_symlink_ex(Sftp, path, (unsigned int)strlen(path), target, targetSize - 1,
                                     LIBSSH2_SFTP_READLINK);
    if (rc <= 0)
        return FALSE;
    target[rc] = 0;
    return TRUE;
}

BOOL CSFTPSession::Mkdir(const char* path, long mode)
{
    if (Sftp == NULL)
        return FALSE;
    return libssh2_sftp_mkdir_ex(Sftp, path, (unsigned int)strlen(path), mode) == 0;
}

BOOL CSFTPSession::Rmdir(const char* path)
{
    if (Sftp == NULL)
        return FALSE;
    return libssh2_sftp_rmdir_ex(Sftp, path, (unsigned int)strlen(path)) == 0;
}

BOOL CSFTPSession::Unlink(const char* path)
{
    if (Sftp == NULL)
        return FALSE;
    return libssh2_sftp_unlink_ex(Sftp, path, (unsigned int)strlen(path)) == 0;
}

BOOL CSFTPSession::Rename(const char* from, const char* to)
{
    if (Sftp == NULL)
        return FALSE;
    int rc = libssh2_sftp_rename_ex(Sftp, from, (unsigned int)strlen(from), to, (unsigned int)strlen(to),
                                    LIBSSH2_SFTP_RENAME_OVERWRITE | LIBSSH2_SFTP_RENAME_ATOMIC |
                                        LIBSSH2_SFTP_RENAME_NATIVE);
    return rc == 0;
}

BOOL CSFTPSession::Symlink(const char* target, const char* linkPath)
{
    if (Sftp == NULL)
        return FALSE;
    // libssh2_sftp_symlink(sftp, orig, linkpath): create 'linkpath' pointing to 'orig'
    int rc = libssh2_sftp_symlink_ex(Sftp, target, (unsigned int)strlen(target),
                                     (char*)linkPath, (unsigned int)strlen(linkPath), LIBSSH2_SFTP_SYMLINK);
    return rc == 0;
}

BOOL CSFTPSession::Chmod(const char* path, unsigned long mode)
{
    if (Sftp == NULL)
        return FALSE;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    memset(&attrs, 0, sizeof(attrs));
    attrs.flags = LIBSSH2_SFTP_ATTR_PERMISSIONS;
    attrs.permissions = mode;
    return libssh2_sftp_stat_ex(Sftp, path, (unsigned int)strlen(path), LIBSSH2_SFTP_SETSTAT, &attrs) == 0;
}

BOOL CSFTPSession::SetMTime(const char* path, __int64 mtime)
{
    if (Sftp == NULL)
        return FALSE;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    memset(&attrs, 0, sizeof(attrs));
    attrs.flags = LIBSSH2_SFTP_ATTR_ACMODTIME;
    attrs.mtime = (unsigned long)mtime;
    attrs.atime = (unsigned long)mtime;
    return libssh2_sftp_stat_ex(Sftp, path, (unsigned int)strlen(path), LIBSSH2_SFTP_SETSTAT, &attrs) == 0;
}

LIBSSH2_SFTP_HANDLE* CSFTPSession::OpenRead(const char* path)
{
    if (Sftp == NULL)
        return NULL;
    return libssh2_sftp_open_ex(Sftp, path, (unsigned int)strlen(path), LIBSSH2_FXF_READ, 0, LIBSSH2_SFTP_OPENFILE);
}

LIBSSH2_SFTP_HANDLE* CSFTPSession::OpenWrite(const char* path, long mode, BOOL append)
{
    if (Sftp == NULL)
        return NULL;
    unsigned long flags = LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT;
    if (append)
        flags |= LIBSSH2_FXF_APPEND;
    else
        flags |= LIBSSH2_FXF_TRUNC;
    return libssh2_sftp_open_ex(Sftp, path, (unsigned int)strlen(path), flags, mode, LIBSSH2_SFTP_OPENFILE);
}

void CSFTPSession::SeekWrite(LIBSSH2_SFTP_HANDLE* h, unsigned __int64 offset)
{
    if (h != NULL)
        libssh2_sftp_seek64(h, offset);
}

__int64 CSFTPSession::Read(LIBSSH2_SFTP_HANDLE* h, char* buf, int len)
{
    if (h == NULL)
        return -1;
    return libssh2_sftp_read(h, buf, len);
}

__int64 CSFTPSession::Write(LIBSSH2_SFTP_HANDLE* h, const char* buf, int len)
{
    if (h == NULL)
        return -1;
    // libssh2_sftp_write may accept only part of the buffer; caller loops
    return libssh2_sftp_write(h, buf, len);
}

void CSFTPSession::CloseHandle(LIBSSH2_SFTP_HANDLE* h)
{
    if (h != NULL)
        libssh2_sftp_close_handle(h);
}

void CSFTPSession::Keepalive(int* secondsToNext)
{
    int next = 0;
    if (Ssh != NULL)
        libssh2_keepalive_send(Ssh, &next);
    if (secondsToNext != NULL)
        *secondsToNext = next;
}
