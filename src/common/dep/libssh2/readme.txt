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

Local patches (re-apply when updating; marked "Tandem Commander local patch"):
  - feature 051, src\pem.c: readline_memory() fails at end of buffer and
    both scan loops of _libssh2_pem_parse_memory() carry bounds guards.
    Upstream 1.11.1's readline_memory can never fail, so feeding any key
    without the expected classic-PEM marker (e.g. an OpenSSH-container
    key routed there by the WinCNG backend) spun the header-scan loop
    forever - a CPU-bound hang. Upstream later rewrote the reader
    (pem_readline fails at EOF); this patch is the minimal equivalent.
  - feature 051, src\pem.c: _libssh2_pem_parse_memory() gained a
    passphrase parameter and the Proc-Type/DEK-Info decryption branch
    ported from _libssh2_pem_parse(), so classic PEM keys encrypted with
    a passphrase load from memory (the plugin's only path). Differences
    from the FILE variant: an encrypted key without a passphrase and a
    PKCS#7 padding mismatch after decryption (i.e. a wrong passphrase)
    both return LIBSSH2_ERROR_KEYFILE_AUTH_FAILED via _libssh2_error();
    on failure *data is freed (the FILE variant frees the wrong pointer).
  - feature 051, src\libssh2_priv.h: prototype of
    _libssh2_pem_parse_memory() updated for the passphrase parameter.
  - feature 051, src\os400qc3.c: the six _libssh2_pem_parse_memory()
    call sites pass NULL for the new passphrase parameter (PKCS#8
    decryption happens inside rsapkcs8privkey/rsapkcs8pubkey there).
    Not compiled; kept consistent so a future backend switch compiles.
  - feature 051, src\wincng.c: OpenSSH-container (openssh-key-v1) key
    support and real error codes on the WinCNG in-memory key paths, so
    libssh2_userauth_publickey_frommemory(pubkey=NULL) works with the
    keys ssh-keygen writes by default since OpenSSH 7.8:
      * _libssh2_wincng_load_private_memory() forwards the passphrase
        (it was discarded) and propagates the decrypt verdict.
      * _libssh2_wincng_rsa_new_private_openssh() /
        _libssh2_wincng_rsa_new_openssh_frommemory(): parse the container
        private-key section (n, e, d, iqmp, p, q) and import the key as
        BCRYPT_RSAPRIVATE_BLOB (e, n, p, q); CNG derives the rest.
        Hooked into _libssh2_wincng_rsa_new_private_frommemory() as the
        fallback when classic PEM parsing fails.
      * _libssh2_wincng_ecdsa_new_private_frommemory(): accepts the raw
        PEM text of an OpenSSH container (callers always pass PEM text,
        but the function expected the base64-decoded container, so it
        could never succeed from memory) and reuses
        _libssh2_wincng_parse_ecdsa_privatekey().
      * _libssh2_wincng_pub_priv_openssh_keyfilememory(): derives method
        name + SSH wire-format public key blob from a container key
        (RSA: e, n; ECDSA: type, curve, Q) for public-key derivation;
        unsupported key types (e.g. ed25519 - WinCNG cannot do it) fail
        immediately with LIBSSH2_ERROR_METHOD_NOT_SUPPORTED.
      * failure paths in the *_frommemory functions set real libssh2
        error codes (LIBSSH2_ERROR_FILE / METHOD_NOT_SUPPORTED /
        KEYFILE_AUTH_FAILED) instead of returning bare -1; the sftp
        plugin classifies failures by these codes.
    Verified by src\plugins\sftp\test\run_keyauth.cmd (7 scenarios).
  - feature 051, src\wincng.c: _libssh2_wincng_ecdh_gen_k() frees the
    bignum the caller passed in before allocating its own, and NULLs
    *secret after freeing it on the failure path. kex.c's ecdh_sha2_nistp
    allocates exchange_state->k via _libssh2_bn_init() and passes its
    address; the function simply overwrote the pointer, leaking one
    16-byte bignum per ECDH key exchange - i.e. per SSH session, which the
    Debug build reports as "Detected memory leaks!" on exit - and left a
    dangling pointer that the caller's cleanup freed a second time. An
    upstream defect in the WinCNG ECDH support, independent of the
    key-format work above; found by the harness's CRT leak check.

How to update:
  1. git clone --depth 1 --branch libssh2-<ver> https://github.com/libssh2/libssh2
  2. Replace include\*.h and src\*.c/src\*.h with the new tree
     (copy only *.c/*.h; keep COPYING in sync).
  3. Update the version/commit above and check for new/removed .c files
     against the ClCompile list in sftp.vcxproj.
  4. Rebuild and run the sftp plugin verification (specs\008-sftp-plugin\quickstart.md).
