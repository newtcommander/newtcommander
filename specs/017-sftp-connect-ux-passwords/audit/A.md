# Audit A — Password / passphrase persistence (SFTP plugin, feature 017)

Scope: why a password the user typed and chose to **Save** is NOT remembered on
the next connect. Full chain: dialog capture → password-manager encrypt →
CSFTPServer blob → SaveConfiguration(registry) → LoadConfiguration → reconnect
decrypt. No source edited.

Files: `src\plugins\sftp\dialogs.cpp`, `sftp.cpp`, `fs.cpp`, `session.cpp`,
`sftp.h`, `session.h`.

---

## Data model (sftp.h)

- `CSFTPServer` = one bookmark **or** the quick-connect entry. Secret fields:
  `EncryptedPassword` (BYTE* blob) + `EncryptedPasswordSize` + `SavePassword`,
  and the passphrase trio (`EncryptedPassphrase`/`Size`/`SavePassphrase`).
  (`sftp.h:63-70`).
- `CSFTPConfig::Bookmarks` (list) + `CSFTPConfig::QuickConnect` (single) +
  `LastBookmark` (`sftp.h:127-130`). `LastBookmark==0` ⇒ quick-connect,
  `>0` ⇒ 1-based index into `Bookmarks`.

The connect dialog returns its result into a **transient throwaway**
`CSFTPServer srv` declared on the stack in `ConnectSFTPServer`
(`fs.cpp:76`), which is used only for the immediate connection and then
discarded. The only persistent state is `Config.Bookmarks[]` and
`Config.QuickConnect`.

---

## Step 1 — CAPTURE (dialogs.cpp, `ConnectProc` / `ConnectReadFields`)

Dialog seeds fields from quick-connect or the last bookmark in `WM_INITDIALOG`
(`dialogs.cpp:733-741`). The password edit is **always cleared** on load
(`dialogs.cpp:570`, passphrase `:573`); the Save checkboxes are set from the
model (`:571`, `:574`).

OK/Connect handler `IDB_CONNECT` (`dialogs.cpp:818-847`):
- Resolves the selected bookmark: `bookmark = Config.Bookmarks[bi]` and
  `Config.LastBookmark = bi+1` (`:830-835`); if nothing selected,
  `Config.LastBookmark = 0`, `bookmark = NULL` (`:837-838`).
- Calls `ConnectReadFields(hwnd, d->Result, bookmark, /*forConnect*/TRUE)`
  (`:839`).
- **Only** for quick-connect writes the result back to the model:
  `if (bookmark == NULL) Config.QuickConnect.CopyFrom(d->Result);` (`:842-843`).

`ConnectReadFields` (`dialogs.cpp:583-682`):
- `s->Set(NULL, host, port, user)` → **ItemName is forced to NULL** on the
  result (`:605`).
- `s->SavePassword = IsDlgButtonChecked(hwnd, IDC_SAVEPASSWORD);` (`:609`),
  `SavePassphrase` (`:610`).
- Reads the typed password from `IDE_PASSWORD` (`:623`).
- If a password was typed and `forConnect`, copies plaintext to the transient
  global `ConnectPlainPassword` (`:627`).
- If `s->SavePassword && pm != NULL`, encrypts and stores the blob **onto the
  result object** `s->EncryptedPassword` (`:628-640`, see Step 2).
- Else (password field empty), reuses a stored blob **only if
  `selectedBookmark != NULL`** (`:643-649`); passphrase mirror `:674-679`.

## Step 2 — ENCRYPT (dialogs.cpp)

`pm = SalamanderGeneral->GetSalamanderPasswordManager()` (`dialogs.cpp:618`).
`pm->EncryptPassword(pwd, &blob, &blobSize, enc)` then
`s->SetBlob(&s->EncryptedPassword, &s->EncryptedPasswordSize, blob, blobSize)`
(`:635-638`); passphrase mirror `:666-668`. `SetBlob` deep-copies onto the
heap (`sftp.cpp:209-223`). **The blob is correctly produced — but it is placed
on the transient `d->Result`, not on any persistent object** (except the one
quick-connect `CopyFrom` at `:843`).

## Step 3 — PERSIST (sftp.cpp `SaveConfiguration`)

`SaveConfiguration` (`sftp.cpp:548-605`): writes globals, then deletes+recreates
the `Bookmarks` key and writes each `Config.Bookmarks[i]` via `SaveServer`
(`:566-581`). `SaveServer` (`:416-441`) writes `SRV_SAVEPWD`
(`"Save Password"`, `:423`) and, when `srv->SavePassword && EncryptedPassword`,
the blob under `PasswordE`/`PasswordS` (`:425-430`); passphrase mirror
(`:431-436`).

**Defect:** `Config.QuickConnect` is **never written** anywhere in
`SaveConfiguration`. The only persisted per-server records are the bookmarks.

## Step 4 — RELOAD (sftp.cpp `LoadConfiguration`)

`LoadConfiguration` (`sftp.cpp:464-546`): reads globals, then bookmarks under the
`Bookmarks` key via `LoadServer` (`:491-514`). `LoadServer` (`:389-414`) reads
`SavePassword` (`:397-398`) and the blob (`:402-404`, `PasswordE` else
`PasswordS`); passphrase mirror (`:399-407`). This roundtrip is correct **for
bookmarks that actually hold a blob**.

**Defect (symmetric to Step 3):** `Config.QuickConnect` is **never read back**.

## Step 5 — CONSUME (fs.cpp / session.cpp)

