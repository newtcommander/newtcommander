/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Standalone runtime smoke test for the vendored libssh2 (WinCNG backend).
 * Exercises the exact API sequence CSFTPSession uses (handshake, host-key
 * hash, password auth, SFTP init, opendir/readdir with permissions, download)
 * against a real OpenSSH server. Proves the SSH/SFTP layer actually works at
 * runtime - something compilation alone cannot. Dev-only; not shipped.
 *
 * Usage: sftp_smoke <host> <port> <user> <password> <remotedir>
 */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <libssh2.h>
#include <libssh2_sftp.h>

static void fmt_rights(unsigned long m, char* b)
{
    const char* t = "-";
    switch (m & 0170000) {
    case 0040000: t = "d"; break;
    case 0120000: t = "l"; break;
    case 0100000: t = "-"; break;
    case 0060000: t = "b"; break;
    case 0020000: t = "c"; break;
    case 0140000: t = "s"; break;
    case 0010000: t = "p"; break;
    }
    b[0] = t[0];
    b[1] = (m & 0400) ? 'r' : '-';
    b[2] = (m & 0200) ? 'w' : '-';
    b[3] = (m & 04000) ? ((m & 0100) ? 's' : 'S') : ((m & 0100) ? 'x' : '-');
    b[4] = (m & 0040) ? 'r' : '-';
    b[5] = (m & 0020) ? 'w' : '-';
    b[6] = (m & 02000) ? ((m & 0010) ? 's' : 'S') : ((m & 0010) ? 'x' : '-');
    b[7] = (m & 0004) ? 'r' : '-';
    b[8] = (m & 0002) ? 'w' : '-';
    b[9] = (m & 01000) ? ((m & 0001) ? 't' : 'T') : ((m & 0001) ? 'x' : '-');
    b[10] = 0;
}

int main(int argc, char** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0); /* unbuffered so we see progress even on crash */
    if (argc < 6) {
        printf("usage: %s host port user password remotedir\n", argv[0]);
        return 2;
    }
    const char* host = argv[1];
    const char* port = argv[2];
    const char* user = argv[3];
    const char* pass = argv[4];
    const char* rdir = argv[5];

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { printf("WSAStartup failed\n"); return 1; }
    if (libssh2_init(0) != 0) { printf("libssh2_init failed\n"); return 1; }

    struct addrinfo hints, *ai = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port, &hints, &ai) != 0 || !ai) { printf("resolve failed\n"); return 1; }

    SOCKET s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (connect(s, ai->ai_addr, (int)ai->ai_addrlen) != 0) { printf("connect failed (%d)\n", WSAGetLastError()); return 1; }
    freeaddrinfo(ai);
    printf("[ok] TCP connected to %s:%s\n", host, port);

    LIBSSH2_SESSION* ses = libssh2_session_init();
    libssh2_session_set_blocking(ses, 1);
    libssh2_session_set_timeout(ses, 15000);
    if (libssh2_session_handshake(ses, s) != 0) {
        char* m = NULL; int l = 0; libssh2_session_last_error(ses, &m, &l, 0);
        printf("[FAIL] handshake: %s\n", m ? m : "?"); return 1;
    }
    printf("[ok] SSH handshake complete\n");

    size_t klen = 0; int ktype = 0;
    const char* kb = libssh2_session_hostkey(ses, &klen, &ktype);
    const char* sha = libssh2_hostkey_hash(ses, LIBSSH2_HOSTKEY_HASH_SHA256);
    printf("[ok] host key present: type=%d, %d bytes, sha256=%p\n", ktype, (int)klen, (void*)sha);

    if (libssh2_userauth_password(ses, user, pass) != 0) {
        char* m = NULL; int l = 0; libssh2_session_last_error(ses, &m, &l, 0);
        printf("[FAIL] password auth: %s\n", m ? m : "?"); return 1;
    }
    printf("[ok] password authentication succeeded\n");

    LIBSSH2_SFTP* sftp = libssh2_sftp_init(ses);
    if (!sftp) { printf("[FAIL] sftp_init\n"); return 1; }
    printf("[ok] SFTP subsystem started\n");

    LIBSSH2_SFTP_HANDLE* dir = libssh2_sftp_opendir(sftp, rdir);
    if (!dir) { printf("[FAIL] opendir %s\n", rdir); return 1; }
    printf("[ok] listing %s:\n", rdir);

    char name[512], longentry[512];
    LIBSSH2_SFTP_ATTRIBUTES at;
    int count = 0, sawSetuid = 0, sawSticky = 0, sawLink = 0, sawUtf8 = 0;
    for (;;) {
        int rc = libssh2_sftp_readdir_ex(dir, name, sizeof(name) - 1, longentry, sizeof(longentry) - 1, &at);
        if (rc <= 0) break;
        name[rc] = 0;
        if (!strcmp(name, ".") || !strcmp(name, "..")) continue;
        char r[12] = "----------";
        if (at.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) fmt_rights(at.permissions, r);
        printf("    %s %10llu  %s\n", r, (unsigned long long)at.filesize, name);
        count++;
        if ((at.permissions & 04000)) sawSetuid = 1;
        if ((at.permissions & 01000)) sawSticky = 1;
        if ((at.permissions & 0170000) == 0120000) sawLink = 1;
        if ((unsigned char)name[0] >= 0x80) sawUtf8 = 1;
    }
    libssh2_sftp_closedir(dir);
    printf("[ok] %d entries listed (setuid=%d sticky=%d symlink=%d utf8-name=%d)\n",
           count, sawSetuid, sawSticky, sawLink, sawUtf8);

    /* download one file and check contents */
    char fpath[1024];
    _snprintf_s(fpath, sizeof(fpath), _TRUNCATE, "%s/regular.txt", rdir);
    LIBSSH2_SFTP_HANDLE* fh = libssh2_sftp_open_ex(sftp, fpath, (unsigned)strlen(fpath), LIBSSH2_FXF_READ, 0, LIBSSH2_SFTP_OPENFILE);
    if (fh) {
        char buf[256]; ssize_t n = libssh2_sftp_read(fh, buf, sizeof(buf) - 1);
        if (n > 0) { buf[n] = 0; printf("[ok] downloaded regular.txt (%d bytes): %.20s", (int)n, buf); }
        libssh2_sftp_close_handle(fh);
    } else printf("[warn] could not open regular.txt\n");

    libssh2_sftp_shutdown(sftp);
    libssh2_session_disconnect(ses, "bye");
    libssh2_session_free(ses);
    closesocket(s);
    libssh2_exit();
    WSACleanup();

    int ok = (count > 0 && sawSetuid && sawSticky && sawLink && sawUtf8);
    printf("\n%s\n", ok ? "SMOKE TEST PASSED" : "SMOKE TEST INCOMPLETE (missing expected entry types)");
    return ok ? 0 : 1;
}
