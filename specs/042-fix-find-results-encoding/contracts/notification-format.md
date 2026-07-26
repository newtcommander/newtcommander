# Contract: List views that render file names

**Feature**: `042-fix-find-results-encoding`
**Applies to**: every list view in the main application whose item text is, or
can contain, a file or directory name.

---

## §1 The control must be asked for wide notifications, and asked at a time the answer can be heard

A list view negotiates its notification format once, at creation, by sending
`WM_NOTIFYFORMAT` / `NF_QUERY` to its parent. In this application that query
**always arrives before the parent dialog can answer**: `CDialog::CDialogProc`
attaches the C++ object to the window only on `WM_INITDIALOG`, and returns
`FALSE` for every message that precedes it (`src/common/winlib.cpp:704-771`).

Consequently a `WM_NOTIFYFORMAT` handler written in a derived `DialogProc` is
**unreachable**, and the control falls back to `IsWindowUnicode(parent)`, which
is `FALSE` for every dialog here.

**Required**: a dialog owning such a list view MUST send

```
SendMessage(hListView, WM_NOTIFYFORMAT, (WPARAM)HWindow, NF_REQUERY);
```

from `WM_INITDIALOG` (or later), so the control re-asks when the handler can
respond.

**Forbidden**: relying on a `WM_NOTIFYFORMAT`/`NF_QUERY` handler alone. It is
dead code. A handler that has never executed is not a mechanism.

---

## §2 Scope of the change

`WM_NOTIFYFORMAT` governs **only** the direction control → parent
(`LVN_GETDISPINFO*`, `LVN_ODFINDITEM*`, and other notifications).

Messages sent **to** the control — `ListView_InsertColumn`,
`ListView_SetItemCount`, `ListView_SetItemState` — are unaffected and keep their
existing ANSI behaviour. Column headers loaded with `LoadStr` therefore continue
to work unchanged.

This asymmetry is what makes the repair safe: no rendering, styling or layout
behaviour of the control changes.

---

## §3 Text handed to the control

Once wide notifications are negotiated:

- `LVN_GETDISPINFOW` MUST supply the name converted from UTF-8 with
  `SalU8ToW` (strict) or `SalU8ToWDisplay` (lenient, `U+FFFD` per malformed
  character — required where malformed stored names are possible).
- The ANSI `LVN_GETDISPINFO` handler MUST NOT convert a name through
  `WideCharToMultiByte(CP_ACP, …)`. That conversion is lossy by construction and
  is forbidden by FR-002. An ANSI handler may be retained only as an
  ASCII-only fallback, or removed.

**Forbidden**, and checked by the FR-016 guard:

```c
// FORBIDDEN on a name-carrying path — silently destroys any character
// outside the machine's codepage, one '?' per UTF-16 code unit
WideCharToMultiByte(CP_ACP, 0, wide, -1, ansi, size, NULL, NULL);
```

---

## §4 Behaviours that must follow the real names, not the displayed text

| Behaviour | Requirement |
|---|---|
| Type-to-search (`LVN_ODFINDITEM*`) | Compare against stored names, in one encoding on both sides (FR-006) |
| Sorting | Order by real names (FR-007) |
| Clipboard, open, focus-in-panel | Carry the real name, never the rendered text (FR-008) |

A control in wide notification mode delivers `LVN_ODFINDITEMW`; comparing its
UTF-16 text against UTF-8 stored names requires converting one side. Comparing
ANSI text against UTF-8 names — the present state — silently fails for every
non-ASCII name.

---

## §5 Known members of this class

| Site | State before this feature |
|---|---|
| `src/finddlg1.cpp` (Find results) | Has both handlers **and** an unreachable `WM_NOTIFYFORMAT`; ANSI route live; lossy conversion at `:4181` — **Report 1** |
| `src/packac.cpp:178` (Pack, archiver list) | Has both handlers, **no** `WM_NOTIFYFORMAT` at all; ANSI route always live; its comment "active only if the Unicode format was refused" is therefore inaccurate |
| `src/tserver/tablist.cpp:748` | Same shape; separate helper utility, not the main binary — **out of scope**, recorded |

---

## §6 Verification

- The reported Find search displays all four names exactly (SC-001).
- The Path column, sorting, long-path results and the searched-directory progress
  text are unchanged (FR-012).
- Type-to-search reaches non-ASCII fixture names (SC-003).
- A 10,000-row result set scrolls as before (SC-009).
- Language class: **bare name** — verified once, not per language (FR-009c).
