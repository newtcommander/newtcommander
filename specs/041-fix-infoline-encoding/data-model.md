# Data Model: Fix Information Line Encoding

**Feature**: 041-fix-infoline-encoding
**Date**: 2026-07-26

There is no persisted data. The "model" is a set of in-memory strings, the
encoding each carries, and who produces and consumes them.

## Entities

### 1. Locale-derived text

Text the application obtains from the operating system's regional settings.

| Attribute | Before | After |
|-----------|--------|-------|
| `DecimalSeparator` | `char[5]`, system ANSI code page | `char[16]`, UTF-8 |
| `ThousandsSeparator` | `char[5]`, system ANSI code page | `char[16]`, UTF-8 |
| Short date text | ANSI, from `GetDateFormat` | UTF-8, via the wrapper |
| Time text | ANSI, from `GetTimeFormat` | UTF-8, via the wrapper |

Declared in `src/consts.h:1674,1676`, defined in `src/salamdr1.cpp:135,137`,
initialised in `src/salamdr1.cpp:944-966`. Not visible to plugins — no plugin
includes `consts.h` (verified).

**Sizing**: `LOCALE_SDECIMAL` and `LOCALE_STHOUSAND` are at most 3 characters
plus terminator; 12 UTF-8 bytes plus terminator fits in 16 with room to spare.

### 2. Composed display string

The string a display surface builds from several parts and converts once.

| Attribute | Value |
|-----------|-------|
| Encoding | UTF-8 (the application's internal contract, feature 004) |
| Producers | file name (UTF-8, from the file system), `NumberToStr` (locale-derived), date/time (locale-derived), `LoadStr` (UTF-8, from the language module) |
| Consumer | `CStatusWindow::SetText` → UTF-16 mirror → W drawing APIs |
| Failure mode today | one non-UTF-8 byte invalidates the entire string |
| Failure mode after | a character that cannot be represented becomes `U+FFFD`; everything else still displays |

### 3. Wide mirror

`CStatusWindow::TextW` / `TextLenW` (`src/stswnd.cpp:158-161`) — the UTF-16
copy that measurement, hit-testing, truncation and drawing all index into.

| Attribute | Before | After |
|-----------|--------|-------|
| Built with | `SalU8ToWAlloc` (strict) | lenient display conversion |
| `NULL` when | input is not valid UTF-8 | never, for any input |
| Consequence of `NULL` | every consumer falls back to byte-wise ANSI | — |

Making this mirror always present is what turns "all-or-nothing" into
"per-character".

### 4. Shared number formatting

| Attribute | Value |
|-----------|-------|
| Functions | `NumberToStr`, `NumberToStr2` (`src/salamdr1.cpp:2883,2901`), `PointToLocalDecimalSeparator` (`:2920`) |
| Published to plugins as | `CSalamanderGeneral::NumberToStr`, `CSalamanderGeneral::PointToLocalDecimalSeparator` (`src/plugins/shared/spl_gen.h:1411`, `src/plugins.h:2336`) |
| Forwarders | `src/zip.cpp:1397,5170` |
| Signature / ABI | **unchanged** |
| Output encoding | ANSI → **UTF-8** |
| In-repo plugin callers | 18 (see research.md finding 5) |

## Relationships

```text
Regional settings (OS)
   │  GetLocaleInfoW / GetDateFormatW / GetTimeFormatW
   ▼
UTF-8 locale wrappers ─────────────────┐
   │                                   │
   ├─► DecimalSeparator ──┐            ├─► date / time fields
   └─► ThousandsSeparator ┤            │
                          ▼            │
              NumberToStr / NumberToStr2 / PointToLocalDecimalSeparator
                          │            │
      ┌───────────────────┼────────────┴───────────────┬─────────────────┐
      ▼                   ▼                            ▼                 ▼
 information line   selection summary            panel columns     plugins (18)
 (name + size +     (LoadStr + number)           (per column)      via CSalamanderGeneral
  date + time)            │
      │                   ├─► information line
      │                   └─► Find dialog status
      ▼
 CStatusWindow::SetText ──► lenient UTF-8→UTF-16 ──► W drawing APIs
```

The file name enters the composed string already in UTF-8 from the file system;
it is never the malformed part. It is only the victim.

## Invariants

- **INV-1**: Every narrow string the application produces from regional
  settings is valid UTF-8 from the moment it is produced. (FR-004, FR-010)
- **INV-2**: `CStatusWindow`'s wide mirror is non-`NULL` for every input, so no
  input can push the line onto the byte-wise path. (FR-003)
- **INV-3**: A character that cannot be represented appears as `U+FFFD` and
  affects nothing but itself; nothing is silently dropped. (FR-003a)
- **INV-4**: The name shown in the information line equals the name shown in the
  panel for the same item, character for character. (FR-002)
- **INV-5**: The plugin interface's shape, version and binary compatibility are
  unchanged; only the encoding of returned content differs. (FR-012)
- **INV-6**: The strict `SalU8ToW` / `SalWToU8` contract and its existing
  callers are unchanged — file operations still refuse malformed names rather
  than substituting characters. (Constitution III)
- **INV-7**: Truncation never splits a character, including one made of a
  surrogate pair. (FR-007)
- **INV-8**: Numbers, dates and times are still formatted exactly as the
  regional settings prescribe — only their encoding changes, never their
  content. (FR-004, FR-009)

## State transitions

None. The strings are rebuilt from scratch on every focus or selection change;
there is no lifecycle to model.
