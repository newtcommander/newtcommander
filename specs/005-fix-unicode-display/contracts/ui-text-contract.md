# Contract: UI Text Boundary (UTF-8 ⇄ Win32 controls)

**Feature**: [../spec.md](../spec.md) | **Plan**: [../plan.md](../plan.md)
**Extends**: feature 004 `contracts/plugin-interface-vnext.md` §3
("display/measure inside plugin UI must convert to UTF-16").

## Rule

Narrow (`char*`) file/directory names, paths, masks, and history
entries carry **UTF-8** (feature 004 R1). Any crossing between such a
buffer and a window/control/menu/GDI text API MUST convert at the
boundary and use the `W` form:

| Direction | Required form |
|-----------|---------------|
| program → control (`WM_SETTEXT`, `CB_ADDSTRING`, `LB_*STRING`, `LVM_*`, `TVM_*`, `SetWindowText`, `SetDlgItemText`, menu `dwTypeData`) | convert `SalU8ToW*` / `SplU8ToW*`, send the `W` message / call the `W` API |
| control → program (`WM_GETTEXT`, `GetWindowText`, `GetDlgItemText`, `CB_GETLBTEXT`, in-place-edit `pszText`) | read wide (`GetWindowTextW`, `…W` message), convert `SalWToU8` / `SplWToU8`; UTF-8 output buffer sized ≥ 3×WCHAR count + 1 |
| GDI measure/draw of names (`GetTextExtentExPoint`, `DrawText`, `ExtTextOut`, `TextOut`) | convert once, measure/draw wide; truncation only at WCHAR boundaries (no surrogate split) |

Invalid-UTF-8 input falls back to the legacy ANSI call (transitional
rule, same as `CTransferInfo::EditLine`, common/winlib.cpp:1042-1055).

Byte-level name-validation loops MUST treat bytes as `unsigned char`;
bytes ≥ 0x80 are always legal name bytes.

## Provided helpers

| Layer | Helpers (new in 005 unless noted) |
|-------|-----------------------------------|
| core `src/common/` | `SalU8ToW*`/`SalWToU8*` (004); `SalSetWindowTextU8`, `SalGetWindowTextU8`, `SalSetDlgItemTextU8`, `SalGetDlgItemTextU8`, `SalComboAddStringU8` |
| core shared UI | `CTransferInfo::EditLine` (004, fixed), `LoadComboFromStdHistoryValues` (005: converts internally), `CTruncatedString` (005: wide-safe), `CMessageBox`, `CStaticText`, `CStatusWindow`, menu/toolbar/tooltip draw (005: wide) |
| plugin SDK `src/plugins/shared/` | `splunicode.h` `SplU8ToW*`/`SplWToU8*` (004); `winliblt CTransferInfo::EditLine` (005: converts internally), `lukas HistoryComboBox` (005: converts internally), shared `SetDlgItemTextU8`/`GetDlgItemTextU8` (005: centralizes the zip/splitcbn pattern) |

## Compatibility

- **No ABI change.** `winliblt`/`lukas` are source-shared and compiled
  into each plugin; bundled plugins pick the fix up on rebuild within
  this solution. Third-party binaries keep their own compiled copies
  and are unaffected; interface 104 semantics are unchanged (narrow
  dialog text was already documented as UTF-8 — this feature makes the
  shipped helpers honor it).
- `CSalamanderGeneral::LoadComboFromStdHistoryValues` (spl_gen.h)
  keeps its signature; behavior change is display-correctness only.
- SDK doc update: spl_gen.h comments on the history/dialog helpers
  state the UTF-8 contract explicitly.
- History entries persisted by *buggy* builds may hold invalid UTF-8;
  the registry load path drops such entries (protective rule, 004
  precedent).
