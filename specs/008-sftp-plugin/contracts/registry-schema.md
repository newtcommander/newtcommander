# Contract: SFTP Plugin Registry Schema

**Feature**: `008-sftp-plugin` | Storage: host-managed plugin key (regKeyName `"SFTP"`)

All persistence goes through `CPluginInterface::LoadConfiguration` /
`SaveConfiguration` (host passes `HKEY` + `CSalamanderRegistryAbstract*`).
The plugin never opens registry keys directly. Layout mirrors the FTP
plugin's conventions so behavior (restart survival, master-password
re-encryption) is identical.

```
<plugin key "SFTP">
│   Version                REG_DWORD   ConfigVersion for upgrade logic
│   Connect Timeout        REG_DWORD   seconds (default 20)
│   Operation Timeout      REG_DWORD   seconds (default 30)
│   Keep Alive Send Every  REG_DWORD   seconds (default 60)
│   Keep Alive Stop After  REG_DWORD   minutes (default 30)
│   Connect Attempts       REG_DWORD   default 3
│   Delay Connect Retries  REG_DWORD   seconds (default 10)
│   Column View            REG_DWORD   0 = Unix rights (default), 1 = attribute-style   [FR-021]
│   Show Octal             REG_DWORD   bool (default 1)                                  [FR-018]
│   Resume Overlap         REG_DWORD   bytes re-read on resume (FTP parity)
│   Resume Min File Size   REG_DWORD   below this, restart instead of resume
│   Enable Logging         REG_DWORD   bool (default 1)                                  [FR-032]
│   Log Max Size           REG_DWORD   KB
│   Max Closed Con. Logs   REG_DWORD
│   Last Bookmark          REG_DWORD   0 = quick connect
│   (dialog placement values)
│
├── Bookmarks\                          ordered, one numbered subkey per profile  [FR-007]
│   ├── 1\
│   │     Name              REG_SZ      ItemName
│   │     Address           REG_SZ      host (UTF-8)
│   │     Port              REG_DWORD   default 22
│   │     User              REG_SZ
│   │     Auth Method       REG_DWORD   0 password, 1 private key
│   │     PasswordE         REG_BINARY  AES blob (master password active)   ─┐ at most one
│   │     PasswordS         REG_BINARY  scrambled blob (no master password) ─┘ present
│   │     Save Password     REG_DWORD   bool; if 0, neither Password* value exists
│   │     Key File          REG_SZ      private-key path (Auth Method = 1)
│   │     PassphraseE / PassphraseS  REG_BINARY  key passphrase blobs (same rules) [FR-004]
│   │     Save Passphrase   REG_DWORD
│   │     Initial Path      REG_SZ      UTF-8 remote path (empty = home)
│   │     Target Panel Path REG_SZ      optional local path for other panel
│   │     Keep Alive Send Every / Stop After   REG_DWORD  per-profile overrides (optional)
│   │     Use Compression   REG_DWORD   bool, default 0 (stretch FR-026)
│   └── 2\ …
│
├── Quick Connect\                      same value set as one bookmark subkey
│
└── Known Hosts\                        TOFU trust store            [FR-006, clarification #3]
    ├── 1\
    │     Host              REG_SZ      normalized lowercase
    │     Port              REG_DWORD
    │     Key Type          REG_SZ      e.g. "ssh-ed25519"
    │     Public Key        REG_SZ      base64 of full public-key blob (exact-match comparison)
    │     Fingerprint       REG_SZ      cached "SHA256:<base64>" display form
    │     Added             REG_BINARY  FILETIME
    └── 2\ …
```

## Invariants

1. **No plaintext secrets, ever.** Only password-manager blobs
   (`*E`/`*S` values) are written; `Save Password/Passphrase = 0` ⇒ the
   corresponding blob values are absent (US6-3).
2. **Master-password events**: on `PME_MASTERPASSWORD*`, all `*E`/`*S`
   blobs in Bookmarks + Quick Connect are re-encrypted in one pass
   (FTP `EncryptPasswords` pattern); a blob that cannot be decrypted
   prompts per FTP's `EnsurePasswordCanBeDecrypted` flow.
3. **Trust store updates only via user action** (accept-and-store or
   explicit replace after mismatch warning). "Connect once" never writes.
4. **Missing/corrupt values degrade to defaults** — a damaged bookmark
   loads with empty secret and prompts; it never blocks plugin load.
5. **`Version`** gates one-way upgrades of this schema; unknown future
   versions load read-only-safe defaults rather than destroying data.
6. UTF-8 in `REG_SZ`: paths/names are stored as UTF-8 strings
   (interface-104 convention, consistent with feature 004).
