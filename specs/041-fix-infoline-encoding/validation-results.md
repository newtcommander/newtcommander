# Validation Results: Fix Information Line Encoding

**Feature**: 041-fix-infoline-encoding
**Date**: 2026-07-26
**Build**: Debug x64, BUILD SUCCEEDED

Verification was performed against the running application: the information line
is drawn by `CStatusWindow`, which keeps its text in a member and never calls
`SetWindowText`, so it cannot be read with `GetWindowText` — every check below is
a screen capture read visually.

## The reported defect

**Before** (Czech, focused `Epizoda IV – Nová naděje (Despecialized) - původní kinodabing  CZ dabing.mkv`):

```text
Epizoda IV â€" NovÃ¡ nadÄ›je (Despecialized) - pÅ¯vodnÃ­ kinodabing  CZ dabing.mkv: 1 948 456 197, 24.01.2018, 21:56:25, A, …
```

**After**:

```text
Epizoda IV – Nová naděje (Despecialized) - původní kinodabing  CZ dabing.mkv: 1 948 456 197, 24.01.2018, 21:56:25, A, …
```

The panel above shows the same name; the two now agree character for character.

## Root cause, confirmed by measurement

| | Value |
|---|---|
| Culture | `cs-CZ` |
| ANSI code page | 1250 |
| `LOCALE_STHOUSAND` | U+00A0 (non-breaking space) |
| As CP1250 | the single byte `0xA0` |

`0xA0` alone is invalid UTF-8. `NumberToStr` spliced it into the size, the
composed line became invalid, `SalU8ToWAlloc` (strict) returned `NULL`, and
`CStatusWindow` fell back to byte-wise ANSI drawing — which is what rendered the
UTF-8 file name as mojibake. The panel escaped because it converts each column
separately, and the size column contains nothing but digits and the separator.

## Requirement coverage

| ID | Requirement | Result | Evidence |
|----|-------------|--------|----------|
| FR-001 | Name shown exactly as stored | PASS | reported file + 11 fixtures |
| FR-002 | Information line matches the panel | PASS | side-by-side capture |
| FR-003 | No field corrupts another | PASS | 999 B vs 2 000 000 B give the same name |
| FR-003a | Unrepresentable character → `U+FFFD`, nothing else affected | PASS | see below |
| FR-004 | Locale formatting correct and correctly displayed | PASS | `1 948 456 197`, `2 000 000` |
| FR-005 | Selection summary readable in every language | PASS | 9/9 languages |
| FR-006 | Holds for any field arrangement | PASS | default layout; layout is data-driven from the same string |
| FR-007 | Truncation never splits a character | PASS | 12-width sweep over a surrogate-pair name |
| FR-008 | No regression elsewhere | **PARTIAL** | see "Regressions found and fixed" and "Not verified" |
| FR-009 | No user action, no new option | PASS | nothing added to configuration |
| FR-010 | Fixed at the cause | PASS | locale text is UTF-8 at the producer |
| FR-011 | All locale sites classified | PASS | 42/42, table below |
| FR-012 | Plugin interface unchanged | PASS | `src/plugins/` has no diff; `spl_gen.h` untouched |
| FR-013 | In-repo plugins reviewed | **PARTIAL** | 18/18 classified, 8 not runtime-verified |

## Success criteria

| ID | Criterion | Result |
|----|-----------|--------|
| SC-001 | Reported file correct | PASS — 0 garbled characters |
| SC-002 | Fixture names match the panel | PASS — 11/11 |
| SC-003 | Threshold no longer matters | PASS — same name correct at 999 B and 2 000 000 B |
| SC-004 | Summary readable in every language | PASS — 9/9 |
| SC-005 | Locale formatting matches settings | PARTIAL — verified on `cs-CZ`; see "Not verified" |
| SC-006 | No regression in other displays | PARTIAL — see below |
| SC-007 | Every located surface accounted for | PASS — 42/42 |
| SC-008 | Every plugin has a recorded outcome | PASS for the review; 8 lack runtime verification |
| SC-009 | Interface version unchanged, plugins load | PASS — `LAST_VERSION_OF_SALAMANDER` untouched |

## FR-011 — classification of all 42 locale call sites

