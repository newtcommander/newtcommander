# SFTP Connect Window — Audit Brief (Feature 017)

You are auditing the **SFTP plugin** of Open Salamander (WinAPI C++). Repo:
`E:\Projects\salamander`, plugin at `src\plugins\sftp\`. The plugin IS
implemented (features 008/009). Read this brief, then audit your assigned area.

## Context (the data model)

- `sftp.h`: `CSFTPServer` = one saved connection (bookmark) OR the quick-connect
  entry. Fields: `ItemName` (NULL=quick-connect), `Address`, `Port`, `UserName`,
  `AuthMethod` (saPassword/saPrivateKey), `EncryptedPassword` (BYTE* blob) +
  `EncryptedPasswordSize` + `SavePassword`, `KeyFile`, `EncryptedPassphrase` +
  `EncryptedPassphraseSize` + `SavePassphrase`, `InitialPath`, `TargetPanelPath`,
  keepalive/compression.
- `CSFTPConfig`: `Bookmarks` (CSFTPServerList), `QuickConnect`, `LastBookmark`,
  timeouts, `ConnectDlgPlacement`. Persisted under the plugin registry key via
  `CPluginInterface::LoadConfiguration`/`SaveConfiguration` (sftp.cpp).
- `dialogs.h`: `ShowConnectDialog(parent, organizeMode, result)` is the connect
  window (Ctrl+Shift+S). `ConnectPlainPassword`/`ConnectPlainPassphrase` globals
  carry a just-typed secret the user chose NOT to store.
- Secrets are stored as **password-manager blobs** (Salamander's
  CSalamanderPasswordManagerAbstract via SalamanderGeneral), never plaintext at
  rest. `CSFTPServerList::EncryptPasswords` re-encrypts on master-password change
  (`PasswordManagerEvent`).

## The two user-reported problems

1. **Bookmark UX is muddled**: in the connect window it is not clear how a
   bookmark is created / edited / saved / deleted. "Záložka musí mít jasné
   funkce pro vytvoření, editaci, uložení" — the create/edit/save actions must
   be explicit and discoverable. Right now it feels mixed together.
2. **Password not saved**: the user typed a password, chose "save", connected —
   and on the next connect the password was NOT remembered. The persistence of
   `SavePassword`/`EncryptedPassword` is broken somewhere in the chain:
   dialog capture → password-manager encrypt → CSFTPServer blob →
   SaveConfiguration(registry) → LoadConfiguration → reconnect decrypt.

## Rules for all agents

- **Do NOT edit source.** Audit only. Cite `file:line` for every claim.
- Trace the FULL chain end to end; do not stop at the first suspicious spot.
- For a bug, state: the exact defective line(s), WHY it fails (concrete
  scenario), and the precise fix (what to change to what).
- Distinguish a real bug from a mere style issue.
- Write findings to `specs\017-sftp-connect-ux-passwords\audit\<LETTER>.md`.

## Output (RETURN to orchestrator — compact)

- `BUGS:` bullet list, each `file:line — one-line defect — one-line fix`
- `UX_FINDINGS:` (UI agent) concrete redesign points, prioritized
- `NOTES:` anything structural the orchestrator must know
- Keep the returned summary tight; put detail in your `<LETTER>.md`.
