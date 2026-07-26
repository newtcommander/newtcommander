# Validation Results (FR-015)

**Feature**: `042-fix-find-results-encoding`
**Date**: 2026-07-26
**Build**: Debug x64 and Release x64, both `Počet chyb: 0`
**Tests**: `saltests` — **1118 checks, 0 failed**
**Guard**: `python tools/check_encoding.py --strict` → exit 0

Everything below was exercised in the running application unless the row says
otherwise. What was *not* verified is stated explicitly at the end, following
the precedent set by feature 041.

---

## The two reported defects

### Report 1 — Find results list

Search `🙂-d` in `C:\Users\pavel\AppData\Local\Temp\salamander-test\010`.

| Before | After |
|---|---|
| `emoji-??-dir` | `emoji-🙂-dir` |
| `emoji-??-dir - Copy` | `emoji-🙂-dir - Copy` |
| `emoji-??-dir - Copyě ??????` | `emoji-🙂-dir - Copyě 😍😍😍` |

Verified by reading the control's text with `LVM_GETITEMTEXTW` and comparing code
points, not by eye:

```
emoji-🙂-dir - Copyě 😍😍😍
U+0065 U+006D U+006F U+006A U+0069 U+002D U+D83D U+DE42 U+002D U+0064 U+0069
U+0072 U+0020 U+002D U+0020 U+0043 U+006F U+0070 U+0079 U+011B U+0020
U+D83D U+DE0D U+D83D U+DE0D U+D83D U+DE0D
```

The surrogate pairs are intact, `U+011B` (ě) is intact, nothing substituted.

| Check | Result |
|---|---|
| 3 reported names exact | PASS |
| item count still 4 | PASS |
| no `?` substitution | PASS |
| no `U+FFFD` | PASS |
| Path column rooted | PASS |
| all 12 top-level fixtures verbatim (83-item result set) | PASS |
| no `?` across all 83 names | PASS |
| no mojibake digraphs across all 83 names | PASS |

### Report 2 — duplicate-name notice

Entering `C:\Users\pavel\AppData\Local\Temp\salamander-test`. Before: `ÄŤ-dir`.
After: `č-dir`.

Verified by reading the control text with `GetWindowTextW` — decisive, because
`CMessageBox` uses `SetWindowTextW` only when the body is valid UTF-8 and
silently falls back to the ANSI call when it is not.

| Language | Name | Localized text | Verdict |
|---|---|---|---|
| english | ok | ok | PASS |
| czech | ok | ok | PASS |
| german | ok | ok (`enthält`, `Einträge`) | PASS |
| french | ok | ok (`entrées`) | PASS |
| dutch | ok | ok | PASS |
| hungarian | ok | ok (`bejegyzéseket`, `eltérő`) | PASS |
| romanian | ok | ok (`conține`, `căror`) | PASS |
| slovak | ok | ok (`Táto`, `ktorých`) | PASS |
| spanish | ok | ok (`idénticos`) | PASS |

**9/9.** Run twice — before and after the 84-site bulk conversion.

---

## Third symptom, not reported by the user

Type-to-search in the results list compared ANSI keystrokes against UTF-8 stored
names and silently matched nothing for any non-ASCII name.

Driven through `LVM_FINDITEMW`, which the owner-data list dispatches to the
parent as `LVN_ODFINDITEMW` — i.e. the handler this feature changed.

| Prefix | Selected | Verdict |
|---|---|---|
| `Тес` (Cyrillic) | `Тест-Ελλάδα-测试 +ěš` | PASS |
| `č-dir` (Czech) | `č-dir` | PASS |
| `emoji-🙂` (emoji) | `emoji-🙂-dir` | PASS |
| `emoji-` (ASCII, no regression) | `emoji-🙂-dir` | PASS |
| no-match | returns −1, selection unmoved, no error | PASS |

---

## Regression checks (FR-012)

| Check | Result |
|---|---|
| Find Path column, 83 cells rooted or ellipsised | PASS |
| Long-path results — 36 long-path cells rendered | PASS |
| Sorting by Name orders by real names (`č-dir` after `copy-target-dir`, correct locale collation) | PASS |
| Sorted list still free of `?` and `U+FFFD` | PASS |
| Find dialog in all 9 languages | PASS |
| ASCII type-to-search unchanged | PASS |

### Performance (SC-009)

Searched `C:\Windows\System32`: **26,225 items**, search + populate **3.3 s**,
item text retrieval **1 ms for 6 probes including the last row**. The 10,000-item
requirement is met with a wide margin, and retrieval does not degrade at the end
of the list.

---

## Feature 041 re-verification (FR-013)

The reported file focused in a panel, Czech UI. Information line reads:

