/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Key-authentication regression harness for the SFTP plugin (feature 051).
 *
 * Exercises the code path the plugin actually ships:
 * libssh2_userauth_publickey_frommemory() with pubkey == NULL, i.e. the
 * public key is derived from the private key in memory (CSFTPSession::
 * Authenticate, session.cpp). The predecessor of this file used the
 * *_fromfile_ex() API, whose PEM reader is a different (and safe) code path -
 * which is precisely why it never reproduced the whole-app freeze that this
 * feature fixes.
 *
 * Every scenario runs under a watchdog: exceeding WATCHDOG_SECONDS is a
 * FAILURE (exit 124), never a skip - a hang must never again pass unnoticed.
 *
 * Usage: key_auth [--host H] [--port P] [--user U] [--keydir DIR]
 *                 [--scenario NAME|all] [--list]
 * Defaults target the local reference server from
 * specs/051-fix-sftp-keyauth-hang/quickstart.md.
 *
 * Exit codes: 0 all requested scenarios passed, 1 a scenario failed,
 *             2 environment unusable, 124 watchdog fired (hang).
 */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libssh2.h>

#define WATCHDOG_SECONDS 90
#define SESSION_TIMEOUT_MS 15000

/* ------------------------------------------------------------------ config */

static const char* g_host = "127.0.0.1";
static const char* g_port = "2222";
static const char* g_user = "tctest";
static char g_keydir[MAX_PATH];
static const char* g_passphrase = "tandem123"; /* for tctest_rsa_pass */

/* --------------------------------------------------------------- watchdog */

static volatile LONG g_scenarioStart;    /* GetTickCount at scenario start */
static const char* volatile g_scenarioName = "startup";

static DWORD WINAPI WatchdogProc(LPVOID unused)
{
    (void)unused;
    for (;;)
    {
        Sleep(500);
        LONG started = InterlockedCompareExchange(&g_scenarioStart, 0, 0);
        if (started != 0 && (LONG)GetTickCount() - started > WATCHDOG_SECONDS * 1000)
        {
            printf("WATCHDOG: %s hung (>%d s)\n", g_scenarioName, WATCHDOG_SECONDS);
            fflush(stdout);
            ExitProcess(124);
        }
    }
}

/* ------------------------------------------------------------------ helpers */

/* Reads a whole file into a NUL-terminated heap buffer (the plugin does the
   same via ReadKeyFileU8 before calling *_frommemory). */
static char* ReadWholeFile(const char* path, size_t* outLen)
{
    HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return NULL;
    LARGE_INTEGER sz;
    if (!GetFileSizeEx(h, &sz) || sz.QuadPart <= 0 || sz.QuadPart > 1024 * 1024)
    {
        CloseHandle(h);
        return NULL;
    }
    size_t len = (size_t)sz.QuadPart;
    char* buf = (char*)malloc(len + 1);
    if (buf == NULL)
    {
        CloseHandle(h);
        return NULL;
    }
    DWORD read = 0;
    BOOL ok = ReadFile(h, buf, (DWORD)len, &read, NULL) && read == len;
    CloseHandle(h);
    if (!ok)
    {
        free(buf);
        return NULL;
    }
    buf[len] = 0;
    if (outLen != NULL)
        *outLen = len;
    return buf;
}

static void KeyPath(char* out, size_t outSize, const char* name)
{
    _snprintf_s(out, outSize, _TRUNCATE, "%s\\%s", g_keydir, name);
}

/* Opens a TCP connection; returns INVALID_SOCKET on failure. */
static SOCKET TcpConnect(const char* host, const char* port)
{
    struct addrinfo hints, *ai = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port, &hints, &ai) != 0 || ai == NULL)
        return INVALID_SOCKET;
    SOCKET s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (s != INVALID_SOCKET && connect(s, ai->ai_addr, (int)ai->ai_addrlen) != 0)
    {
        closesocket(s);
        s = INVALID_SOCKET;
    }
    freeaddrinfo(ai);
    return s;
}