**Group A — output reaches a UTF-8 consumer (converted): 36 sites**

| File | Sites | Feeds |
|------|-------|-------|
| `salamdr1.cpp` | 2 | `DecimalSeparator`, `ThousandsSeparator` → `NumberToStr` → information line, summary, panel, plugins |
| `execute.cpp` | 4 | information-line date/time |
| `fileswn2.cpp` | 5 | panel date/time columns + `LOCALE_SSHORTDATE` width measurement |
| `salamdr4.cpp` | 4 | internal column text |
| `worker.cpp` | 4 | progress and error text |
| `finddlg1.cpp` | 7 | Find dialog date/time |
| `dialogs.cpp` | 2 | beta-expiry long date |
| `dialogs5.cpp` | 2 | task list date/time |
| `fileswn4.cpp` | 2 | date+time combined with `LoadStr(IDS_INVALID_DATEORTIME)` |
| `packac.cpp` | 2 | archive date/time |
| `mainwnd1.cpp` | 1 | beta-expiry long date |
| `salamdr2.cpp` | 1 | `LOCALE_SLANGUAGE` — the language name, **displayed** in the chooser |

`salamdr2.cpp` is worth calling out: the plan had provisionally filed it as
exempt ("used for matching, not display"). Tracing it showed it is
`CLanguage::GetLanguageName`, which is displayed. The provisional guess was
wrong; the trace corrected it.

**Group C — cannot reach a UTF-8 consumer (left alone): 6 sites**

| File | Sites | Why exempt |
|------|-------|-----------|
| `bugreprt.cpp` | 5 | `LOCALE_ICOUNTRY`, `SENGCOUNTRY`, `ILANGUAGE`, `SENGLANGUAGE`, `IDEFAULTANSICODEPAGE` — numeric or English-only values appended to the crash-report **text file**, never a UI surface |
| `salamdr1.cpp:3824` | 1 | `LOCALE_IDEFAULTANSICODEPAGE` consumed by `atoi()` for charset detection; never displayed |

36 + 6 = 42. A grep for the ANSI APIs now returns exactly these 6.

## FR-003a — unrepresentable character

A file whose name contains an unpaired UTF-16 high surrogate (legal on NTFS,
created here with `CreateFileW`) displays as:

```text
Lone�surrogate.txt: 0, 26.07.2026, 20:18:19, A, Lone�surrogate.txt
```

One replacement character, exactly where the offending character is. `Lone`,
`surrogate.txt`, the size, the date, the time and the attributes are all intact.

## FR-007 — truncation

The window was swept across 12 widths with a name built from 36 emoji (every
one a UTF-16 surrogate pair), forcing the cut to land at many different offsets.
Every visible glyph is whole and the ellipsis always follows a complete
character — no lone surrogate halves.

## Regressions found and fixed during implementation

Two regressions were introduced by this work and caught by the verification
steps. Both are recorded because the second changed the design.

### 1. Selection summary showed replacement characters

Making the separator UTF-8 while `LoadStr` still returned ANSI made the summary
a **mixed-encoding** string. Combined with the new lenient conversion, the
Czech summary read `2 014 499 bait� v 12 vybran�ch souborech`.

`LoadStr` uses `LoadStringA`, which transcodes the UTF-16 resource to the system
ANSI code page. That is a pre-existing hole in the application's "narrow strings
are UTF-8" contract, not something this feature introduced — but this feature
made it visible.

### 2. Converting `LoadStr` globally broke the Find dialog

The first attempt was to make `LoadStr` return UTF-8 — the fix at the true
cause. It fixed the summary, and it broke the Find dialog, whose list and status
bar still use ANSI display APIs: `Vyhledat soubory a adresÃ¡Å™e`, `NÃ¡zev`,
`13888 vybranÃ½ch souborĹŻ`.

**That attempt was reverted.** Converting `LoadStr` for the whole application is
a far larger change than this defect warrants and would need its own feature.

**What shipped instead**: `LoadStrU8()` alongside `LoadStr()`, used only where
text is composed with UTF-8 data and drawn through the UTF-8 path — the
information line. `ExpandPluralFilesDirs` / `ExpandPluralBytesFilesDirs` take an
optional `u8` flag (default `FALSE`), set only by the information-line callers.
Every other caller — dialogs, captions, the Find dialog, plugins through
`CSalamanderGeneral` — keeps the ANSI behaviour it had.

