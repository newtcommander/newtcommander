# Contract: Hot Paths Registry Format

**Feature**: 047-hot-path-names-icons
**Key**: `HKCU\Software\Tandem Commander\0.1\Hot Paths`

## Schema

One subkey per assigned slot, named by the **1-based** slot number `"1".."30"`.
Legacy read quirk (preserved): a subkey named `"0"` is read as slot 10.

Per-subkey values:

| Value | Type | Required | Content |
|-------|------|----------|---------|
| `Name` | REG_SZ | yes (for assigned slots) | **Effective label** — the custom display name if the user set one, otherwise the path text. Never empty for an assigned slot. |
| `Path` | REG_SZ | yes | Escaped path (`$` doubled), may contain `$(...)` variables. |
| `Visible` | REG_DWORD | no (default 1) | 1 = shown in the Change Drive menu. |
| `Icon` | REG_DWORD | no (default 0) | Gallery index 0–9. **New in feature 047.** Written only when ≠ 0. |

Unassigned slots have **no subkey** (the writer deletes them).

## Write rules (new build)

1. Slot with empty `Path` → delete the subkey (regardless of name/icon).
2. `Name` value := in-memory custom name if non-empty, else the user-visible
   path text (stored `Path` with `$$` unescaped to `$`). This keeps the file
   format identical to what pre-047 builds produce and expect.
3. `Icon` value written only when the gallery index is non-zero; when the index
   is 0 an existing `Icon` value is deleted.

## Read rules (new build)

1. Missing `Name` or `Path` → slot stays empty (as today).
2. If `Name` is byte-identical to the user-visible path text (i.e. the value a
   pre-047 auto-assign or rule 2 above would have written) → in-memory custom
   name is **empty** (unnamed; label follows the path). Otherwise the value is
   the custom name.
3. `Icon` missing, or ≥ gallery size, or negative → 0.
4. `Load1_52` legacy import keeps its existing name=path defaulting, which the
   rule above then classifies as unnamed.

## Compatibility matrix

| Writer → Reader | Result |
|-----------------|--------|
| pre-047 → 047 | All slots load; auto-filled labels (`Name==Path`) become unnamed entries displaying the path — visually identical (FR-010, SC-004). User-renamed entries keep their custom name. All icons default. |
| 047 → pre-047 | Old build sees a fully valid old-format file: every assigned slot has a non-empty `Name` (the effective label) and displays exactly what the new build displayed. `Icon` is ignored. No data loss. |
| 047 → 047 | Round-trip stable: unnamed slots save `Name=Path` and load unnamed; named slots round-trip verbatim; icons round-trip via `Icon`. |

## Explicitly out of contract

- No values are read from or written to Open Salamander, Altap or Newt
  Commander registry roots (constitution II).
- The global values `Hot Paths Bar`, `Auto Configurate Hot Paths`,
  `Hot Paths Index/Break/Width` are untouched by this feature.