`ConnectSFTPServer` (`fs.cpp:74-114`): `FillParamsFromServer(&srv, &g_PendingParams)`
decrypts `srv.EncryptedPassword` → `params->Password` when `srv->SavePassword`
(`fs.cpp:64-67`, `DecryptInto` `:26-47`). Then `ConnectPlainPassword` overrides
if present (`:84-88`), else it prompts (`:96-102`). The live session uses
`Params.Password` in `libssh2_userauth_password_ex` (`session.cpp:375-376`).
**The consume path is correct** — for the *current* connection it reads from
either the freshly-typed plaintext global or the decrypted blob of the transient
`srv`. The failure is purely that the **persistent** model never receives (or
never saves) the blob.

---

## DEFECTS

### BUG A1 — Quick-connect entry is never persisted to the registry
`SaveConfiguration` (`sftp.cpp:548-605`) and `LoadConfiguration`
(`sftp.cpp:464-546`) handle only `Config.Bookmarks`; `Config.QuickConnect` is
never saved or loaded. The dialog updates it in memory
(`dialogs.cpp:843 Config.QuickConnect.CopyFrom(d->Result)`), so a saved
quick-connect password survives only until Salamander exits.
That the omission is a bug (not a design choice) is confirmed by
`CSFTPServerList::EncryptPasswords`, which already re-encrypts
`Config.QuickConnect`'s blobs on master-password change
(`sftp.cpp:292-293`) — the author intended it to persist.
**Scenario:** quick-connect, type password, check Save, connect, exit
Salamander, relaunch → password gone.
**Fix:** add a dedicated `"QuickConnect"` subkey. In `SaveConfiguration` call
`SaveServer(registry, qcKey, &Config.QuickConnect)`; in `LoadConfiguration`
`LoadServer(registry, qcKey, &Config.QuickConnect)` (mirroring the bookmark
create/open pattern at `sftp.cpp:565-581` / `490-514`).

### BUG A2 — A password typed for a selected bookmark is never written back to that bookmark
In `IDB_CONNECT` the freshly typed+encrypted secret is stored on the transient
`d->Result`, and the write-back to the model happens **only for quick-connect**:
`if (bookmark == NULL) Config.QuickConnect.CopyFrom(d->Result);`
(`dialogs.cpp:842-843`). When a bookmark is selected (`bookmark != NULL`)
**nothing** copies `d->Result`'s `EncryptedPassword`/`SavePassword` (or the
passphrase trio) back onto `Config.Bookmarks[bi]`. `SaveConfiguration` then
serialises the *unmodified* bookmark (`SavePassword` still FALSE, no blob).
**Scenario:** select an existing bookmark, type password, check Save, connect →
`Config.Bookmarks[bi]` is untouched → next launch the bookmark still has no
password.
**Fix:** when `bookmark != NULL`, copy the updated secret fields
(`EncryptedPassword`+`EncryptedPasswordSize`, `SavePassword`,
`EncryptedPassphrase`+size, `SavePassphrase`) from `d->Result` back onto
`Config.Bookmarks[bi]`. **Must preserve `ItemName`** — `ConnectReadFields`
forces `d->Result->ItemName = NULL` (`dialogs.cpp:605`), so a blind
`CopyFrom(d->Result)` would erase the bookmark name.

### BUG A3 — Quick-connect stored blob is neither reused nor preserved on a second connect
The "reuse stored blob when the password field is empty" branch in
`ConnectReadFields` fires **only if `selectedBookmark != NULL`**
(`dialogs.cpp:643-649`; passphrase `:674-679`). For quick-connect,
`IDB_CONNECT` passes `bookmark == NULL` as `selectedBookmark`
(`dialogs.cpp:839`), and the password edit was cleared on load
(`dialogs.cpp:570`). So on a second quick-connect with the password left blank,
`d->Result->EncryptedPassword` stays NULL, and then
`Config.QuickConnect.CopyFrom(d->Result)` (`:843`) **overwrites the previously
stored blob with NULL** — destroying it even within the same session. The
subsequent connect finds no password and re-prompts (`fs.cpp:96-102`).
**Scenario:** quick-connect, type+save password, connect (works); reopen the
dialog, click Connect without retyping → prompted again; QuickConnect blob wiped.
**Fix:** in the `bookmark == NULL` branch pass `&Config.QuickConnect` as the
`selectedBookmark` fallback so the empty-password path reuses the stored
quick-connect blob before `CopyFrom` writes it back.

---

## NOTES

- The three bugs are independent and additive. A1+A3 together fully break the
  quick-connect "Save password" case (A3 loses it in-session, A1 loses it across
  sessions); A2 breaks the bookmark "Save password" case. All three reproduce
  the reported "typed a password, chose save, connected — not remembered".
- What already works: (a) an existing bookmark that *already* holds a saved blob
  reconnects fine — `ConnectReadFields` reuses it (`dialogs.cpp:643-649`) and the
  blob is already in `Config.Bookmarks[bi]`, so `SaveConfiguration` re-persists
  it; (b) creating a **new** bookmark via `IDB_NEWBOOKMARK` *after* typing a
  password captures the blob onto the new object
  (`dialogs.cpp:775 ConnectReadFields(...,FALSE)`; added at `:779`), which does
  persist. Only *setting/changing* a password through the Connect button is
  broken.
- `SavePassword` checkbox state for a bookmark is subject to the same A2 root
  cause: toggling it via the Connect button is not written back to the bookmark
  object.
- Because `ConnectReadFields` always nulls `ItemName` (`dialogs.cpp:605`), any
  write-back fix for A2 must merge secret fields rather than replace the whole
  object.
- The plugin does not force a config save after a successful connect; it relies
  on Salamander's normal SaveConfiguration. That is standard and not itself a
  bug, but it means the in-memory model must be correct at connect time — which
  A2/A3 violate.
