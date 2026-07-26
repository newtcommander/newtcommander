# Research: Fix Information Line Encoding

**Feature**: 041-fix-infoline-encoding
**Date**: 2026-07-26

All three specification clarifications were resolved during `/speckit.clarify`.
This document records the technical findings behind the implementation approach,
including the bounded investigation FR-011 requires and the plugin review
surface FR-013 requires.

## Finding 1 — The exact mechanism, confirmed

The application carries narrow strings as **UTF-8** internally
(feature 004, `src/common/salunicode.h`). `CStatusWindow::SetText`
(`src/stswnd.cpp:126`) keeps a UTF-16 mirror of the line so measurement and
drawing run on the W APIs:

```cpp
TextW = SalU8ToWAlloc(Text);
TextLenW = TextW != NULL ? (int)wcslen(TextW) : 0;
```

`SalU8ToWAlloc` is documented as **strict**: "invalid input sequences fail
(return 0) instead of being silently replaced". When it returns `NULL`, every
consumer in `CStatusWindow` switches to the legacy byte-wise ANSI path —
`DrawTextSeg` (`src/stswnd.cpp:713`) calls `ExtTextOut` instead of
`ExtTextOutW`, and the UTF-8 bytes of the file name are rendered through the
system code page. That is the mojibake.

**What makes the string invalid.** `NumberToStr` (`src/salamdr1.cpp:2883`)
splices `ThousandsSeparator` into the digits. That separator is read once at
startup with the **ANSI** `GetLocaleInfo` (`src/salamdr1.cpp:956`):

```cpp
GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, ThousandsSeparator, 5)
```

Measured on this machine:

| | Value |
|---|---|
| Culture | `cs-CZ` |
| ANSI code page | 1250 |
| `LOCALE_STHOUSAND` | U+00A0 (non-breaking space) |
| As CP1250 bytes | `0xA0` |

A lone `0xA0` is a UTF-8 continuation byte with no lead byte — invalid. One such
byte anywhere in the line destroys the whole line.

**Decision**: fix the producers so locale-derived text is UTF-8 like everything
else, rather than teaching consumers to cope with mixed encodings.

## Finding 2 — Why the panel is unaffected and the line is not

`CFilesWindow`'s drawing code (`src/fileswn2.cpp`) converts **each column
separately**:

```cpp
wlen = SalU8ToW(formatedFileName, f->NameLen, wbuf, _countof(wbuf)) - 1;
```

The name column contains only the name — valid UTF-8 — so it converts and draws
correctly. The size column contains only digits and the separator; its
conversion fails, it falls back to the byte path, and `0xA0` happens to render
as a non-breaking space in CP1250, so nothing looks wrong.

The information line concatenates name, size, date, time and attributes into one
string and converts **once**. Failure is therefore all-or-nothing across the
whole line.

**Consequence for the design**: fixing the producers removes the cause, but the
all-or-nothing conversion remains a latent amplifier — any future stray byte
would again destroy the entire line rather than one field. FR-003a asks for
graceful degradation, which is exactly the right remedy for that amplifier.

**Decision**: do both. Fix the producers (FR-010) *and* make the line's
conversion lenient (FR-003a) so a single bad character can only ever cost that
character.

## Finding 3 — A lenient conversion is needed, and does not exist yet

`salunicode.h` deliberately offers only strict conversions, because its original
consumers were file operations where silent substitution would be dangerous:
"Stored names are NEVER normalized or case-folded by these helpers."

Display is a different contract. A name that cannot be represented should be
*shown* with `U+FFFD` (FR-003a), never silently dropped and never allowed to
corrupt its neighbours.

**Decision**: add a display-only lenient conversion alongside the strict pair,
clearly named and documented as display-only, and use it in `CStatusWindow`.
The strict helpers keep their current semantics and current callers untouched.

**Alternative considered — make `SalU8ToW` itself lenient.** Rejected: it would
silently change behaviour for every file-operation caller, exactly the class of
caller the strict contract exists to protect. Constitution principle III also
argues against altering adjacent untouched code.

