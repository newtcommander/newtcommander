# Phase 0 Research: Fix File Name Encoding in Find Results and Name Notices

**Feature**: `042-fix-find-results-encoding`
**Date**: 2026-07-26
**Input**: [spec.md](./spec.md)

All findings below are measured against the working tree at commit `01b1556`, not
inferred. Line numbers are from that revision.

---

## R1 — Why the Find results list uses the legacy notification route

**Decision**: Ask the list view to re-query its notification format after the
dialog exists, by sending `WM_NOTIFYFORMAT` / `NF_REQUERY` to the control from
`WM_INITDIALOG`.

**Root cause (conclusive, not inferred).** `CDialog::CDialogProc`
(`src/common/winlib.cpp:704`) attaches the C++ object to the window **only when
`WM_INITDIALOG` arrives**:

```
case WM_INITDIALOG:   dlg = (CDialog*)lParam;  ... WindowsManager.AddWindow(...)
default:              dlg = (CDialog*)WindowsManager.GetWindowPtr(hwndDlg);
...
if (dlg != NULL) dlgRes = dlg->DialogProc(...);
else             dlgRes = FALSE;   // winlib.cpp:769
```

Any message arriving **before** `WM_INITDIALOG` finds no registered object,
returns `FALSE`, and never reaches the derived `DialogProc`.

A list view created from a dialog template sends `WM_NOTIFYFORMAT(NF_QUERY)` to
its parent **during its own creation** — which happens while the dialog is being
built, i.e. before `WM_INITDIALOG`. Therefore:

- `CFindDialog`'s `WM_NOTIFYFORMAT` handler (`src/finddlg1.cpp:3864`), the one
  that returns `NFR_UNICODE`, **has never executed once since it was written**.
- Returning `FALSE` means "not handled", so `DefDlgProc` answers, and its rule is
  `IsWindowUnicode(parent)`. The dialog is created with the ANSI
  `DialogBoxParam` / `CreateDialogParam` (`winlib.cpp:616`, `:630`) because
  `CDialog::UnicodeWnd` is never set to `TRUE` anywhere in the application.
- The control therefore settles permanently on **ANSI** notifications, and
  `LVN_GETDISPINFOW` (`finddlg1.cpp:3878`, added by feature 004) is dead code.
  The live handler is the ANSI `LVN_GETDISPINFO` at `finddlg1.cpp:4162`, whose
  `WideCharToMultiByte(CP_ACP, …)` at line 4181 produces the reported `?`.

This closes the loop on the spec's Problem Statement: the application "already
prefers the modern interface and already supplies the name through it" — the
preference is simply expressed too late to be heard.

**Why `NF_REQUERY` works.** `NF_REQUERY` is the documented way to make a control
re-ask its parent after creation. At `WM_INITDIALOG` the object is attached, so
the handler runs and returns `NFR_UNICODE`. `NF_REQUERY` currently appears
**nowhere** in the codebase — this mechanism has never been used here.

**Scope of the change.** `WM_NOTIFYFORMAT` governs only the direction
*control → parent* (notifications). Messages sent *to* the control
(`ListView_InsertColumn` with `LoadStr` headers at `finddlg1.cpp:1194`,
`ListView_SetItemCount`, etc.) are unaffected and keep their present ANSI
behaviour. This is what makes the fix small and low-risk.

**Alternatives considered**

| Alternative | Rejected because |
|---|---|
| Set `CDialog::UnicodeWnd = TRUE` for `CFindDialog` | Converts the whole dialog to a Unicode window, changing the semantics of every `SetWindowText`/`GetWindowText`/`GetDlgItemText` on it. Feature 041 already broke this exact dialog once with a change of comparable reach and had to revert it. Disproportionate risk for one column. |
| Keep the ANSI route, widen the conversion (e.g. UTF-8 best-fit) | Forbidden by FR-002, and would be the third consecutive "widen the surviving character set" fix on this same column. |
| Move the `WM_NOTIFYFORMAT` handling into `CDialog::CDialogProc` so early messages are served | Changes dialog dispatch for all ~100 dialogs in the application to fix one list. Violates Principle III (incremental modernization). Recorded as a candidate for the future application-wide feature. |

---