```
Epizoda IV – Nová naděje (Despecialized) - původní kinodabing  CZ dabing.mkv:
1 948 456 197, 24.01.2018, 21:56:25, A
```

En-dash, `á`, `ě`, `ů`, `í` all correct; the non-breaking-space thousands
separator still correct. Feature 041's fix is **unaffected**.

`src/stswnd.cpp` has an empty diff, so the information-line drawing path was not
touched.

---

## Plugin boundary (FR-011, FR-014, SC-008)

| Check | Result |
|---|---|
| files changed under `src/plugins/` | **0** |
| `src/plugins.h` diff | **empty** |
| `src/spl_gen.h` diff | **empty** |
| `src/spl_vers.h` diff (ABI version) | **empty** |
| `src/msgbox.cpp` diff (`SalMessageBox` / `CMessageBox`) | **empty** |
| `LoadStr()` itself modified | **no** |

No shared entry point changed behaviour, so a plugin passing ANSI text still
takes the same path and renders exactly as before. `LoadStrU8` is opted into per
call site, as FR-014a requires.

---

## Guard demonstration (FR-017, SC-012)

| State | Guard exit | Detected |
|---|---|---|
| both fixes in place | **0** | — |
| Report 1 fix reverted | **1** | `finddlg1.cpp: [cp-acp-display]` |
| Report 2 fix reverted | **1** | `fileswnb.cpp: [mixed-composition]` |
| both restored | **0** | — |

The guard has been *observed* failing for each defect, not assumed to. It runs
from `build.cmd` on every build, so there is no step a contributor can skip
(SC-013).

---

## Codepage independence (SC-010)

Established structurally, as decided during clarification: no name passes
through a legacy-codepage conversion anywhere, and `check_encoding.py`'s
`cp-acp-display` rule reports **0** across every name-carrying display path. No
machine locale was changed.

---

## What was NOT verified, and why

Stated plainly rather than implied.

1. **The 84 bulk-converted composed-message sites were not each triggered
   individually at runtime.** Reaching all 84 would mean provoking 84 distinct
   error conditions. What *was* verified: the mechanism (unit tests), one
   representative site end-to-end in all 9 languages (`fileswnb.cpp`, Report 2 —
   the same code shape as the other 83), and the guard proving no site is left
   mixed. Two attempts to trigger further name-bearing dialogs
   (create-directory with an invalid name, and with an over-long name) produced
   messages that carry no file name, so they did not add coverage; both rendered
   their Czech text correctly.

2. **Plugin dialogs were not exercised at runtime.** They are unchanged by
   construction — `src/plugins/` has an empty diff and no shared entry point
   changed — so the check applied was the diff, not a runtime pass. This matches
   what feature 041 recorded as unverified for the same reason.

3. **A machine whose legacy codepage is not Central European was not tested.**
   Deliberate: SC-010 was made structural during clarification precisely so this
   would not require changing the machine's regional settings.

4. **`src/tserver/tablist.cpp`** carries the same `dead-dispinfow` defect but
   belongs to a separate helper executable. Recorded in `inventory.md`, not
   fixed.

5. **15 composed-message sites were deliberately not converted** — plugin
   metadata (13), a network share name, a configuration name. Each is annotated
   in place with its reason and listed in `inventory.md`. Converting them could
   leave the message mixed the other way and damage text that renders correctly
   today.

---

## Two defects found and fixed that no user reported

- **`packac.cpp`** had the identical dead-handler defect, and a comment
  asserting the opposite ("active only if the Unicode format was refused" — it
  was *always* refused, since the file had no `WM_NOTIFYFORMAT` handler at all).
  Found mechanically.
- **`fileswn3.cpp` and `salamdr5.cpp`** assigned a `text` variable from the ANSI
  `LoadStr()` and substituted it into a template being converted. Converting the
  template alone would have left the message mixed the *other* way and turned the
  localized sentences into mojibake — a regression introduced by the fix itself.
  Caught by a follow-up sweep for indirect ingredients, before the build.

---

## Note on the root cause, for the record

The `WM_NOTIFYFORMAT` handler in `finddlg1.cpp` was broken **twice over**, and
both halves had to be repaired before a single name rendered correctly:

1. It never ran — the control's creation-time `NF_QUERY` arrives before
   `WM_INITDIALOG`, and `CDialog::CDialogProc` attaches the dialog object only at
   `WM_INITDIALOG`.
2. It answered in a way a dialog cannot — `return NFR_UNICODE` is a
   *window*-procedure idiom. A dialog procedure returns only handled/not-handled;
   a real result must go through `DWLP_MSGRESULT`. Returning 2 merely said
   "handled" while the control read a result of zero and stayed on ANSI.

The first fix attempt corrected only (1). The names changed from `??` to
`đź™‚`-style mojibake — different symptom, same defect — which is what exposed (2).
