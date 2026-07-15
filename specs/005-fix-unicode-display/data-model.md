# Data Model: Correct Display of Unicode File Names in Dialogs and Text Fields

**Feature**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md)

This feature changes no business data. Its "data model" is (a) the
encoding contract for text crossing between the program and Win32 UI
controls, and (b) the audit inventory that FR-005 requires.

## Encoding contract (inherited from feature 004, R1)

| Layer | Encoding | Authority |
|-------|----------|-----------|
| Internal `char*` name/path buffers | UTF-8 | 004 research R1 |
| OS file APIs | UTF-16 (`W` calls, `\\?\` normalized) | 004 research R2 |
| **Win32 UI controls (this feature)** | UTF-16 — always cross via `W` messages/APIs | 004 R6/R7, completed here |

**Key mechanism** (root cause of the bug): standard Win32 controls
(Edit, ComboBox, ListBox, Static, …) are *Unicode windows* even inside
an ANSI-created dialog. Any narrow text sent to them through an ANSI
message (`SendMessageA`/`WM_SETTEXT`, `CB_ADDSTRING`, `SetWindowTextA`,
…) is converted by USER32 from bytes → UTF-16 using CP_ACP. A UTF-8
payload therefore renders as mojibake (`č` = `C4 8D` → `ÄŤ` on CP1250).
The reverse direction (`WM_GETTEXT` A) converts UTF-16 → CP_ACP bytes:
characters outside the ACP degrade to `?`, and in-ACP accented
characters come back as single ANSI bytes that are *invalid UTF-8* —
silent name corruption on input.

### Encoding states of a narrow text buffer

| State | Meaning | Validity |
|-------|---------|----------|
| `U8` | Valid UTF-8 (the 004 contract) | Normal |
| `ACP` | ANSI bytes produced by a legacy A read-back | **Contract violation** — must not occur after this feature |
| `U8-torn` | UTF-8 truncated mid-sequence (byte-oriented truncation/measurement) | **Contract violation** — `SalU8ToW*` rejects it, triggering mojibake fallbacks |

The fix converts every name-bearing UI crossing to `U8 ⇄ UTF-16`
(`SalU8ToW*` / `SalWToU8*`) and makes truncation/measurement
sequence-safe, so only state `U8` survives.

## Entities

### 1. Name-bearing text surface (audit unit)

One UI location where a file/directory name or path crosses the
program↔control boundary as text.

| Field | Values |
|-------|--------|
| Surface | dialog/window + control (e.g. "Rename dialog — path combo") |
| Sites | source references `file:line` (display + read-back directions) |
| Direction | `display` (program → control), `input` (control → program), `both` |
| Mechanism | transfer helper (`CTransferInfo`), direct message, GDI draw, menu/list item, message box |
| Verdict | `DEFECTIVE` / `OK` (already W-safe) / `N/A` (cannot carry names) |
| Resolution | fix reference (task ID / commit) + verification ✓ with the sample-name set |

The full populated inventory is in [research.md](research.md) §Audit
inventory; its closure criterion is SC-002 (100% of `DEFECTIVE`
surfaces fixed and verified).

### 2. Name-validation site (secondary defect class)

Byte-level "invalid character" loops that predate UTF-8 and use
`signed char` comparisons (`*s >= 32`), rejecting every UTF-8 byte
≥ 0x80.

| Field | Values |
|-------|--------|
| Site | `file:line` + function |
| Effect | valid Unicode name rejected ("invalid name" error) although FR-004 requires acceptance |
| Fix rule | iterate bytes as `unsigned char`; control-char test `< 32` applies only to ASCII range; UTF-8 trail/lead bytes (≥ 0x80) are always acceptable name bytes |

### 3. History list (shared state touched by the fix)

`char* historyArr[]` rings (quick-rename, copy/move targets, change-dir,
masks, filters…) persisted to the registry.

- Contract: entries are UTF-8 (004 R9 made registry I/O wide; entries
  written from dialogs must be valid UTF-8).
- Corrupted entries recorded by the *buggy* build (ACP bytes read back
  via `WM_GETTEXT` A) may already sit in user registries. Load path
  policy: entries that are not valid UTF-8 are dropped at load (same
  protective rule 004 used for legacy config strings) — no migration
  attempt, matching 004's decision that pre-existing values written by
  ANSI builds were re-encoded on first save.

## Invariants after this feature

1. Every program→control crossing of a name uses UTF-16 (`W` message)
   with text converted by `SalU8ToW*`; every control→program crossing
   uses `W` read + `SalWToU8*`.
2. No code path truncates or measures a UTF-8 name buffer with
   byte/ANSI semantics on a UI surface (truncation happens on the
   UTF-16 form or at UTF-8 sequence boundaries).
3. No name-validation loop rejects bytes ≥ 0x80.
4. A name displayed in any dialog, confirmed unchanged, produces a
   byte-identical name on disk (no-op rename guarantee, FR-003).
