# FR-009 Inventory: where a file name meets a display

**Feature**: `042-fix-find-results-encoding`
**Date**: 2026-07-26
**Instrument**: `tools/check_encoding.py` (also shipped as the FR-016 build guard)

Built along both axes required by FR-009a: a mechanical pass over the source
carries the completeness argument, and a walk of the running application
cross-checks it. Each entry records which axis found it, how its verdict was
reached, and its language class (FR-009b, FR-009c).

---

## Baseline, before any repair (T006)

| Rule | Findings |
|---|---|
| `cp-acp-display` — a name converted through the legacy codepage | 1 |
| `mixed-composition` — a name composed into a `LoadStr()` template | 100 |
| `dead-dispinfow` — an `LVN_GETDISPINFOW` handler that can never run | 2 |

## Final state

| Rule | Findings | Meaning |
|---|---|---|
| `cp-acp-display` | **0** | no name is routed through the legacy codepage anywhere |
| `mixed-composition` | **0 unannotated** (15 annotated with a reason) | every remaining site is a recorded decision |
| `dead-dispinfow` | **0** | both dialogs now send `NF_REQUERY` |

`python tools/check_encoding.py --strict` exits 0. The build fails if that
changes.

---

## Axis 1 — mechanical pass

### `dead-dispinfow` (2 found, 2 fixed)

| Site | Verdict | How reached |
|---|---|---|
| `src/finddlg1.cpp` (Find results) | **fixed** | Report 1. `NF_REQUERY` from `WM_INITDIALOG` + `DWLP_MSGRESULT` answer. Verified in the running app. |
| `src/packac.cpp` (Pack, archiver list) | **fixed** | Same defect, found mechanically, not reported by a user. Added both the `NF_REQUERY` send and the `WM_NOTIFYFORMAT` handler it lacked entirely. |
| `src/tserver/tablist.cpp:748` | **out of scope** | Same shape, but belongs to a separate helper executable, not the main binary. Recorded for a future feature. |

The comment at `packac.cpp` claiming the ANSI route was "active only if the
Unicode format was refused" was factually wrong: with no `WM_NOTIFYFORMAT`
handler at all, the Unicode format was *always* refused. Corrected in place.

### `cp-acp-display` (1 found, 1 fixed)

| Site | Verdict | How reached |
|---|---|---|
| `src/finddlg1.cpp:4181` | **fixed** | The lossy `WideCharToMultiByte(CP_ACP, …)` added by feature 041. Removed; the ANSI handler is now ASCII-only and unreachable in practice. |

Every other `CP_ACP` conversion in the tree was examined and is **not** a display
path — OLE/shell boundaries, clipboard interop, documented invalid-UTF-8
fallbacks, and the separate `translator`/`reglib`/`salmon` executables. The rule
is scoped to conversions whose result reaches a drawing sink, which is why they
do not appear.

### `mixed-composition` (100 found → 84 fixed, 15 recorded, 1 reclassified)

Classified by **ingredient**, because that is what decides whether the site is
this defect. A composed message is broken when a known-UTF-8 value is
substituted into a legacy-codepage `LoadStr()` template.

Ingredient audit (measured, research.md R4):

| Ingredient | Encoding | Since |
|---|---|---|
| file / directory names, paths | UTF-8 | feature 004 |
| `GetErrorText()` | UTF-8 | feature 010 |
| `NumberToStr()`, locale separators | UTF-8 | feature 041 |
| `LoadStr()` | **legacy codepage** | the remaining hole |

`LoadStr` was confirmed to be the *only* ANSI ingredient reaching these calls,
which is what makes the repair a one-symbol change per site.

**84 fixed** — `LoadStr` → `LoadStrU8` throughout the call, across 25 files:
`codetbl` `dialogs` `dialogs3` `dialogs4` `dialogs5` `dialogs6` `execute`
`fileswn0` `fileswn2` `fileswn3` `fileswn5` `fileswn6` `fileswn7` `fileswn8`
`fileswn9` `fileswna` `fileswnb` `mainwnd3` `mainwnd4` `mainwnd5` `salamdr2`
`salamdr3` `salamdr5` `salshlib` `shellsup` `viewer3`.

**15 recorded, deliberately not changed** — annotated in place with
`// encoding-check: allow mixed-composition - <reason>`:

| Sites | Substituted value | Reason |
|---|---|---|
| `dialogs5.cpp` ×3, `fileswn7.cpp` ×2, `plugins1.cpp` ×8 | plugin name / DLL name | Plugin-supplied metadata. Its encoding is not controlled by this feature; converting the template around it could leave the message mixed the *other* way and damage text that renders correctly today. **Revisit when the plugin metadata encoding is itself defined.** |
| `dialogs6.cpp:645` | network share name | Comes from the OS share enumeration, not the file system. **Revisit with the share-enumeration encoding.** |
| `mainwnd3.cpp:2799` | configuration name | Chosen in-app, not a file name. **Revisit if configuration names ever accept arbitrary text.** |

---

## Axis 2 — user-interface walk

Walked against the fixture set in a **Czech** build (English cannot reproduce the
composed-message defect at all — see below).

| Surface | Language class | Verdict | Evidence |
|---|---|---|---|
| Find results, Name column | bare name | **fixed** | 4/4 reported names exact; 83/83 fixture names verbatim; code points asserted |
| Find results, Path column | bare name | verified correct | 83/83 rooted, 36 long-path cells |
| Find results, sorting | bare name | verified correct | orders by real names (`č-dir` after `copy-target-dir` — locale collation) |
| Find results, type-to-search | bare name | **fixed** | Cyrillic / Czech / emoji / ASCII prefixes all select correctly |
| Duplicate-name notice | composed | **fixed** | 9/9 languages, name + localized text simultaneously correct |
| Panel information line | composed | verified correct (unchanged) | reported `.mkv` renders with all diacritics and the en-dash |
| Panel file list | bare name | verified correct | all fixture names render exactly |
| Create-directory error boxes | composed | verified correct | Czech system error text renders correctly |

### Blind spot found in the mechanical axis, as FR-009a requires recording

The **strict** name pattern used during planning did **not** match
`fileswnb.cpp:815` — the very defect the user reported — because the name
arrived from an accessor named `GetEquivalentPairNoticeName()`, which no
"looks like a file name" heuristic recognised. Only a broadened pattern caught
it.

A code-only inventory would therefore have shipped this feature **without fixing
the bug that prompted it**. The shipped guard uses the broadened pattern, and
this limitation is recorded here rather than left implicit.

### Why the user-interface walk had to be in a localized build

English resource strings are pure ASCII; ASCII is valid UTF-8; so an English
build composes a valid UTF-8 message and renders it correctly **even when the
composition is mixed**. Every composed-message defect in this inventory is
invisible in English. This is why FR-009c pins composed surfaces to all 9
shipped languages, and it is measured, not assumed.

---

## Scope gate (T033)

The estimate carried into implementation was ~119 candidate sites; the instrument
measured **100** after false positives were eliminated by proper call-delimiting,
of which 84 were in class. That is within the planned envelope, so scope was
**not** re-cut.

---

## Follow-on work recorded, not done here

1. **Plugin metadata encoding** — 13 of the 15 recorded sites are blocked on it.
2. **`src/tserver/tablist.cpp`** — same `dead-dispinfow` defect, separate binary.
3. **Share and configuration name encoding** — 2 recorded sites.
4. **The application-wide text-encoding change** feature 041 identified and
   deferred. Nothing found here changes that assessment: the per-call-site
   approach worked, and the guard now prevents silent regrowth.
