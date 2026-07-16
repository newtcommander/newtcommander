// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Dev-only unit test harness for the SFTP plugin's pure-logic helpers.
// Not shipped, not part of salamand.sln / plugins.cfg. Build & run:
//   see test\build_and_run.cmd  (compiles sftputils.cpp standalone).
// Exits non-zero on the first failed assertion.

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include "sftputils.h"

static int g_fail = 0;
static int g_pass = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (cond) { g_pass++; }                                         \
        else { g_fail++; printf("FAIL: %s (line %d)\n", #cond, __LINE__); } \
    } while (0)

#define CHECK_STR(a, b)                                                 \
    do {                                                                \
        if (strcmp((a), (b)) == 0) { g_pass++; }                        \
        else { g_fail++; printf("FAIL: \"%s\" != \"%s\" (line %d)\n", (a), (b), __LINE__); } \
    } while (0)

static void TestRights()
{
    char buf[16];
    CHECK_STR(FormatUnixRights(SFTP_S_IFDIR | 0755, buf), "drwxr-xr-x");
    CHECK_STR(FormatUnixRights(SFTP_S_IFREG | 0644, buf), "-rw-r--r--");
    CHECK_STR(FormatUnixRights(SFTP_S_IFLNK | 0777, buf), "lrwxrwxrwx");
    CHECK_STR(FormatUnixRights(SFTP_S_IFREG | SFTP_S_ISUID | 0755, buf), "-rwsr-xr-x");
    CHECK_STR(FormatUnixRights(SFTP_S_IFDIR | SFTP_S_ISVTX | 0777, buf), "drwxrwxrwt");
    // setgid without group-execute -> capital S
    CHECK_STR(FormatUnixRights(SFTP_S_IFREG | SFTP_S_ISGID | 0644, buf), "-rw-r-Sr--");
    // sticky without other-execute -> capital T
    CHECK_STR(FormatUnixRights(SFTP_S_IFDIR | SFTP_S_ISVTX | 0770, buf), "drwxrwx--T");
    // block/char/fifo/socket type chars
    CHECK_STR(FormatUnixRights(SFTP_S_IFBLK | 0660, buf), "brw-rw----");
    CHECK_STR(FormatUnixRights(SFTP_S_IFCHR | 0666, buf), "crw-rw-rw-");
    // permission-only mode (server omitted the type bits) -> regular-file dash
    CHECK_STR(FormatUnixRights(0644, buf), "-rw-r--r--");
}

static void TestOctal()
{
    char buf[8];
    CHECK_STR(FormatOctalMode(0755, buf), "0755");
    CHECK_STR(FormatOctalMode(SFTP_S_ISUID | 0755, buf), "4755");
    CHECK_STR(FormatOctalMode(SFTP_S_IFREG | 0644, buf), "0644"); // type bits masked off

    unsigned long m = 0;
    CHECK(ParseOctalMode("755", &m) && m == 0755);
    CHECK(ParseOctalMode("0644", &m) && m == 0644);
    CHECK(ParseOctalMode("4755", &m) && m == (unsigned long)(SFTP_S_ISUID | 0755));
    CHECK(ParseOctalMode("  700  ", &m) && m == 0700);
    CHECK(!ParseOctalMode("abc", &m));
    CHECK(!ParseOctalMode("789", &m));   // 8,9 not octal
    CHECK(!ParseOctalMode("", &m));
}

static void TestUtf8()
{
    CHECK(IsValidUTF8("hello", 5));
    CHECK(IsValidUTF8("\xC3\xA9", 2));            // e-acute
    CHECK(IsValidUTF8("\xE6\x97\xA5", 3));        // CJK
    CHECK(!IsValidUTF8("\xFF", 1));               // invalid lead byte
    CHECK(!IsValidUTF8("\xC3", 1));               // truncated 2-byte
    CHECK(!IsValidUTF8("\xC0\x80", 2));           // overlong NUL

    char buf[64];
    CHECK_STR(SanitizeUTF8("clean", buf, sizeof(buf)), "clean");
    CHECK_STR(SanitizeUTF8("\xC3\xA9.txt", buf, sizeof(buf)), "\xC3\xA9.txt"); // valid preserved
    // invalid byte becomes a single '?'
    SanitizeUTF8("a\xFF" "b", buf, sizeof(buf));
    CHECK_STR(buf, "a?b");
}

static void TestPaths()
{
    char buf[256];
    CHECK_STR(PosixPathAppend("/home/user", "file.txt", buf, sizeof(buf)), "/home/user/file.txt");
    CHECK_STR(PosixPathAppend("/", "etc", buf, sizeof(buf)), "/etc");
    CHECK_STR(PosixPathAppend("/a/", "b", buf, sizeof(buf)), "/a/b");
    CHECK_STR(PosixPathAppend("", "x", buf, sizeof(buf)), "/x");

    CHECK_STR(PosixBaseName("/home/user/file.txt"), "file.txt");
    CHECK_STR(PosixBaseName("/"), "");
    CHECK_STR(PosixBaseName("noslash"), "noslash");
    CHECK(IsDotOrDotDot(".") && IsDotOrDotDot("..") && !IsDotOrDotDot(".hidden"));
}

static void TestTime()
{
    // round-trip a known instant
    __int64 t = 1700000000; // 2023-11-14T22:13:20Z
    FILETIME ft;
    UnixTimeToFileTime(t, &ft);
    __int64 back = FileTimeToUnixTime(&ft);
    CHECK(back == t);

    CHECK(UnixTypeChar(SFTP_S_IFDIR | 0755) == 'd');
    CHECK(UnixTypeChar(SFTP_S_IFLNK) == 'l');
    CHECK(UnixTypeChar(SFTP_S_IFREG) == '-');
}

static void TestAttrs()
{
    // read-only synthesized when owner lacks write
    DWORD a = SynthesizeWinAttributes(SFTP_S_IFREG | 0444, "file");
    CHECK((a & FILE_ATTRIBUTE_READONLY) != 0);
    a = SynthesizeWinAttributes(SFTP_S_IFREG | 0644, "file");
    CHECK((a & FILE_ATTRIBUTE_READONLY) == 0);
    // dotfile hidden
    a = SynthesizeWinAttributes(SFTP_S_IFREG | 0644, ".bashrc");
    CHECK((a & FILE_ATTRIBUTE_HIDDEN) != 0);
    // directory bit
    a = SynthesizeWinAttributes(SFTP_S_IFDIR | 0755, "d");
    CHECK((a & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

int main()
{
    TestRights();
    TestOctal();
    TestUtf8();
    TestPaths();
    TestTime();
    TestAttrs();

    printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
