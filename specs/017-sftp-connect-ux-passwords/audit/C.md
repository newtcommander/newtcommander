# Audit C — Private-key handling + password-manager crypto correctness

Feature 017 · SFTP plugin (`src\plugins\sftp\`). Audit only, no edits.
Scope: secret encryption/decryption round-trip, private-key loading,
host-key store, `session.cpp Authenticate`. Independent of agent A.

## Password-manager API contract (baseline)

From `src\plugins\shared\spl_gen.h:767-799` (and mirror `src\pwdmngr.h`):

- `EncryptPassword(plain, &blob, &size, encrypt)` — if `encrypt==TRUE` uses
  AES (caller must have entered master password); if `encrypt==FALSE` it
  *still* produces a valid **scrambled** blob. So a blob is ALWAYS created
  regardless of the `encrypt` flag (spl_gen.h:783-789). The returned buffer is
  on Salamander's heap → free with `SalamanderGeneral->Free()`.
- `DecryptPassword(blob, size, &plain)` — transparently handles both scrambled
  and AES blobs (the blob is self-describing). Buffer on Salamander's heap.
- `IsPasswordEncrypted(blob, size)` — TRUE iff AES. Used to decide the
  registry value name and whether a master-password prompt is needed at load.

The plugin does **not** use `IsPasswordSecure` anywhere (grep: only ftp/pwdmngr).
So the "blob never created because gated on `IsPasswordSecure`" hypothesis is
**not present** here.

## What is correct

- **Encrypt-then-store gating does NOT suppress blob creation.**
  `dialogs.cpp:630-635` computes `enc = IsUsingMasterPassword() &&
  IsMasterPasswordSet()`, falls back to `AskForMasterPassword` when a master
  password exists but isn't unlocked (`:631-632`), and then *always* calls
  `EncryptPassword(pwd,&blob,&size,enc)`. Even when `enc==FALSE` a scrambled
  blob is produced and stored. Same for passphrase (`:661-666`). This is the
  correct usage of the API — no gating defect.
- **Bookmark scramble↔AES round-trip is symmetric.**
  `SaveServer` (`sftp.cpp:425-436`) picks value name `SRV_PASSWORDE` when
  `IsPasswordEncrypted()` else `SRV_PASSWORDS` (same for passphrase).
  `LoadServer` (`sftp.cpp:402-407`) reads `…E` first, falls back to `…S`.
  `DecryptInto` (`fs.cpp:34-46`) prompts for the master password only when the
  blob is AES and MP is not yet set, then `DecryptPassword`. A blob written
  scrambled loads scrambled; written AES loads AES. Round-trips both ways.
- **Blob memory discipline is consistent — no leak / double-free found.**
  Local `CSFTPServer` blobs are `malloc`/`free` (`sftp.cpp:209-223,182-194`);
  every password-manager buffer is released with `SalamanderGeneral->Free`
  (`dialogs.cpp:638,669`; `sftp.cpp:278,281`; `fs.cpp:45`). `SetBlob` frees the
  prior blob before replacing. `ReEncryptBlob` frees the old `malloc`'d blob,
  re-`malloc`s a copy of the PM buffer, then `Free`s the PM buffer
  (`sftp.cpp:266-278`); OOM path sets blob=NULL & size=0 consistently
  (`:270-277`). Plaintext buffers are `SecureZeroMemory`'d
  (`dialogs.cpp:641,672`; `fs.cpp:44,87,92`; `sftp.cpp:280`; `session.h:73-77`).
- **Re-encrypt-on-master-change path is wired correctly.**
  `PasswordManagerEvent` (`sftp.cpp:648-653`) chooses `encrypt` from
  PME_MASTERPASSWORDCREATED/CHANGED and calls
  `Config.Bookmarks.EncryptPasswords`, which re-encrypts every bookmark AND
  `Config.QuickConnect` (`sftp.cpp:284-294`). Uses Decrypt→Encrypt on the same
  PM, so it round-trips.
- **Host-key store is independent and round-trips.** Saved under
  `CFG_KNOWNHOSTS` with HK_HOST/PORT/TYPE/BLOB/FINGERPRINT
  (`sftp.cpp:583-604`), loaded symmetrically (`sftp.cpp:517-545`). It is NOT
  confused with password storage (different subkey, different fields). Only the
  `Added` FILETIME is not persisted — cosmetic. `keyload.cpp` and `hostkeys.cpp`
  contain no crypto correctness defects.
- **Password vs passphrase are routed to the right libssh2 calls.**
  `session.cpp:343-345` uses `Params.Passphrase` (NULL when empty) to unlock the
  key; `session.cpp:375-376` uses `Params.Password` for password auth;
  `:385-387` feeds `Params.Password` to keyboard-interactive. Correct mapping.

## Defects

### BUG 1 (High) — Quick-connect saved password is never persisted across sessions
`Config.QuickConnect` is written in memory by the dialog
(`dialogs.cpp:843 Config.QuickConnect.CopyFrom(d->Result)`) and is even
re-encrypted on master-password change (`sftp.cpp:292-293`), but
`SaveConfiguration` (`sftp.cpp:548-605`) and `LoadConfiguration`
(`sftp.cpp:464-546`) **never serialize `Config.QuickConnect`** — only the
scalar config, `Config.Bookmarks`, and the host-key store are saved/loaded.
Also `Config.LastBookmark` is persisted (`sftp.cpp:487,563`) and `0` means
"quick connect", so the intent to persist the quick-connect entry is clear, yet
its data (including `EncryptedPassword`) is dropped on exit.

*Scenario:* user leaves the bookmark list unselected (default, `LastBookmark==0`
seeds fields from QuickConnect — `dialogs.cpp:734-741`), types host/user/password,
checks "Save password", connects. The blob lives only in `Config.QuickConnect`
in RAM. On the next Salamander start it is default-constructed empty → password
gone. This matches the user report exactly for the quick-connect case.

*Fix:* in `SaveConfiguration` create a `"QuickConnect"` subkey and call
`SaveServer(registry, qcKey, &Config.QuickConnect)`; in `LoadConfiguration`
open it and `LoadServer(registry, qcKey, &Config.QuickConnect)` (mirroring the
bookmark loop). `SaveServer/LoadServer` already handle the blob E/S value names.

### BUG 2 (High) — Quick-connect stored blob is never reused and is overwritten with an empty one in-session
In `IDB_CONNECT`, `bookmark` is `NULL` for quick-connect because nothing is
selected in the list (`dialogs.cpp:826-838`, `sel == LB_ERR`). That `NULL` is
passed as `selectedBookmark` to `ConnectReadFields`
(`dialogs.cpp:839`). The password field was cleared when the entry was seeded
(`ConnectLoadServerToFields`, `dialogs.cpp:570`), so at connect `pwd[0]==0` and
the reuse branch — guarded by `selectedBookmark != NULL`
(`dialogs.cpp:643-648` for password, `:674-678` for passphrase) — does **not**
run. Therefore `d->Result` gets **no** `EncryptedPassword`. Then
`dialogs.cpp:843 Config.QuickConnect.CopyFrom(d->Result)` **overwrites** the
previously stored QuickConnect blob with the empty one.

*Effect:* even within a single session, a saved quick-connect password is never
re-applied on the next connect (it silently falls through to the
"prompt for password" path at `fs.cpp:96-102`) and the stored blob is destroyed.
Combined with BUG 1 this makes quick-connect "Save password" completely
non-functional.

*Fix:* pass the quick-connect entry as the reuse source, e.g.
`ConnectReadFields(hwnd, d->Result, bookmark != NULL ? bookmark : &Config.QuickConnect, TRUE)`
at `dialogs.cpp:839`. Additionally, guard `CopyFrom` (`dialogs.cpp:842-843`) so
it does not clobber a still-valid stored blob when no new secret was typed
(after the reuse fix this is automatically preserved because the blob is copied
into `d->Result`).

### BUG 3 (Medium) — Private-key format validation is dead code
`DetectKeyFormat` / `KeyFormatSupported` (`keyload.cpp:8-53`, declared
`keyload.h:25-30`) exist to reject unsupported key formats (notably `.ppk` v3)
with a clear message (`keyload.h:28-29`, FR-003). Grep confirms they are
**never called** anywhere in the plugin. `session.cpp Authenticate` hands the
path straight to `libssh2_userauth_publickey_fromfile_ex`
(`session.cpp:343-345`), which does not understand PuTTY `.ppk` at all → the
user gets a generic `crKeyUnlock`/`crAuthKey` error instead of the intended
"unsupported format" message.

*Fix:* before the `saPrivateKey` libssh2 call (or in `ConnectReadFields`/connect
setup), call `DetectKeyFormat(KeyFile)` + `KeyFormatSupported(fmt,&reason)` and,
on FALSE, surface `LoadStr(reason)` and abort with a dedicated result code.

### BUG 4 (Low-Medium) — Passphrase has no on-demand prompt; asymmetric with password
The password trio prompts when the secret is missing: `fs.cpp:96-102` shows
`ShowPasswordPrompt` when `AuthMethod==saPassword && Password[0]==0`. There is
**no equivalent** for `saPrivateKey`: if the key is passphrase-protected but the
passphrase was neither stored nor typed, `Params.Passphrase` is empty →
`session.cpp:345` calls libssh2 with `NULL` → decrypt fails → `crKeyUnlock`
error, no prompt. The passphrase trio (`SavePassphrase`/`EncryptedPassphrase`/
`ConnectPlainPassphrase`) is otherwise handled in lockstep with the password
trio in the dialog (`dialogs.cpp:651-680`) and in `FillParamsFromServer`
(`fs.cpp:64-67`), so this is the one place the two diverge.

*Fix:* mirror `fs.cpp:96-102` for `saPrivateKey` (prompt for a passphrase when
empty), or retry with a passphrase prompt on `crKeyUnlock`.

## Notes (structural, non-crypto-defect)

- **No way to update a saved bookmark's stored password.** The connect dialog
  offers only New / Rename (name only, `dialogs.cpp:787-805`) / Remove. Typing a
  new password on a *selected* bookmark and pressing Connect writes the blob
  only into the transient `d->Result` (used for that connect); `Config.Bookmarks[bi]`
  is never updated (`dialogs.cpp:818-845`). A bookmark's saved password can thus
  only be changed by deleting and recreating it. UX/persistence — overlaps agent B.
- **`CSFTPServer::Set` does not reset blobs** (`sftp.cpp:225-231`); only strings
  are freed via `SetString`. Latent only — all current callers pass freshly
  constructed objects (`new CSFTPServer` / stack `srv` in `fs.cpp:76`), so no
  live leak/stale-blob today. Would bite if a `CSFTPServer` were ever reused via
  `Set`.
- **`enc==FALSE` fallback under a master password is a mild security downgrade,
  not a functional bug.** If the user has a master password but cancels
  `AskForMasterPassword` (`dialogs.cpp:632`), the secret is stored *scrambled*
  rather than AES. It still round-trips (load side detects scramble and skips the
  MP prompt). Acceptable, but worth a conscious decision.
- **Host-key `Added` timestamp not persisted** (`sftp.cpp:595-599` vs
  `:530-534`) — cosmetic only.

## Bottom line

Crypto primitives (encrypt/decrypt, scramble↔AES symmetry, re-encrypt on
master-password change, blob memory management) are implemented **correctly**.
The reported "password not saved" is not a crypto defect but two persistence /
reuse defects specific to the **quick-connect** entry (BUG 1 + BUG 2); bookmarks
created via "New Bookmark" do persist their password correctly. Private-key
handling has a wired-out format check (BUG 3) and a missing passphrase prompt
(BUG 4).