## R2 — The same dead-handler pattern elsewhere

`packac.cpp:178` (the Pack/archiver-selection list) has **both** an ANSI
`LVN_GETDISPINFO` and a `LVN_GETDISPINFOW` handler, and the ANSI one carries the
comment *"legacy ANSI route, active only if the Unicode format was refused"*.

That comment is **wrong in practice**: `packac.cpp` has no `WM_NOTIFYFORMAT`
handler at all, so the Unicode format is *always* refused and the ANSI route is
always the live one. Its `LVN_GETDISPINFOW` is dead code for the same reason as
Find's.

`src/tserver/tablist.cpp:748` has the same shape but belongs to a separate helper
utility, not the main application binary — recorded, out of scope.

**Consequence for the plan**: `NF_REQUERY` is not a one-off patch for Find; it is
the repair for a small, closed class. Main-application members: `finddlg1.cpp`,
`packac.cpp`.

---

## R3 — Report 2: mechanism and the shape of its fix

**Decision**: Convert the *composition* to UTF-8 by using `LoadStrU8` at the
composing call site. Do **not** touch `SalMessageBox`.

**Mechanism, measured.**

- `LoadStr` (`src/salamdr2.cpp:~40`) is `LoadStringA` → bytes in the system ANSI
  codepage.
- `LoadStrU8` (`src/salamdr2.cpp:89`, added by feature 041) is `LoadStringW` +
  `SalWToU8` → UTF-8.
- `fileswnb.cpp:815` composes `LoadStr(IDS_EQUIVNAMESPAIR)` (ANSI) with
  `GetEquivalentPairNoticeName()` (UTF-8) and passes the result to
  `SalMessageBox`.
- `CMessageBox` has a wide drawing path (`src/msgbox.cpp:672`, `:687`, `:714`)
  and prefers it, but only accepts it when the **whole** body converts from
  UTF-8. A Czech ANSI template is not valid UTF-8, so the whole notice drops to
  the ANSI fallback (`:674`, `:697`, `:716`) — template correct, name mojibake.

**Why this went unnoticed until now.** English resource strings are pure ASCII.
ASCII is valid UTF-8, so in an English build the composed string converts
cleanly, the wide path is taken, and the name renders **correctly**. The defect
exists only in the 8 localized languages. This is a direct, measured
justification for the Q5 clarification: composed surfaces must be verified in all
9 shipped languages, because English is exactly the configuration that hides
them.

**Why the fix is per-call-site and not central.** Changing `SalMessageBox` to
interpret its text as UTF-8 would fix all callers at once — and would change
behaviour for plugins, which reach the same implementation through
`CSalamanderGeneralAbstract::SalMessageBox` (`src/plugins.h:1898`). FR-014 /
FR-014a forbid that. With `LoadStrU8` at the call site instead, plugin callers
still pass ANSI text, which still fails the UTF-8 test, still takes the ANSI
fallback, and still renders exactly as today — **bit-identical plugin
behaviour, no shared entry point modified.**

---

## R4 — Ingredient audit: what else can poison a composed message

A composed message is correct only if *every* ingredient is UTF-8. Current state
of each ingredient that reaches these compositions:

| Ingredient | Encoding today | Status |
|---|---|---|
| File/directory names | UTF-8 | Correct (feature 004) |
| `GetErrorText()` | **UTF-8** — `FormatMessageW` + conversion, `src/salamdr2.cpp:187-210` | Correct since feature 010 |
| Locale text (separators, date, time) | UTF-8 via `SalGetLocaleInfoU8` etc. | Correct since feature 041 |
| `LoadStr()` | **ANSI** | **The remaining hole** |

**Finding worth recording**: 85 call sites already compose `LoadStr` (ANSI) with
`GetErrorText` (UTF-8). Those messages are *already* mixed-encoding today,
independent of any file name. `LoadStr` is the last ANSI ingredient in the chain,
which is why converting it at the composing sites is sufficient — and why no
further ingredient audit is needed beyond this table.

---

## R5 — Inventory sizing (FR-009), and a measured blind spot

Mechanical pass over `src/*.cpp`, excluding `plugins/`, `saltests/`, `tserver/`,
`shellext/`, `setup/`, `salmon/`. Criteria: a printf-family call whose **format
argument** is `LoadStr(...)`, at least one substituted argument carrying a name,
and the result reaching a message box within the following statements.

