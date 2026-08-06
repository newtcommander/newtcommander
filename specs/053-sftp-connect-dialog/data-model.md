# Data Model: SFTP Connection Entries and Dialog Geometry

**Feature**: 053-sftp-connect-dialog · Phase 1 artifact

## Entity: connection entry (`CSFTPServer`)

One class serves two roles. After this feature the roles differ in **lifetime**,
not in shape.

| Field | Bookmark | Quick Connect (after) |
|---|---|---|
| name (`ItemName`) | required, non-empty — the entry's identity | always absent |
| address | optional; required only at connect | typed per use, discarded |
| port | defaults to 22; validated only at connect | 22 on every open |
| user | optional | typed per use, discarded |
| auth method | persisted | password auth on every open |
| key file, initial path | optional | typed per use, discarded |
| save-password / save-passphrase flags | persisted, user's choice | **forced FALSE**, controls disabled |
| encrypted password / passphrase blobs | persisted when opted in | **never created** |
| target panel path, keepalive overrides, compression | as today | reset on every open |

**Lifetime rules**
- A bookmark lives in the plugin configuration and survives restarts.
- Quick Connect lives only for as long as the dialog is open. It is reset to
  constructor defaults when the dialog opens, and cleared again once a
  connection request has been handed on.
- "Empty" is represented consistently as *absent* (no stored value), not as an
  empty string, so an entry looks the same before and after a reload.

**Validation rules**
| Rule | When it applies |
|---|---|
| bookmark name must be non-empty | creating, duplicating, renaming a bookmark |
| address must be non-empty | **connecting only** |
| port must be 1–65535 | **connecting only**; elsewhere an invalid/blank port becomes 22 |
| everything else | never required |

**State transitions**

```
dialog opens        -> Quick Connect reset to defaults (blank, port 22, no secrets)
row selected        -> fields loaded from that entry; save-secret controls and Save
                       enabled only for bookmarks
New (named)         -> current fields snapshotted into a new bookmark; empty is fine
Save                -> fields written into the selected bookmark; empty is fine;
                       unavailable while Quick Connect is selected
Connect             -> address/port validated here; on success the request is handed
                       on and the last-used marker updated (only after validation)
dialog closes       -> Quick Connect holds nothing worth keeping; nothing persisted
configuration saved -> bookmarks, known hosts, options written; the QuickConnect
                       subtree is deleted and never rewritten
config version < 2  -> one forced purge-only save removes a stale QuickConnect subtree
```

## Entity: stored configuration

See [contracts/sftp-plugin-persistence.md](contracts/sftp-plugin-persistence.md)
for the authoritative table. Summary of the change: the `QuickConnect` subtree
is removed; every other stored item keeps its format and meaning. Config version
goes 1 → 2 purely to mark "the stale subtree has been purged".

## Entity: dialog geometry (per language)

Two representations of the same thing, and the second wins at build time:

| Representation | Where | Scope |
|---|---|---|
| English dialog template | `src/plugins/sftp/lang/lang.rc2` — `DIALOGEX` size plus each control's `x, y, cx, cy` | English module, and the structural baseline every language is derived from |
| Per-language geometry + text | `translations/<lang>/sftp.slt` — one row per control: `id, x, y, cx, cy, flag, "text"` | that language's built module; **overrides the template** |

**Invariant this feature establishes**: for every control in every shipped
language, `estimate_width(text) <= cx` — the same predicate the build's widener
uses (`tools/translate/layout.py`). Today 124 controls violate it.

**Change rules**
- Structural geometry (dialog size, control positions, the minimum width of a
  label column) changes in the English template first.
- Per-language widths are then re-derived from that template, with the widener
  growing each text control into the newly created free space.
- **Texts are never touched** (spec FR-008) — verified by diffing the quoted
  strings of every `.slt` before and after the refresh.
- Row count, row order and control ids must not change: `.slt` import is
  strictly positional and rejects the whole file otherwise. No control is added
  or removed by this feature.
- Input field widths are a floor, not a target: they may grow, never shrink
  (FR-011).

## Measured geometry targets

The plugin defines nine dialogs. Measured across the eight shipped languages
(`estimate_width` vs each control's `cx`), **26 distinct controls in six
dialogs** are too narrow; a seventh dialog overflows only at run time. Minimum
dialog widths come from row-packing: grow every text control to its
cross-language maximum, preserve the gaps, keep the 4-unit right margin.

| `.slt` section | Dialog | cx now | cx min | must grow | label column | input column |
|---|---|---|---|---|---|---|
| 500 | `IDD_CONNECT` | 340 | **408** | **+68** | 40 → **99** | x 165 → **226** |
| 510 | `IDD_PASSWORD` | 220 | 217 | 0 | — | — (prompt static must grow, see below) |
| 515 | `IDD_HOSTKEY` | 300 | **355** | **+55** | — | buttons: 76 → **123** / **87** |
| 520 | `IDD_CHMOD` | 210 | **257** | **+47** | 30 → **46** | x 40 → **55** |
| 530 | `IDD_CONFIG` | 260 | **350** | **+90** | 90/100/110 → **176** | x 130 → **185** |
| 540 | `IDD_SYMLINK` | 240 | **294** | **+54** | 50 → **107** | x 60 → **116** |
| 550 | `IDD_RENAME` | 240 | 237 | 0 | — | — |
| 560 | `IDD_LOGS` | 400 | 397 | 0 | — | — |
| 570 | `IDD_OWNERGROUP` | 220 | 219 | 0 | 108 → **127** | x 120 → **136** |

Worst individual controls, with the language that drives them:

| Dialog | Control | cx now | needs | driven by |
|---|---|---|---|---|
| connect | initial path label | 40 | 99 | French |
| connect | key-file label | 40 | 74 | Spanish |
| config | "show octal" checkbox | 118 | 208 | French |
| config | resume-overlap / summary-size labels | 100 | 176 | German / Dutch |
| config | log-size label | 70 | 139 | Czech |
| chmod | "set modification time" | 90 | 143 | Spanish |
| symlink | link-name label | 50 | 107 | Slovak |
| host key | "trust and save" button | 76 | 123 | French |

**Additional constraints the estimator cannot see** (research D10):
- the host-key "trust" button is also re-captioned at run time (~111 units), so
  it must satisfy the larger of the two;
- the password prompt static has a 2 × 206 = 412-unit budget for a message
  needing ~407 units *before* a file path is substituted — it must grow;
- the configuration dialog already contains a 44-unit control **overlap**
  (research D11) that the relayout must resolve.

**Fixed inputs**: 24 hand-curated Czech texts for this module (12 of them on the
controls being widened) win over machine output and are not negotiable — their
widths are part of the target, not something to work around.

**Accelerators**: zero duplicates in any shipped language; three pre-existing
collisions in the English template's connect dialog are left as-is (research
D12). No accelerator letter changes.
