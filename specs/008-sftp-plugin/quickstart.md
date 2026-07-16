# Quickstart: Building & Verifying the SFTP Plugin

**Feature**: `008-sftp-plugin` | Maps to success criteria SC-001…SC-010 in [spec.md](spec.md)

## Prerequisites

- Windows 11, VS2022 with C++ Desktop workload (repo standard).
- `OPENSAL_BUILD_DIR` set (or default `.\build\`).
- A **real OpenSSH server** to test against. Easiest options:
  - **WSL2** (Ubuntu): `sudo apt install openssh-server && sudo service ssh start`
    → connect to the WSL IP (`hostname -I`).
  - **Windows OpenSSH Server** feature (Settings → Optional Features) —
    fine for auth/transfer tests, but permissions/owner semantics are
    Windows-flavored; prefer WSL/Linux for US2 checks.
- Test keys (on the server, in WSL):
  ```bash
  ssh-keygen -t ed25519 -f ~/k_ed25519 -N "pass1"       # OpenSSH container + passphrase
  ssh-keygen -t rsa -b 3072 -m PEM -f ~/k_rsa_pem -N "" # classic PEM
  ssh-keygen -t rsa -b 3072 -f ~/k_rsa_ossh -N "pass2"  # RSA in OpenSSH container
  cat ~/k_*.pub >> ~/.ssh/authorized_keys
  ```

## Build

```batch
build.cmd            :: incremental Debug x64 — sftp builds because plugins.cfg has sftp=on
build.cmd full       :: also regenerates plugins\plugins.ver so the plugin auto-registers
```

Output: `%OPENSAL_BUILD_DIR%salamander\Debug_x64\plugins\sftp\sftp.spl`
(+ `lang\english.slg`). Run Salamander from the build dir; the plugin
appears in Plugin Manager and as an item in the Alt+F1/F2 menu.

## Verification walkthrough (≈ the DoD)

| # | Scenario | Pass criteria | SC |
|---|---|---|---|
| 1 | Plugins menu → SFTP → Connect; new bookmark host/port/user + password; Connect | First-connect **fingerprint dialog** appears; after accept, home dir lists; UTF-8 names (create `žluťoučký_кůň_日本語.txt` on server) render correctly | SC-001/003, US1 |
| 2 | Reconnect to same server | No fingerprint prompt (trusted) | SC-003 |
| 3 | Change server host key (`ssh-keygen -A` after deleting `/etc/ssh/ssh_host_*`), reconnect | Prominent mismatch warning; declining refuses connection; explicit accept replaces stored key | SC-003, US4 |
| 4 | Bookmark with `Save password`; exit Salamander; relaunch; connect | Connects with zero typing; also verify with Master Password enabled in host config | SC-004, US6 |
| 5 | Key auth: bookmark with `k_rsa_pem` (no passphrase), then `k_ed25519` (passphrase `pass1`, test both prompt and saved passphrase), then `k_rsa_ossh` | All connect; wrong passphrase gives "key could not be unlocked" (not "server rejected") | SC-001, US5 |
| 6 | Panel shows Rights/Owner/Group columns; prepare on server: `chmod 4755` binary, `chmod 1777` dir, symlink, broken symlink | `rwsr-xr-x`, `rwxrwxrwt` shown; owner/group correct; symlink target visible; broken link marked | SC-002, US2 |
| 7 | Ctrl+F2 (Change Attributes) on a file: 0644 → 0755 | Server shows new mode (`ls -l`); refreshed panel matches | SC-002, US2 |
| 8 | Round-trip: upload a tree with non-ASCII names, rename remotely, download back; `fc /b` compare | Byte-identical contents, names preserved | SC-005 |
| 9 | F3 on a remote text file | Opens in internal viewer via temp cache | FR-031 |
| 10 | Transfer a ≥4 GB file both directions; kill network mid-transfer (`wsl --shutdown` or unplug); retry | Resume offered, completes, size+hash match; reconnect returns to same directory | SC-007/008, US7 |
| 11 | Directory with 10k entries (`touch f{1..10000}`) | Lists without UI freeze; ESC cancels cleanly | SC-007 |
| 12 | Logs window (plugin menu) | Shows connect/auth/host-key/operation lines; **no secrets in log** | FR-032 |
| 13 | FTP plugin smoke: connect/list/download against any FTP server | Unchanged behavior | SC-009 |
| 14 | `.ppk` v3 file as key | Clear rejection naming supported formats | FR-003 |

## Runtime SSH/SFTP smoke test (no GUI needed)

`test\sftp_smoke.c` is a standalone console program that runs the exact libssh2
API sequence `CSFTPSession` uses (handshake → host-key hash → password auth →
SFTP init → opendir/readdir with permissions → download) against a real OpenSSH
server, linking the plugin's compiled libssh2 objects. It proves the libssh2 +
WinCNG transport works at runtime — which compilation alone cannot.

Verified on 2026-07-17 against OpenSSH in WSL2 (both Debug and Release builds):
handshake, host-key SHA-256, **password authentication**, directory listing with
correct Unix permissions (`-rwsr-xr-x` setuid, `drwxrwxrwt` sticky, `lrwxrwxrwx`
symlinks), a **UTF-8 filename** (`žluťoučký_кůň_日本語.txt`), and file download —
all passed. This covers the runtime core of SC-001, SC-002, and SC-005.

Build & run:
```
cl /MD /DLIBSSH2_WINCNG /I<dep>\libssh2\include test\sftp_smoke.c <intermediate>\libssh2_*.obj ws2_32.lib bcrypt.lib crypt32.lib
sftp_smoke <host> <port> <user> <password> <remotedir>
```

## Dev test harness (pure units)

Key-loader and rights-formatter units (OpenSSH container parser, bcrypt
KDF, ed25519 test vectors, `mode → "drwxr-xr-x"` mapping, .ppk v2) are
exercised by a small console harness (dev-only project, not shipped,
not in `plugins.cfg`) — details defined in tasks phase (SC-010).

## Troubleshooting

- `sftp.spl` missing → check `plugins.cfg` has `sftp=on`; every plugin
  dir must be listed (007 policy, build stops otherwise).
- Connection issues → open the plugin's Logs window first; server side
  `journalctl -u ssh` / `/var/log/auth.log`.
- WSL2 IP changes per boot — re-check `hostname -I`.