### 3. Find dialog sizes showed `2Â 055`

A consequence of `NumberToStr` now returning UTF-8: the Find results list has
both a wide and an ANSI `LVN_GETDISPINFO` handler, and the ANSI one was handing
the raw UTF-8 bytes to the control. It now transcodes. Sizes read `2 065`,
`1 147` again and the whole dialog is correct.

## FR-013 — plugin review (18 plugins)

The interface itself is unchanged; what changed is that `NumberToStr` and
`PointToLocalDecimalSeparator` now return UTF-8.

| Category | Plugins | Outcome |
|----------|---------|---------|
| Writes `TransferBuffer` for a panel column | 7zip, ftp, regedt, renamer, unrar, zip | **Improved.** The main application converts each column with `SalU8ToW`; UTF-8 is what that path wants. |
| Builds the archive info-line string (`"%s, %s, %s"` with the number) | 7zip, tar, uncab, unchm, undelete, uniso, unmime, unrar, zip | **Improved.** This text is drawn by the information line — the very surface this feature fixes. Previously it carried the same defect. |
| Formats a number into plugin dialog text | automation, checksum, filecomp, ftp, pictview, regedt, renamer, undelete | **Not runtime-verified** — see below. |
| Has its own private `NumberToStr` | zip/selfextr | **Unaffected.** Standalone self-extractor, does not use the plugin API. |
| Demo only, disabled in `plugins.cfg` | demoplug | **Not applicable.** |

## Not verified

Stated plainly rather than assumed:

- **Plugin dialogs that display a formatted number** (8 plugins above). If such
  a dialog sets its text with a plain ANSI Win32 call, the UTF-8 separator will
  render as `Â ` exactly as the Find dialog did. Reproducing each needs its own
  scenario (an FTP server, specific archives, a deleted-file volume), which was
  out of reach here. This is the most likely place for a residual defect.
- **A locale whose date or time format contains non-ASCII characters**
  (SC-005, T030). Verifying it means changing the machine's Windows regional
  settings, which is a change to the user's system and was not made without
  asking. The date/time sites were converted regardless, so the code path is in
  place — it is the observation that is missing.
- **Size-reporting dialogs and archive browsing** (part of T028). The panel
  columns, the directory line and the Find dialog were checked; the
  occupied-space dialog needs a selection-driven flow that the harness did not
  reach.

## Performance

The information line is rebuilt on every focus change. 400 consecutive Down
presses over `src\`: **0.12 ms of process CPU per focus change**. No perceptible
change to panel navigation.

## Changed files

| File | Change |
|------|--------|
| `src/common/salunicode.h/.cpp` | `SalU8ToWDisplay(Alloc)` (lenient, display-only) and `SalGetLocaleInfoU8` / `SalGetDateFormatU8` / `SalGetTimeFormatU8`. Strict helpers untouched. |
| `src/consts.h` | separators widened to `char[16]`, documented as UTF-8; `LoadStrU8` declared; `u8` flag on the plural helpers |
| `src/salamdr1.cpp` | separators read as UTF-8; a >4-byte separator is refused so `NumberToStr`'s 50-byte caller buffer cannot overflow |
| `src/salamdr2.cpp` | `LoadStrU8` added next to `LoadStr` |
| `src/salamdr4.cpp` | plural helpers select the loader from the `u8` flag |
| `src/stswnd.cpp` | wide mirror built leniently; surrogate-pair-safe truncation; hidden-count text in UTF-8 |
| `src/execute.cpp`, `fileswn2/3/4.cpp`, `fileswnb.cpp`, `finddlg1.cpp`, `dialogs.cpp`, `dialogs5.cpp`, `mainwnd1.cpp`, `packac.cpp`, `worker.cpp` | Group A conversions; Find dialog ANSI notification transcodes |

`src/plugins/` has no diff.

## Observation outside this feature's scope

`src/finddlg1.cpp:3865` fails `clang-format`. The deviation predates this work
(it is inside a feature-004 comment block, far from any line changed here) and
was left alone, per the constitution's rule against refactoring adjacent
untouched code.
