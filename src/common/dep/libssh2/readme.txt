libssh2 - client-side SSH2 / SFTP library
https://libssh2.org / https://github.com/libssh2/libssh2

Vendored version: release tag libssh2-1.11.1
  commit a312b43325e3383c865a87bb1d26cb52e3292641
  (include/libssh2.h reports "1.11.1_DEV" because upstream stamps the
  final version string only into release tarballs via maketgz; the code
  is identical to the 1.11.1 release.)
License: BSD-3-Clause (see COPYING; notice recorded in doc\third_party.txt)

Layout (trimmed upstream tree - no docs/examples/tests/build files):
  include\   public headers (libssh2.h, libssh2_sftp.h, libssh2_publickey.h)
  src\       implementation (.c/.h)

How it is built (no standalone project):
  The sftp plugin (src\plugins\sftp\vcxproj\sftp.vcxproj) compiles the
  sources directly with the preprocessor define LIBSSH2_WINCNG (Windows
  CNG crypto backend - bcrypt.lib, crypt32.lib, ws2_32.lib). crypto.c
  #includes the selected backend, therefore the backend files
  (wincng.c, openssl.c, mbedtls.c, libgcrypt.c, os400qc3.c) are vendored
  but MUST NOT be compiled as separate translation units.
  zlib compression (LIBSSH2_HAVE_ZLIB) is not enabled.

IMPORTANT build note (Debug builds):
  The libssh2 C files MUST be compiled with /RTCc (SmallerTypeCheck) and
  /RTC1 (BasicRuntimeChecks) DISABLED - sftp.vcxproj sets
  <SmallerTypeCheck>false</SmallerTypeCheck> and
  <BasicRuntimeChecks>Default</BasicRuntimeChecks> on every libssh2 ClCompile
  entry. libssh2's crypto/transport code legitimately truncates integers to
  smaller types; the shared plugin Debug property sheet enables /RTCc, whose
  run-time check calls abort() on such truncation and crashes the Debug build
  during the SSH handshake. This was found via the runtime smoke test
  (test\sftp_smoke.c) - the Release build was unaffected because it does not
  use /RTCc.

How to update:
  1. git clone --depth 1 --branch libssh2-<ver> https://github.com/libssh2/libssh2
  2. Replace include\*.h and src\*.c/src\*.h with the new tree
     (copy only *.c/*.h; keep COPYING in sync).
  3. Update the version/commit above and check for new/removed .c files
     against the ClCompile list in sftp.vcxproj.
  4. Rebuild and run the sftp plugin verification (specs\008-sftp-plugin\quickstart.md).
