# Tasks: SFTP Security & Functional Review

**Feature**: 024-sftp-security-hardening | **Spec/Plan**: [spec.md](spec.md) · [plan.md](plan.md) · [research.md](research.md)

Ordering: build/handshake first (the headline, independently verifiable), then
security-hardening, then localized robustness. `[P]` = independent, could be done
in any order within its group. Each task cites the consolidated finding ID.

## Phase A — Handshake & build config (headline)

- [x] **T001 (CF-1)** `vcxproj/sftp.props`: add `LIBSSH2_ECDSA_WINCNG` to the C/C++
  `PreprocessorDefinitions`; raise `WINVER` and `_WIN32_WINNT` from `0x0601` to
  `0x0A00` (both the main and resource-compiler `PreprocessorDefinitions`).
- [x] **T002 (CF-3)** `session.cpp` `SetLastErrorFromSsh` + `lang/lang.rc2`: make
  the composed message not emit a literal `%s`. Change `IDS_ERR_HANDSHAKE` to
  `"SSH handshake failed"` (no `%s`) and `IDS_ERR_SFTPINIT` wording so the
  appended libssh2 text reads cleanly; verify `SetLastErrorFromSsh` prefix logic.
- [x] **T003 (CF-1 verify)** Compile `sftp_smoke.c` twice (with/without
  `LIBSSH2_ECDSA_WINCNG`) against the vendored libssh2; run both against the
  hardened Docker server (before = handshake fails, after = connects) and a
  legacy-config server (after still connects — non-regression).

## Phase B — Transport hardening

- [x] **T010 (CF-4)** `session.cpp` after `libssh2_session_init_ex` (:432): call
  `libssh2_session_method_pref` for `LIBSSH2_METHOD_KEX`, `CRYPT_CS`, `CRYPT_SC`,
  `MAC_CS`, `MAC_SC`, `HOSTKEY` with modern-only lists that keep the interop floor
  (ecdh + group14-sha256 + group16/18; chacha20-poly1305 + aes256/128-ctr;
  hmac-sha2-256/512; ecdsa + rsa-sha2-256/512). Log-and-continue on failure.
- [x] **T011 (CF-27) [P]** `session.cpp`: call
  `libssh2_session_flag(Ssh, LIBSSH2_FLAG_COMPRESS, 1)` before handshake when
  `Params.UseCompression` (make the existing setting actually work).

## Phase C — File-operation safety

- [x] **T020 (CF-5)** `operats.cpp`: add a recursion-depth cap to `DownloadDir`/
  `DownloadRecursive`, `DeleteRecursive`, `ChmodRecursive`, `ChownRecursive`,
  `UploadDirRecursive`/`DeleteLocalTree`; abort the operation with a clear error
  past the cap (e.g. 128 levels).
- [x] **T021 (CF-6)** `operats.cpp` `DeleteLocalTree`/`UploadDirRecursive`: test
  `FILE_ATTRIBUTE_REPARSE_POINT`; do not recurse into reparse-point dirs; remove
  the link itself with `RemoveDirectoryW` (no target deletion).
- [x] **T022 (CF-16) [P]** `operats.cpp` `SanitizeLocalName`: reject/rename
  Windows reserved device basenames (`CON PRN AUX NUL COM1-9 LPT1-9`, with/without
  extension) and strip trailing `.`/space.
- [x] **T023 (CF-17) [P]** `operats.cpp` `DownloadOneFile`: on cancel/error for a
  freshly created (`CREATE_ALWAYS`) target, delete the partial file.

## Phase D — Path correctness

- [x] **T030 (CF-7)** `fs.cpp` `ExecuteOnFS`/`GetFullName` up-dir handling: build
  the parent/navigation path from the FS object's full `Path` (large buffer)
  instead of the `MAX_PATH`-limited `GetCurrentPath`.
- [x] **T031 (CF-26) [P]** `fs.cpp:180,375`: build the remote path from
  `SFTPRealName(&file)` rather than the sanitized panel `Name`.

## Phase E — Credential / key Unicode & format

- [x] **T040 (CF-8)** `dialogs.cpp:98,748,786`: read password/passphrase edits via
  `GetDlgItemTextU8` (placeholder logic stays ASCII).
- [x] **T041 (CF-9)** `session.cpp`: use `SplU8ToWAlloc` + `GetFileAttributesW`
  for the key-file existence check; load the key via `CreateFileW` +
  `libssh2_userauth_publickey_frommemory` so non-ASCII key paths work.
- [x] **T042 (CF-10) [P]** `keyload.cpp` `KeyFormatSupported`: return FALSE for
  `kfPuTTY` with a dedicated "convert to OpenSSH format" reason string.

## Phase F — Secret hygiene & robustness

- [x] **T050 (CF-11)** `dialogs.cpp:752,787`: exclude `SFTP_SECRET_PLACEHOLDER`
  from the "typed a new secret" test.
- [x] **T051 (CF-12) [P]** `fs.cpp` `CopyOrMoveFromDiskToFS`: also compare the
  target FS `user` (not just host/port).
- [x] **T052 (CF-14) [P]** `fs.cpp`: wipe `g_PendingParams` / connect-plaintext
  globals on every connect exit path (including cancel).
- [x] **T053 (CF-13) [P]** `session.cpp` `ListDir`: NUL-terminate `longEntry`
  after `libssh2_sftp_readdir_ex`.
- [x] **T054 (CF-15) [P]** `fs.cpp` `DecryptInto`: fail with an error when the
  decrypted secret would be truncated (`strlen(plain) >= outSize`).
- [x] **T055 (CF-18) [P]** `fs.cpp:868-870`, `operats.cpp:19-33`: check the
  `MultiByteToWideChar` return; abort on 0 (no `CreateFileW` on garbage).
- [x] **T056 (CF-19) [P]** `operats.cpp`, `listing.cpp`, `session.cpp`,
  `hostkeys.cpp`: NULL-check `_strdup`/allocation results; skip/fail the item.
- [x] **T057 (CF-21) [P]** `session.cpp:275-279`: fail closed when
  `libssh2_hostkey_hash` returns NULL (no trust of an unverifiable key).
- [x] **T058 (CF-23) [P]** `fs.cpp` `ViewFile`: replace the function-`static`
  64 KB transfer buffer with a call-local (heap/stack) buffer.
- [x] **T059 (CF-28) [P]** `fs.cpp`: null `CurrentListing` when the listing is
  released.
- [x] **T060 (CF-20)** `fs.cpp`: prompt for a passphrase (reuse `ShowPasswordPrompt`
  with `IDS_ENTERPASSPHRASE`) when key auth is chosen and no passphrase is saved.

## Phase G — Build & test

- [x] **T070** Debug x64 build clean (`build.cmd`) — plugin `sftp.spl` +
  `english.slg`.
- [x] **T071** `test\build_and_run.cmd` (sftputils unit tests) stays green.
- [x] **T072** Live handshake before/after via `sftp_smoke` (Phase A T003) +
  legacy-config non-regression.
- [x] **T073** Release x64 build if the running-exe lock allows; else note the
  LNK1104 pitfall.
- [x] **T074** Update memory / feature docs; final report.

## Deferred (documented in research.md, not implemented)

CF-D1 Ed25519/curve25519 · CF-D2 `\\?\` local long paths · CF-D3 IDN hostnames ·
CF-D4 UTF-8-boundary truncation · CF-D5 Disconnect stall (dead path) · CF-D6
SetMTime 2038 · CF-D7 `.slg` format-string validation · CF-D8 no-master-password
scrambling.
