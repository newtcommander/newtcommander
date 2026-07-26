# Contract: Messages that combine localized text with a file name

**Feature**: `042-fix-find-results-encoding`
**Applies to**: every message, notice, caption or status text in the main
application that substitutes a file or directory name into a localized template.

---

## §1 One message, one encoding

A composed message MUST be built from ingredients that all share one encoding.

**Current encoding of every ingredient** (measured, see research.md R4):

| Ingredient | Producer | Encoding |
|---|---|---|
| File / directory name | file enumeration | UTF-8 |
| System error text | `GetErrorText()` | UTF-8 *(since feature 010)* |
| Locale text | `SalGetLocaleInfoU8` / `SalGetDateFormatU8` / `SalGetTimeFormatU8` | UTF-8 *(since feature 041)* |
| Localized template | `LoadStrU8()` | UTF-8 |
| Localized template | `LoadStr()` | **ANSI** |

`LoadStr` is the last ANSI ingredient in the chain. A composition that includes a
name MUST therefore use `LoadStrU8`.

```c
// CORRECT — every ingredient is UTF-8, the message keeps its wide path
_snprintf_s(buf, _TRUNCATE, LoadStrU8(IDS_EQUIVNAMESPAIR), GetEquivalentPairNoticeName());

// DEFECTIVE — ANSI template + UTF-8 name; the whole message loses its wide path
_snprintf_s(buf, _TRUNCATE, LoadStr(IDS_EQUIVNAMESPAIR), GetEquivalentPairNoticeName());
```

---

## §2 Why a mixed message costs the whole message

`CMessageBox` prefers a wide drawing path (`src/msgbox.cpp:672`, `:687`, `:714`)
and accepts it only when the **entire** body converts from UTF-8. The decision is
all-or-nothing:

```
composed string
      ├── valid UTF-8 ──────► DrawTextW    ► template correct, name correct
      └── not valid UTF-8 ──► DrawText     ► template correct, NAME MOJIBAKE
```

One ANSI byte in the template is enough to send a perfectly good name down the
legacy branch. The name is never the faulty ingredient; it is the casualty.

---

## §3 English is the configuration that hides this

English resource strings are pure ASCII. ASCII is valid UTF-8, so an English
build takes the upper branch and renders correctly **even when the composition is
mixed**. The defect is visible only in the 8 localized languages.

**Therefore**: any surface covered by this contract MUST be verified in all 9
shipped languages (FR-009c). Verifying in English proves nothing about it.

---

## §4 Shared entry points are not modified

`SalMessageBox` is exported to plugins via
`CSalamanderGeneralAbstract::SalMessageBox` (`src/plugins.h:1898`). Plugins reach
the same implementation the main application does.

**Forbidden**: changing how `SalMessageBox`, `CMessageBox` or `LoadStr`
interpret their text. That would alter plugin output with no plugin recompiled
and no plugin source touched — prohibited by FR-014 and FR-014a.

**Required**: the main application opts in **per call site**, by choosing
`LoadStrU8` instead of `LoadStr`. A caller that does not opt in behaves exactly
as today.

This is why plugin behaviour stays bit-identical: a plugin passes ANSI text,
which still fails the UTF-8 test, still takes the ANSI fallback, and still renders
exactly as it does now.

---

## §5 Malformed input

Where a stored name may be malformed, the display conversion MUST be lenient
(`SalU8ToWDisplay`): exactly one `U+FFFD` per offending character, every other
character of the name and every other part of the message intact (FR-005).

`U+FFFD` MUST NOT be written back into a name, a path, or anything persisted.

**Distinguish the two symptoms** — they look similar and mean opposite things:

| Symptom | Meaning |
|---|---|
| `?` | A lossy route destroyed a well-formed name — **always a defect** (FR-002) |
| `�` | Stored data was itself malformed — **correct handling** (FR-005) |

---

## §6 Scale

Mechanical pass over the main application (research.md R5):

| Pass | Sites | Files |
|---|---|---|
| Strict name pattern | 75 | 23 |
| Broadened name pattern | 119 | 30 |

All route to message boxes.

**The strict pass does not contain the reported defect** (`fileswnb.cpp:815`),
because the name arrives from an accessor whose identifier the pattern did not
recognise. A code-only inventory would have shipped this feature without fixing
the bug that prompted it — which is why FR-009a makes the user-interface walk
load-bearing rather than confirmatory.

---

## §7 Verification

- The reported notice reads `č-dir`, in all 9 languages, with the surrounding
  sentences still correct (SC-001, SC-004).
- Every site the inventory identifies is demonstrated correct in the running
  application (SC-007).
- A plugin's message boxes are observed before and after and are identical
  (SC-008a).
- The FR-016 guard fails when the composition is reverted to `LoadStr` (SC-012).