## Finding 4 — FR-011 bounded investigation: where else locale text is produced

Search of the main application for the ANSI locale APIs
(`GetLocaleInfo`, `GetDateFormat`, `GetTimeFormat`): **42 call sites**, all in
`src/*.cpp` — none in `src/common/` or `src/shellext/`.

Exact distribution:

| File | Sites |
|------|-------|
| `finddlg1.cpp` | 7 |
| `fileswn2.cpp` | 5 |
| `bugreprt.cpp` | 5 |
| `worker.cpp` | 4 |
| `salamdr4.cpp` | 4 |
| `execute.cpp` | 4 |
| `salamdr1.cpp` | 3 |
| `packac.cpp` | 2 |
| `fileswn4.cpp` | 2 |
| `dialogs5.cpp` | 2 |
| `dialogs.cpp` | 2 |
| `salamdr2.cpp` | 1 |
| `mainwnd1.cpp` | 1 |

They fall into three groups. **36 sites remain to be classified individually**;
the entries below are the ones traced during this research, not the complete
classification — producing that classification is itself a task (FR-011).

### Group A — produces text that reaches a UTF-8 consumer (must be fixed)

Traced so far:

| Site | What it feeds |
|------|---------------|
| `salamdr1.cpp:944,956` | `DecimalSeparator` / `ThousandsSeparator` → `NumberToStr`, `NumberToStr2`, `PointToLocalDecimalSeparator` → information line, selection summary, panel size column, dialogs, plugins |
| `execute.cpp:1066,1090,1121,1145` | date and time fields of the information line |
| `fileswn2.cpp:3762,3855,4026,4037` | panel date/time columns, already converted with `SalU8ToW` under a comment that assumes UTF-8 — the assumption is currently wrong |
| `salamdr4.cpp` (4 sites) | column text via `TransferBuffer` |
| `worker.cpp` (4 sites) | progress/error text shown to the user |
| `finddlg1.cpp` (7 sites) | Find dialog date fields |

Not yet traced, expected to be Group A but to be confirmed: `packac.cpp` (2),
`fileswn4.cpp` (2), `dialogs5.cpp` (2), `dialogs.cpp` (2), `mainwnd1.cpp` (1),
and the third `salamdr1.cpp` site.

### Group B — confirmed second victim of the same cause

`ExpandPluralBytesFilesDirs` (`src/salamdr4.cpp:673`) builds the selection
summary as `LoadStr(...)` — localized UTF-8 from the `.slg` — combined with
`NumberToStr(...)` — ANSI separator:

```cpp
ret = sprintf(lpOut, expanded, NumberToStr(number, selectedBytes), files);
```

In a localized user interface the plural text carries diacritics, so the whole
summary is garbled. Two callers: the information line
(`fileswnb.cpp:990`) and the **Find dialog** status (`finddlg2.cpp:284`). The
Find dialog is a surface the report did not mention and the investigation found.

### Group C — provisionally recorded as unaffected

| Site | Why unaffected |
|------|----------------|
| `bugreprt.cpp` (5 sites) | writes the crash-report text file, not a UI surface; consumed as ANSI end to end |
| `salamdr2.cpp:LOCALE_SLANGUAGE` | language-name lookup used for matching, not display |

**6 sites provisionally in Group C.** Each still needs its reason confirmed and
recorded in the validation results, per FR-011 — "provisionally" because the
reason above was reasoned from the call site, not traced to every consumer.

**Note on `GetDateFormat` returning ASCII by luck.** For `cs-CZ` the short date
is `dd.MM.yyyy` and the time `H:mm:ss` — pure ASCII, which is why the date and
time in the reported screenshot look fine. The size was the only poison *on this
machine*. Locales whose formats contain non-ASCII characters would poison the
line the same way, which is why Group A includes the date/time sites even though
they are not reproducible here.

## Finding 5 — FR-013 plugin review surface

