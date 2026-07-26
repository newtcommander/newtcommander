# Quickstart: Reproduce and Verify

**Feature**: `042-fix-find-results-encoding`
**Date**: 2026-07-26

---

## Build

```batch
set OPENSAL_BUILD_DIR=D:\Build\OpenSal\
build.cmd
```

Use `build.cmd full` when language modules must be regenerated — required for any
verification step that switches the UI language (most of §3 below).

Call `build.cmd` by its absolute path from the repository root.

---

## Fixtures

Already present at `C:\Users\pavel\AppData\Local\Temp\salamander-test`:

```
010\emoji-🙂-dir
010\emoji-🙂-dir - Copy
010\emoji-🙂-dir - Copyě 😍😍😍
010\Тест-Ελλάδα-测试 +ěš
010\Тест-Ελλάδα-测试 +ěš - Copy
010\😍
010\Můj disk qšěščrč1
010\long-paths\…            (long-path subtree)
č-dir                        ┐ canonically equivalent pair —
č-dir                        ┘ triggers the duplicate-name notice
```

The two `č-dir` entries look identical but are stored with different Unicode
spellings (precomposed vs decomposed). That pair is what makes §2 reproducible.

---

## §1 Reproduce Report 1 — Find results

1. Press **Ctrl+F**.
2. **Names**: `🙂-d`
3. **Look in**: `C:\Users\pavel\AppData\Local\Temp\salamander-test\010`
4. Tick **Include subdirectories**, press **Find**.

**Before the fix** — 4 items found, Name column reads:

```
emoji-??-dir
emoji-??-dir - Copy
emoji-??-dir - Copyě ??????
```

Note that the **Path** column is correct. Two columns, two routes.

**After the fix** — names read `emoji-🙂-dir`, `emoji-🙂-dir - Copy`,
`emoji-🙂-dir - Copyě 😍😍😍`, plus the fourth match.

**Reading the symptom**: `ě` survives and each emoji becomes exactly **two** `?`.
That two-to-one ratio is the fingerprint of a codepage conversion (one `?` per
UTF-16 code unit, and an emoji is a surrogate pair). If you ever see `?` in a
name again, count them — the ratio tells you which route produced it.

---

## §2 Reproduce Report 2 — duplicate-name notice

1. Set the UI language to **Czech** (Options ▸ Language). Any localized language
   reproduces it; **English will not** (see §3).
2. Enter `C:\Users\pavel\AppData\Local\Temp\salamander-test`.

**Before the fix** — the notice names the directory as `ÄŤ-dir`, while every
Czech sentence around it is correct.

**After the fix** — it reads `č-dir`, and the surrounding sentences are still
correct.

---

## §3 The English trap

**English builds do not reproduce Report 2 and never will.**

English resource strings are pure ASCII, ASCII is valid UTF-8, so the composed
message passes the UTF-8 test and takes the correct rendering path. The defect
requires a template containing non-ASCII — i.e. one of the 8 localized languages.

Verifying a composed message in English proves nothing about it. This is why
FR-009c pins composed surfaces to all 9 shipped languages: English, czech,
german, french, dutch, hungarian, romanian, slovak, spanish.

Bare-name surfaces (the Find Name column) contain no localized text and are
verified once.

---

## §4 Regression checks — must still pass

| Check | Expectation |
|---|---|
| Find **Path** column, incl. the long path | Correct, unchanged |
| Find column sorting by Name | Orders by real names |
| Find long-path results | Display as before |
| Searched-directory progress text | Correct |
| Find dialog in all 9 languages | Labels, buttons, status bar correct |
| Panel information line (feature 041) | The reported `.mkv` file still correct |
| Selection summary (feature 041), all 9 languages | Still correct |
| A plugin that shows file names in a message box | **Visually identical** before and after |
| 10,000-row result set | Scrolls with no perceptible delay |

The information line and selection summary are listed because feature 041
repaired them and this feature touches adjacent machinery — FR-013 requires them
re-verified, not assumed.

---

## §5 Type-to-search

With the results list focused, type the leading characters of
`Тест-Ελλάδα-测试 +ěš`.

- **Before**: nothing happens for any non-ASCII name.
- **After**: the selection moves to that item.

Also confirm ASCII type-to-search is unchanged, and that typing text matching
nothing moves the selection nowhere and raises no error.

---

## §6 Malformed-name behaviour

A name containing an unpaired surrogate must show exactly **one** `�` for the
offending character, with the rest of the name and every other column intact.

Do not confuse the two symptoms:

| Symptom | Meaning |
|---|---|
| `?` | A lossy route destroyed a good name — **a defect** |
| `�` | The stored data was malformed — **correct handling** |

---

## §7 Guard demonstration (FR-017)

The guard is only evidence once it has been seen failing:

1. Revert the Report 1 fix alone → build must **fail**.
2. Restore it; revert the Report 2 fix alone → build must **fail**.
3. Restore both → build passes, all tests pass.

Record all three outcomes in `validation-results.md`.

---

## §8 Driving the app for verification

When scripting the GUI, note that the process's `MainWindowHandle` is the
**splash screen**, not the main window — enumerate top-level windows and match on
class/title instead of trusting `MainWindowHandle`.