| Pass | Sites | Files |
|---|---|---|
| Strict name pattern | **75** | 23 |
| Broadened name pattern | **119** | 30 |

Every hit routes to a message box; no `SetWindowText`/`DrawText` site survived
the criteria, because those paths were already converted by features 005/041.

**The blind spot, measured.** The strict pass **does not contain
`fileswnb.cpp:815` — the actually reported defect.** The name there arrives from
`GetEquivalentPairNoticeName()`, an accessor whose identifier the strict pattern
does not recognise as a name. Only the broadened pattern catches it.

This is empirical confirmation of the Q2 decision (FR-009a): a mechanical pass
alone would have shipped this feature **without fixing the bug that prompted
it**. The UI walk is not belt-and-braces; it is load-bearing. Per FR-009a the
pattern that missed it is recorded here as part of the method's known limits.

**Working estimate for planning**: ~119 candidate sites, of which the subset that
can actually display a non-ASCII name is the true work list. Classification is
the first implementation task, before any repair (FR-009).

**Category A (Report 1 class — list views on the legacy notification route)**:
2 sites in the main application — `finddlg1.cpp`, `packac.cpp`.

---

## R6 — Guard design (FR-016 / FR-017)

**Decision**: two independent mechanisms, both wired into the ordinary build.

**(a) Unit tests** in the existing `src/saltests/saltests.cpp` (single-file
project, already carrying `TestConversions`, `TestNormalization`,
`TestMatching`, and 7 more suites). New suite asserts:

- a composed string of localized template + non-ASCII name round-trips as UTF-8;
- `LoadStrU8` output is valid UTF-8 for a resource string containing non-ASCII;
- lenient display conversion yields exactly one `U+FFFD` per malformed unit and
  leaves neighbouring characters intact;
- surrogate pairs are never split by truncation.

Note honestly: these test the *helpers*, and helper bugs are not what caused
either report. They are the regression floor, not the guard.

**(b) Mechanical source check** — the part that actually addresses recurrence.
A script invoked by `build.cmd` that fails the build on:

1. `WideCharToMultiByte(CP_ACP, …)` reachable from a name-carrying display path;
2. a printf-family call with a `LoadStr(` format **and** a name argument **and** a
   message-box display within the statement window (the R5 pattern, broadened);
3. an `LVN_GETDISPINFOW` handler in a dialog that never sends `NF_REQUERY`
   (the R1/R2 pattern — a handler that cannot run).

Rule 3 is the one that would have caught Report 1 at the moment feature 004
introduced the dead handler.

**FR-017 demonstration**: revert each reported fix in turn and record the guard
failing. A guard never observed failing is not evidence.

---

## R7 — Verification matrix (Q5, FR-009c)

9 shipped UI languages: English + czech, german, french, dutch, hungarian,
romanian, slovak, spanish (`translations/languages.cfg`; russian,
chinesesimplified, ukrainian are `off`).

| Surface class | Languages | Rationale |
|---|---|---|
| Composes localized text **with** a name | all 9 | English is the configuration that hides the defect (R3) |
| Bare name, no localized text | 1 | No localized text present for the name to interact with |

The Find results Name column is a bare-name surface. The duplicate-name notice
and all ~119 composed sites are composed surfaces.

---

## R8 — Performance

- `NF_REQUERY`: one message at dialog initialisation. Not measurable.
- `LVN_GETDISPINFOW`: the list is virtual, so conversion runs only for visible
  rows — the same per-row work the ANSI handler does today, minus the second
  conversion down to the codepage. Expected to be marginally cheaper.
- `LoadStrU8` vs `LoadStr`: `LoadStringW` + one conversion, on message-box
  construction only. Not on any hot path.

SC-009 (10,000-row scroll) is therefore expected to pass unchanged; it is
verified, not assumed.

---

## Open items carried into Phase 1

None blocking. One judgement is deliberately deferred to implementation: the
exact subset of the 119 candidate sites that can display a non-ASCII name.
FR-009 requires that classification to be produced and written down **before**
the repairs, so the work list is known rather than discovered.
