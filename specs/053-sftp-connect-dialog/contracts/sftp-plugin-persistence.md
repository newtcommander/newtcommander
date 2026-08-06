# Contract: What the SFTP Plugin Stores — and What It Must Not

**Feature**: 053-sftp-connect-dialog · Phase 1 artifact
**Amends**: feature 017, which introduced persistence of the Quick Connect
entry *including its saved password*. That decision is reversed here.

## 1. Persisted plugin data (after this feature)

| Stored under the plugin's config key | Contents | Secrets? |
|---|---|---|
| options (timeouts, keepalive, retries, column view, octal display, resume, logging) | plain values | no |
| `Bookmarks\<n>` | one entry per bookmark: name, address, port, user, auth method, key file, initial path, save-secret flags, and the encrypted password/passphrase blobs **when the user opted in** | yes, by explicit opt-in |
| `KnownHosts\<n>` | host-key trust records | no |
| last-used entry marker | index of the entry the dialog seeds next time | no |
| `Version` | config version | no |
| ~~`QuickConnect`~~ | **removed — never written again** | **never** |

## 2. The Quick Connect contract

- Quick Connect is a **transient** entry. Nothing about it is written to
  persistent storage: not the address, port, user, key file, initial path, auth
  method, save-flags, and above all **no password or passphrase**.
- Its stale subtree from earlier versions is deleted, not read. The delete
  already happens on every configuration save (the subtree is cleared before
  being rewritten); dropping the rewrite turns that existing call into the
  purge, so no migration code is added.
- A secret typed for a Quick Connect session exists only in the transient
  plain-text buffers used for that connection attempt. It is never handed to
  the password manager, and the user is never prompted to unlock a master
  password on Quick Connect's behalf.
- The options to save a password or passphrase are unchecked and **disabled**
  while Quick Connect is the selected entry, as is Save — Quick Connect has
  nothing to save into (research D8).
- Selecting Quick Connect presents empty fields, every time, in the same
  session or after a restart.

## 3. The bookmark contract (unchanged except for validation timing)

- A bookmark requires a **non-empty name** and nothing else. Address, port,
  user, key file and path may all be empty; the entry round-trips through
  storage in that state.
- Address and port are required only when the user attempts to **connect**.
  The connect-time checks and their existing messages are unchanged.
- Bookmarks keep their save-password/save-passphrase behaviour and their
  encrypted blobs exactly as today.
- Text fields are normalized so "empty" is represented the same way in memory
  and after a reload (absent rather than an empty string), and the entry-list
  label treats an empty address as absent.

## 4. What this feature must not do

- Must not change any stored format that stays (bookmarks, known hosts,
  options) — the only storage change is a deletion.
- Must not migrate or convert Quick Connect data — it is dropped unread.
- Must not change the plugin interface version or any SDK signature.
- Must not reword any translated text (spec FR-008). Localized **geometry** may
  change; localized **wording** may not.