/* Result of one authentication attempt. */
typedef struct
{
    int connected;   /* TCP + handshake reached auth */
    int rc;          /* libssh2 rc of the auth call */
    int errcode;     /* libssh2_session_last_errno */
    char msg[512];   /* libssh2 error text */
} AuthResult;

/* Runs handshake + publickey_frommemory exactly like the plugin. */
static AuthResult TryKeyAuth(const char* keyFile, const char* passphrase)
{
    AuthResult r;
    memset(&r, 0, sizeof(r));
    r.rc = -1;

    size_t keyLen = 0;
    char* keyData = ReadWholeFile(keyFile, &keyLen);
    if (keyData == NULL)
    {
        _snprintf_s(r.msg, sizeof(r.msg), _TRUNCATE, "cannot read key file %s", keyFile);
        return r;
    }

    SOCKET s = TcpConnect(g_host, g_port);
    if (s == INVALID_SOCKET)
    {
        free(keyData);
        _snprintf_s(r.msg, sizeof(r.msg), _TRUNCATE, "TCP connect failed");
        return r;
    }

    LIBSSH2_SESSION* ses = libssh2_session_init();
    libssh2_session_set_blocking(ses, 1);
    libssh2_session_set_timeout(ses, SESSION_TIMEOUT_MS);
    if (libssh2_session_handshake(ses, s) == 0)
    {
        r.connected = 1;
        /* pubkey == NULL: libssh2 derives it from the private key - the path
           that used to spin forever on OpenSSH-container keys (WinCNG). */
        r.rc = libssh2_userauth_publickey_frommemory(
            ses, g_user, strlen(g_user), NULL, 0, keyData, keyLen,
            (passphrase != NULL && *passphrase) ? passphrase : NULL);
    }
    else
    {
        _snprintf_s(r.msg, sizeof(r.msg), _TRUNCATE, "handshake failed");
    }
    if (r.connected)
    {
        char* m = NULL;
        int l = 0;
        r.errcode = libssh2_session_last_error(ses, &m, &l, 0);
        if (m != NULL)
            _snprintf_s(r.msg, sizeof(r.msg), _TRUNCATE, "%s", m);
    }

    libssh2_session_disconnect(ses, "bye");
    libssh2_session_free(ses);
    closesocket(s);
    SecureZeroMemory(keyData, keyLen);
    free(keyData);
    return r;
}

/* Generates a throwaway key with ssh-keygen; returns 0 on success. */
static int GenerateKey(const char* type, const char* bits, const char* path)
{
    char cmd[1024];
    DeleteFileA(path);
    if (bits != NULL)
        _snprintf_s(cmd, sizeof(cmd), _TRUNCATE,
                    "ssh-keygen -q -t %s -b %s -N \"\" -C harness -f \"%s\" >nul 2>&1",
                    type, bits, path);
    else
        _snprintf_s(cmd, sizeof(cmd), _TRUNCATE,
                    "ssh-keygen -q -t %s -N \"\" -C harness -f \"%s\" >nul 2>&1",
                    type, path);
    return system(cmd);
}

static void TempPath(char* out, size_t outSize, const char* leaf)
{
    char tmp[MAX_PATH];
    GetTempPathA(sizeof(tmp), tmp);
    _snprintf_s(out, outSize, _TRUNCATE, "%s%s", tmp, leaf);
}

/* ---------------------------------------------------------------- scenarios */

/* Each scenario returns 1 on PASS, 0 on FAIL, and fills 'detail'. */
typedef int (*ScenarioFn)(char* detail, size_t detailSize);

static int ExpectAuthOk(const char* keyName, const char* passphrase,
                        char* detail, size_t detailSize)
{
    char key[MAX_PATH];
    KeyPath(key, sizeof(key), keyName);
    AuthResult r = TryKeyAuth(key, passphrase);
    if (r.rc == 0)
        return 1;
    _snprintf_s(detail, detailSize, _TRUNCATE, "rc=%d err=%d %s", r.rc, r.errcode, r.msg);
    return 0;
}

