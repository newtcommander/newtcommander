# Phase 1 Data Model: Text Encoding States

**Feature**: `042-fix-find-results-encoding`
**Date**: 2026-07-26

This feature stores nothing and changes no persisted format. The "data model"
that matters is the **encoding state of a string as it travels from the file
system to the screen**, because every defect in this class is a state error at a
boundary.

---

## Entities

### Stored name

A file or directory name as the file system holds it.

| Attribute | Value |
|---|---|
| Encoding in memory | UTF-8 (feature 004) |
| Source | `FindFirstFileW`/`FindNextFileW`, converted at the OS boundary |
| Mutability | **Never modified by this feature.** Read-only input to every display. |
| Validity | Normally well-formed UTF-8. May be malformed only for names containing unpaired UTF-16 surrogates (legal on NTFS, producible only deliberately). |

Invariant: **a stored name is never rewritten, normalized, or case-folded for
display purposes.** Normalization exists only for transient matching/collation
copies (`SalNormalizeNFC`).

---

### Text ingredient

Any string that can be concatenated into a message shown to the user. The whole
defect class reduces to ingredients disagreeing about encoding.

| Ingredient | Producer | Encoding | Since |
|---|---|---|---|
| Stored name | file enumeration | UTF-8 | feature 004 |
| System error text | `GetErrorText()` | UTF-8 | feature 010 |
| Locale text (separators, date, time) | `SalGetLocaleInfoU8` / `SalGetDateFormatU8` / `SalGetTimeFormatU8` | UTF-8 | feature 041 |
| Localized template | `LoadStrU8()` | UTF-8 | feature 041 |
| Localized template | `LoadStr()` | **ANSI (system codepage)** | legacy — the remaining hole |
| Plugin-supplied text | plugin code | ANSI | unchanged, by FR-014 |

**Rule (FR-004)**: every ingredient of one composed message must share one
encoding. A single ANSI ingredient makes the whole message invalid UTF-8, and the
consequence is not local — it costs the *entire* message its modern rendering
path.

---

### Composed message

Localized text with one or more ingredients substituted into it.

| Attribute | Value |
|---|---|
| Composition | printf-family call, format from `LoadStr`/`LoadStrU8` |
| Valid states | **All-UTF-8** (correct) · **All-ANSI** (correct, legacy — what plugins do) · **Mixed** (defective) |
| Display route | `SalMessageBox` → `CMessageBox` |
| Language class (FR-009c) | Composed → verify in all 9 languages · Bare name → verify once |

**State transition — what "mixed" costs:**

```
composed string
      │
      ├── valid UTF-8 ──────► wide path (DrawTextW)      ► name correct, template correct
      │
      └── not valid UTF-8 ──► ANSI fallback (DrawText)   ► template correct, NAME MOJIBAKE
```

The branch is all-or-nothing, which is why one bad ingredient destroys the name
even though the name itself was never the problem.

**Why English hides it**: an English template is pure ASCII; ASCII is valid
UTF-8; so an English build takes the upper branch and looks correct. Only the 8
localized languages take the lower branch.

---

### Display route

Where a name is turned into pixels. The route decides the symptom.

| Route | Mechanism | Failure mode when handed UTF-8 |
|---|---|---|
| Wide (`DrawTextW`, `SetWindowTextW`, `LVN_GETDISPINFOW`) | UTF-16 | None — correct |
| ANSI + codepage conversion | `WideCharToMultiByte(CP_ACP)` | **Lossy** — `?` per unrepresentable unit (Report 1) |
| ANSI, raw bytes | `DrawText` on UTF-8 bytes | **Uninterpreted** — `Ã`/`Ä` sequences (Report 2) |

Invariant (FR-002): a name must never traverse the lossy route. A name *may*
traverse the wide route only.

---

### Notification format (list views)

A per-control setting deciding whether a list view asks its parent for text in
ANSI or wide form.

| Attribute | Value |
|---|---|
| Negotiated by | `WM_NOTIFYFORMAT` |
| When | `NF_QUERY` at control creation; `NF_REQUERY` on demand |
| Default when parent does not answer | `IsWindowUnicode(parent)` — **FALSE** for every dialog in this application |
| Direction affected | control → parent (notifications) **only**; messages sent *to* the control are unaffected |

**State transition — the Report 1 defect:**

```
control created
      │  sends NF_QUERY to parent
      ▼
parent dialog object not yet attached (attaches only on WM_INITDIALOG)
      │  CDialogProc returns FALSE  =  "not handled"
      ▼
DefDlgProc answers from IsWindowUnicode(parent) = FALSE
      ▼
format := ANSI  ── permanent unless NF_REQUERY is sent
      │
      └──► LVN_GETDISPINFOW handler is unreachable; the ANSI handler serves every row
```

The repair adds one transition: send `NF_REQUERY` at `WM_INITDIALOG`, when the
parent can finally answer, moving the state to wide.

---

## Validation rules

| Rule | Source | Applies to |
|---|---|---|
| A name is never passed through a lossy conversion | FR-002 | every display route |
| All ingredients of one message share one encoding | FR-004 | every composed message |
| Malformed stored data yields exactly one `U+FFFD` per offending character, nothing else affected | FR-005 | display conversion only |
| A substitution character is never written back into a name or path | `salunicode.h` contract | all callers |
| Truncation never splits a surrogate pair | feature 041, Edge Cases | every truncating display |
| Shared entry points keep ANSI semantics for plugin callers | FR-014a | `SalMessageBox`, `LoadStr` |

**FR-002 vs FR-005 — not a contradiction.** FR-002 governs a *route* too narrow
to carry a well-formed name: never acceptable, and `?` is its signature. FR-005
governs *stored data* that is itself malformed: `U+FFFD` is the correct answer,
and it is the signature of correct handling. Same-looking symptom, opposite
meanings.
