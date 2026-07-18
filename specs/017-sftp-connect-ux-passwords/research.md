# Research: SFTP Connect Window — Consolidated Audit (Feature 017)

Three parallel audits (A password flow, B UX, C crypto/keys) plus the
orchestrator's own trace. Full per-agent detail in `audit/A.md`, `B.md`, `C.md`.
All three independently converge on the same password root causes.

## R1 — Password persistence bugs (the reported "password not saved")

Confirmed by A, B, and C. Capture/encrypt (`dialogs.cpp:618-640`) and
consume/decrypt (`fs.cpp:26-67`, `session.cpp:375-376`) are correct; the crypto
itself is correct (C: `enc==FALSE` still yields a valid scrambled blob; no
gating bug; no leak/double-free; scramble↔AES round-trips). The defects are in
getting the blob onto the *persistent* model and in the save/load round-trip:

- **P1 — QuickConnect never persisted.** `SaveConfiguration` (sftp.cpp:548-605)
  and `LoadConfiguration` (:464-546) handle only `Config.Bookmarks`;
  `Config.QuickConnect` is never written/read, though `EncryptPasswords` already
  re-encrypts it (:292-293) — proving intent. A quick-connect saved password
  dies on restart. **Fix**: `SaveServer`/`LoadServer` of `Config.QuickConnect`
  under a new `"QuickConnect"` subkey.
- **P2 — QuickConnect blob not reused, then clobbered (in-session).** The
  "reuse stored blob when password field empty" branch (dialogs.cpp:643-649 /
  :674-679) fires only for `selectedBookmark != NULL`, but Connect passes
  `bookmark == NULL` for quick-connect (:839); the password edit was blanked on
  load (:570). So on a second connect `d->Result->EncryptedPassword` is NULL and
  `Config.QuickConnect.CopyFrom(d->Result)` (:843) overwrites the good blob with
  NULL. **Fix**: pass `bookmark != NULL ? bookmark : &Config.QuickConnect` as the
  reuse fallback.
- **P3 — Edits/password for a selected bookmark are never written back.**
  Connect writes to the model only for quick-connect (`if (bookmark == NULL)
  Config.QuickConnect.CopyFrom(...)`, :842-843); a selected bookmark's
  `EncryptedPassword`/`SavePassword` are never updated, so `SaveConfiguration`
  serializes the unchanged (empty) bookmark. **Fix**: commit the current fields
  back onto the selected bookmark, **preserving `ItemName`** (which
  `ConnectReadFields` nulls via `Set` at :605) — merge, not blind `CopyFrom`.

## R2 — Bookmark lifecycle UX (the "muddled" complaint)

Agent B, confirmed by orchestrator. The dialog can load a bookmark into the
fields but has **no way to commit field edits back** — the single structural gap
behind both the muddle and P3.

- No explicit **Save/Update**; "New" (snapshot fields as a new named entry) is
  the only persist path → editing an existing bookmark is effectively impossible.
- **`IDB_COPYBOOKMARK` (623) is a dead control** — ID declared, no template
  control, no handler.
- Quick-connect is an invisible mode (no list row) → user can't tell which entry
  they're editing (FTP sister plugin shows a literal "Quick Connect" row).
- Delete has no confirmation; double-click doesn't connect; selecting a bookmark
  silently discards field edits; Cancel doesn't undo New/Rename/Remove (they
  mutate live `Config.Bookmarks`).
- Declared-but-dead: `IDM_SRVCONTEXTMENU`/`2` context menus; no Move Up/Down.
- Model fields with no UI: `TargetPanelPath`, per-bookmark keepalive,
  `UseCompression`.

## R3 — Keys & crypto (verification)

Agent C. **Crypto/storage is correct** once P1 lands: `SaveServer`/`LoadServer`
pick `PasswordE` vs `PasswordS` value names via `IsPasswordEncrypted` and
`DecryptPassword` handles both; blob malloc/`SalamanderGeneral->Free` discipline
consistent; re-encrypt-on-master-change and the host-key store round-trip
correctly and are not confused with password storage. Passphrase trio is handled
in lockstep with the password trio. Two auth-flow (not storage) defects found:

- **K1 — dead key-format check.** `DetectKeyFormat`/`KeyFormatSupported`
  (keyload.cpp:8-53) are never called; `session.cpp:343-345` hands any file to
  libssh2, so a non-key / unsupported file fails with a cryptic error instead of
  the intended clear message. **Fix (in scope)**: call them in the saPrivateKey
  branch before libssh2 and emit the clear message on `kfUnknown`.
- **K2 — no passphrase prompt parity.** Password auth prompts on an empty secret
  (fs.cpp:96-102); private-key auth never prompts for a missing passphrase.
  **Deferred (documented follow-up)**: the safe form is a re-prompt on
  `crKeyUnlock` (a retry loop) or reliable encrypted-key detection; both are
  auth-flow changes not verifiable without a live server, and mis-implementing
  risks spurious prompts / broken key auth. Storage of the key file + passphrase
  is unaffected.
- Minor (noted): `CSFTPServer::Set` doesn't reset blobs (latent; the commit
  helper avoids relying on it).

## R4 — Implementation decision (scope)

Deliver the user's explicit asks with bounded, buildable risk (the SFTP dialog
cannot be interactively clicked here — no live server — so verification is
build + registry round-trip + code review):

- **Fix P1, P2, P3** (password storage) — the reported bug, fully.
- **UX**: explicit **Save** button (commit fields → selected entry, incl.
  encrypted password); wire **Duplicate** (`IDB_COPYBOOKMARK`); visible
  **"Quick Connect"** row; **Delete confirmation**; **double-click = Connect**;
  clear labels. Connect also commits the current fields to the selected entry
  (matches "I connected with these settings, so remember them") — so a
  save-password-on-connect persists.
- **Key**: wire **K1** (clear message for an unsupported/non-key file).
- **Defer (documented)**: temp-staging so Cancel undoes structural edits; Move
  Up/Down; context menus; Advanced fields (compression/keepalive/target panel);
  **K2** passphrase prompt/retry. None block the reported problems.

### R5 — Stored-secret indicator (user feedback, follow-up in same feature)

After the password fix worked, the user noted the password field is blank when a
bookmark with a saved password is selected — correct (it never round-trips the
plaintext) but not visibly obvious that a password IS stored. Added: when an
entry has a stored secret, the password/passphrase field shows a **fixed-length
placeholder** ("********", 8 bullets) — visibly non-empty but NOT the real length
(no length leak). The placeholder is never read as a new secret: a per-field
**Dirty flag** (set on EN_CHANGE, cleared during programmatic load via a
SuppressDirty guard) means the field is treated as a newly typed password only
when the user actually edits it; otherwise the stored blob is reused/kept. On
focus, the placeholder is selected (EM_SETSEL) so the first keystroke replaces
it. Relies solely on the Dirty flag (no content compare) so a literal
"********" password still saves.

Commit-semantics note: structural actions (New/Duplicate/Rename/Delete/Save)
take effect immediately as explicit user actions (existing model); Cancel
discards uncommitted **field** edits. This is the current behavior made
coherent by adding the missing explicit Save — a full atomic-staging model is
the documented follow-up.