`NumberToStr` and `PointToLocalDecimalSeparator` are published to plugins as
virtual methods on `CSalamanderGeneral` (`src/plugins/shared/spl_gen.h:1411`,
`src/plugins.h:2336`), implemented as thin forwarders in `src/zip.cpp`.

Plugins referencing either symbol — **18** of the 28 in the tree:

```text
7zip, automation, checksum, demoplug, filecomp, ftp, pictview, regedt,
renamer, shared, tar, uncab, unchm, uniso, unmime, unrar, undelete, zip
```

**Expected outcome, to be confirmed per plugin**: they call the method and put
the result on screen through the normal UTF-8 display path, so correcting the
encoding fixes them rather than breaking them. A plugin would only be harmed if
it fed the result straight into an ANSI-only API, which is already broken today
for any non-ASCII locale.

**No header or ABI change.** `spl_gen.h` is untouched; the separators
themselves are not exported (`src/consts.h` is not included by any plugin —
verified). Widening the separator buffers is therefore an internal change.

## Finding 6 — Buffer sizes

`DecimalSeparator[5]` and `ThousandsSeparator[5]` (`src/salamdr1.cpp:135,137`)
size for the ANSI form. `LOCALE_SDECIMAL` and `LOCALE_STHOUSAND` are at most
3 characters plus the terminator; in UTF-8 that is up to 12 bytes plus
terminator.

**Decision**: widen both to 16 bytes. `src/consts.h:1674,1676` declares them
`extern char …[5]`; the declaration and the definition move together and no
plugin sees either.

## Finding 7 — FR-007, truncation must not split a character

Once the line is always valid UTF-8 the wide mirror is always present, so the
existing indexing already works in WCHAR units (`src/stswnd.cpp:913-916`) rather
than bytes — a UTF-8 sequence can no longer be cut apart.

One gap remains: a character outside the Basic Multilingual Plane is two WCHARs,
and a WCHAR-level cut can still land between them.

**Decision**: treat surrogate-pair-safe truncation as an explicit check rather
than an assumption. It is cheap (do not cut between a high and a low surrogate)
and FR-007 states it unconditionally.

## Finding 8 — Verification environment

The Debug build tree (`build\newtcommander\Debug_x64\`) was removed between
feature 040 and this plan; the tree now holds a Release build. Verification
therefore starts with a fresh `build.cmd full`.

The reported file is present at
`temp/Epizoda IV – Nová naděje (Despecialized) - původní kinodabing  CZ dabing.mkv`
(1 948 456 197 bytes), so the primary scenario reproduces without fixtures. SC-002
and SC-003 need a small fixture set: names covering diacritics, a typographic
dash, a non-Latin script, and the same accented name at 999 bytes and above
1 000 000 bytes.

The GUI can be driven without a human: match the main window by class
`NewtCommanderMainWindowVer01`, read control text with `GetWindowTextW`, capture
with `PrintWindow`. The information line is a child of the panel rather than a
dialog control, so its text is read from `CStatusWindow`'s window text or
captured visually — to be settled in the task list.

## Summary of decisions

| # | Decision | Drives |
|---|----------|--------|
| 1 | Locale-derived text becomes UTF-8 at the producer | FR-004, FR-010 |
| 2 | New display-only lenient UTF-8→UTF-16 conversion, used by the information line | FR-003, FR-003a |
| 3 | Strict `SalU8ToW*` semantics and their callers stay untouched | Constitution III |
| 4 | All 42 locale sites classified; Group A converted, Group C recorded | FR-011, SC-007 |
| 5 | Selection summary and the Find dialog fixed as co-victims | FR-005, US2 |
| 6 | Separator buffers widened to 16 bytes; no header exposed to plugins | FR-012 |
| 7 | Plugin interface unchanged; 18 plugins reviewed and recorded | FR-012, FR-013, SC-008 |
| 8 | Surrogate-pair-safe truncation verified, not assumed | FR-007 |