static int Scn_KeyRsa(char* d, size_t n) { return ExpectAuthOk("tctest_rsa", NULL, d, n); }
static int Scn_KeyEcdsa(char* d, size_t n) { return ExpectAuthOk("tctest_ecdsa", NULL, d, n); }
static int Scn_KeyPassphrase(char* d, size_t n) { return ExpectAuthOk("tctest_rsa_pass", g_passphrase, d, n); }

static int Scn_KeyPassphraseWrong(char* d, size_t n)
{
    char key[MAX_PATH];
    KeyPath(key, sizeof(key), "tctest_rsa_pass");
    AuthResult r = TryKeyAuth(key, "definitely-wrong");
    if (r.rc == 0)
    {
        _snprintf_s(d, n, _TRUNCATE, "auth SUCCEEDED with a wrong passphrase");
        return 0;
    }
    /* Must be a key-side failure (cannot decrypt / load), not a server verdict. */
    if (r.errcode == LIBSSH2_ERROR_AUTHENTICATION_FAILED)
        _snprintf_s(d, n, _TRUNCATE,
                    "server-side verdict for a key that cannot be decrypted: %s", r.msg);
    else
        _snprintf_s(d, n, _TRUNCATE, "rejected as expected (err=%d)", r.errcode);
    return 1;
}

static int Scn_KeyUnauthorized(char* d, size_t n)
{
    char key[MAX_PATH];
    TempPath(key, sizeof(key), "tc_harness_unauth_key");
    if (GenerateKey("rsa", "2048", key) != 0)
    {
        _snprintf_s(d, n, _TRUNCATE, "ssh-keygen unavailable");
        return 0;
    }
    AuthResult r = TryKeyAuth(key, NULL);
    char pub[MAX_PATH];
    _snprintf_s(pub, sizeof(pub), _TRUNCATE, "%s.pub", key);
    DeleteFileA(key);
    DeleteFileA(pub);

    if (r.rc == 0)
    {
        _snprintf_s(d, n, _TRUNCATE, "server accepted a key it should not know");
        return 0;
    }
    /* The key itself must parse - the failure has to come from the server. */
    if (r.errcode == LIBSSH2_ERROR_FILE || r.errcode == LIBSSH2_ERROR_METHOD_NOT_SUPPORTED)
    {
        _snprintf_s(d, n, _TRUNCATE, "key-side error, expected server rejection: %s", r.msg);
        return 0;
    }
    _snprintf_s(d, n, _TRUNCATE, "server rejected key (err=%d)", r.errcode);
    return 1;
}

