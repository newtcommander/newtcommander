/* SPDX-License-Identifier: GPL-2.0-or-later
 * Spike S1: tests libssh2 (WinCNG backend) private-key authentication with a
 * given key file, exactly as CSFTPSession::Authenticate does
 * (libssh2_userauth_publickey_fromfile_ex, pubkey derived from the private key).
 * Usage: key_auth <host> <port> <user> <keyfile>
 */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <libssh2.h>

int main(int argc, char** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc < 5) { printf("usage: %s host port user keyfile\n", argv[0]); return 2; }
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa); libssh2_init(0);
    struct addrinfo hints, *ai = NULL;
    memset(&hints, 0, sizeof(hints)); hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(argv[1], argv[2], &hints, &ai) != 0 || !ai) { printf("resolve fail\n"); return 1; }
    SOCKET s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (connect(s, ai->ai_addr, (int)ai->ai_addrlen) != 0) { printf("connect fail %d\n", WSAGetLastError()); return 1; }
    LIBSSH2_SESSION* ses = libssh2_session_init();
    libssh2_session_set_blocking(ses, 1);
    libssh2_session_set_timeout(ses, 15000);
    if (libssh2_session_handshake(ses, s) != 0) { printf("handshake fail\n"); return 1; }
    int rc = libssh2_userauth_publickey_fromfile_ex(ses, argv[3], (unsigned)strlen(argv[3]),
                                                    NULL, argv[4], NULL);
    if (rc == 0) {
        printf("KEY-AUTH OK: %s\n", argv[4]);
    } else {
        char* m = NULL; int l = 0; libssh2_session_last_error(ses, &m, &l, 0);
        printf("KEY-AUTH FAIL (rc=%d): %s [%s]\n", rc, argv[4], m ? m : "?");
    }
    libssh2_session_disconnect(ses, "bye"); libssh2_session_free(ses);
    closesocket(s); libssh2_exit(); WSACleanup();
    return rc == 0 ? 0 : 1;
}