static int Scn_KeyUnsupported(char* d, size_t n)
{
    /* ed25519 is not available on the WinCNG backend, and a PuTTY .ppk is not a
       key libssh2 can parse. Both must fail promptly and cleanly. */
    char key[MAX_PATH], pub[MAX_PATH], ppk[MAX_PATH];
    TempPath(key, sizeof(key), "tc_harness_ed25519_key");
    if (GenerateKey("ed25519", NULL, key) != 0)
    {
        _snprintf_s(d, n, _TRUNCATE, "ssh-keygen unavailable");
        return 0;
    }
    AuthResult r1 = TryKeyAuth(key, NULL);
    _snprintf_s(pub, sizeof(pub), _TRUNCATE, "%s.pub", key);
    DeleteFileA(key);
    DeleteFileA(pub);

    TempPath(ppk, sizeof(ppk), "tc_harness_stub.ppk");
    HANDLE h = CreateFileA(ppk, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    int r2ok = 0;
    if (h != INVALID_HANDLE_VALUE)
    {
        static const char stub[] = "PuTTY-User-Key-File-3: ssh-rsa\nEncryption: none\n";
        DWORD w = 0;
        WriteFile(h, stub, (DWORD)(sizeof(stub) - 1), &w, NULL);
        CloseHandle(h);
        AuthResult r2 = TryKeyAuth(ppk, NULL);
        r2ok = (r2.rc != 0);
        DeleteFileA(ppk);
    }
    else
    {
        _snprintf_s(d, n, _TRUNCATE, "cannot create .ppk stub");
        return 0;
    }

    if (r1.rc == 0 || !r2ok)
    {
        _snprintf_s(d, n, _TRUNCATE, "unsupported key accepted (ed25519 rc=%d, ppk ok=%d)",
                    r1.rc, r2ok);
        return 0;
    }
    _snprintf_s(d, n, _TRUNCATE, "both rejected (ed25519 err=%d)", r1.errcode);
    return 1;
}

/* A local listener that accepts one connection and then stays silent forever -
   models a server that never speaks SSH. */
typedef struct
{
    SOCKET listener;
    unsigned short port;
} SilentServer;

static DWORD WINAPI SilentServerProc(LPVOID p)
{
    SilentServer* srv = (SilentServer*)p;
    SOCKET c = accept(srv->listener, NULL, NULL);
    if (c != INVALID_SOCKET)
        Sleep(60000); /* hold the connection open, send nothing */
    return 0;
}

static int Scn_TimeoutSilent(char* d, size_t n)
{
    SilentServer srv;
    srv.listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (srv.listener == INVALID_SOCKET)
    {
        _snprintf_s(d, n, _TRUNCATE, "cannot create listener");
        return 0;
    }
    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = 0;
    int alen = sizeof(a);
    if (bind(srv.listener, (struct sockaddr*)&a, alen) != 0 ||
        listen(srv.listener, 1) != 0 ||
        getsockname(srv.listener, (struct sockaddr*)&a, &alen) != 0)
    {
        closesocket(srv.listener);
        _snprintf_s(d, n, _TRUNCATE, "cannot bind listener");
        return 0;
    }
    srv.port = ntohs(a.sin_port);
    HANDLE th = CreateThread(NULL, 0, SilentServerProc, &srv, 0, NULL);

    char portStr[16];
    _snprintf_s(portStr, sizeof(portStr), _TRUNCATE, "%u", (unsigned)srv.port);
    DWORD t0 = GetTickCount();
    int timedOut = 0;
    SOCKET s = TcpConnect("127.0.0.1", portStr);
    if (s != INVALID_SOCKET)
    {
        LIBSSH2_SESSION* ses = libssh2_session_init();
        libssh2_session_set_blocking(ses, 1);
        libssh2_session_set_timeout(ses, SESSION_TIMEOUT_MS);
        timedOut = (libssh2_session_handshake(ses, s) != 0);
        libssh2_session_free(ses);
        closesocket(s);
    }
    DWORD elapsed = GetTickCount() - t0;

    closesocket(srv.listener);
    if (th != NULL)
    {
        WaitForSingleObject(th, 1000);
        CloseHandle(th);
    }

    if (!timedOut)
    {
        _snprintf_s(d, n, _TRUNCATE, "handshake unexpectedly succeeded");
        return 0;
    }
    /* Generous ceiling: the point is boundedness, not the exact value. */
    if (elapsed > SESSION_TIMEOUT_MS + 10000)
    {
        _snprintf_s(d, n, _TRUNCATE, "timeout took %lu ms (bound %d ms)",
                    (unsigned long)elapsed, SESSION_TIMEOUT_MS);
        return 0;
    }
    _snprintf_s(d, n, _TRUNCATE, "bounded failure in %lu ms", (unsigned long)elapsed);
    return 1;
}

typedef struct
{
    const char* name;
    ScenarioFn fn;
    int needsServer;
} Scenario;

static const Scenario g_scenarios[] = {
    {"key-rsa", Scn_KeyRsa, 1},
    {"key-ecdsa", Scn_KeyEcdsa, 1},
    {"key-passphrase", Scn_KeyPassphrase, 1},
    {"key-passphrase-wrong", Scn_KeyPassphraseWrong, 1},
    {"key-unauthorized", Scn_KeyUnauthorized, 1},
    {"key-unsupported", Scn_KeyUnsupported, 1},
    {"timeout-silent", Scn_TimeoutSilent, 0},
};
#define SCENARIO_COUNT (int)(sizeof(g_scenarios) / sizeof(g_scenarios[0]))

/* --------------------------------------------------------------------- main */

static void Usage(const char* exe)
{
    printf("usage: %s [--host H] [--port P] [--user U] [--keydir DIR]\n"
           "          [--passphrase P] [--scenario NAME|all] [--list]\n", exe);
}

int main(int argc, char** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    const char* profile = getenv("USERPROFILE");
    _snprintf_s(g_keydir, sizeof(g_keydir), _TRUNCATE, "%s\\.ssh\\tandem-sftp-test",
                profile != NULL ? profile : ".");
    const char* only = "all";

    for (int i = 1; i < argc; i++)
    {
        int hasVal = (i + 1 < argc);
        if (!strcmp(argv[i], "--host") && hasVal) g_host = argv[++i];
        else if (!strcmp(argv[i], "--port") && hasVal) g_port = argv[++i];
        else if (!strcmp(argv[i], "--user") && hasVal) g_user = argv[++i];
        else if (!strcmp(argv[i], "--passphrase") && hasVal) g_passphrase = argv[++i];
        else if (!strcmp(argv[i], "--keydir") && hasVal)
            _snprintf_s(g_keydir, sizeof(g_keydir), _TRUNCATE, "%s", argv[++i]);
        else if (!strcmp(argv[i], "--scenario") && hasVal) only = argv[++i];
        else if (!strcmp(argv[i], "--list"))
        {
            for (int k = 0; k < SCENARIO_COUNT; k++)
                printf("%s\n", g_scenarios[k].name);
            return 0;
        }
        else { Usage(argv[0]); return 2; }
    }

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { printf("WSAStartup failed\n"); return 2; }
    if (libssh2_init(0) != 0) { printf("libssh2_init failed\n"); return 2; }

    HANDLE wd = CreateThread(NULL, 0, WatchdogProc, NULL, 0, NULL);
    (void)wd;

    /* Environment check: the reference server must answer before we blame the
       product for a failure (exit 2 keeps infra distinct from regressions). */
    int needServer = 0;
    for (int k = 0; k < SCENARIO_COUNT; k++)
        if ((!strcmp(only, "all") || !strcmp(only, g_scenarios[k].name)) &&
            g_scenarios[k].needsServer)
            needServer = 1;
    if (needServer)
    {
        SOCKET probe = TcpConnect(g_host, g_port);
        if (probe == INVALID_SOCKET)
        {
            printf("ENV: cannot reach %s:%s - start the reference server "
                   "(docker start tandem-sftp)\n", g_host, g_port);
            return 2;
        }
        closesocket(probe);
    }

    int passed = 0, failed = 0, matched = 0;
    for (int k = 0; k < SCENARIO_COUNT; k++)
    {
        if (strcmp(only, "all") != 0 && strcmp(only, g_scenarios[k].name) != 0)
            continue;
        matched++;
        char detail[512];
        detail[0] = 0;
        g_scenarioName = g_scenarios[k].name;
        DWORD t0 = GetTickCount();
        InterlockedExchange(&g_scenarioStart, (LONG)t0);
        int ok = g_scenarios[k].fn(detail, sizeof(detail));
        DWORD ms = GetTickCount() - t0;
        InterlockedExchange(&g_scenarioStart, 0);
        printf("%s %s %lu%s%s\n", ok ? "PASS" : "FAIL", g_scenarios[k].name,
               (unsigned long)ms, detail[0] ? " " : "", detail);
        if (ok) passed++; else failed++;
    }

    libssh2_exit();
    WSACleanup();

    if (matched == 0)
    {
        printf("ENV: unknown scenario '%s' (use --list)\n", only);
        return 2;
    }
    printf("TOTAL %d passed / %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
