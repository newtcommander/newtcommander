# Contract: Locale-Derived Text Is UTF-8

**Feature**: 041-fix-infoline-encoding
**Type**: internal API contract (main application)

The application's narrow strings are UTF-8 (feature 004). This contract closes
the last hole in that rule: text obtained from the operating system's regional
settings.

## C-1 — The rule

> Every narrow string the application obtains from regional settings is **valid
> UTF-8 from the moment it is obtained**.

No caller may receive the system's legacy single-byte form. The conversion
happens once, at the boundary, not at each consumer.

## C-2 — The boundary helpers

All access to regional settings goes through UTF-8 wrappers. Direct use of the
ANSI `GetLocaleInfo`, `GetDateFormat` or `GetTimeFormat` is not permitted in code
whose output reaches a UTF-8 consumer.

| Wrapper | Wraps | Returns |
|---------|-------|---------|
| locale-info wrapper | `GetLocaleInfoW` | UTF-8 |
| short/long date wrapper | `GetDateFormatW` | UTF-8 |
| time wrapper | `GetTimeFormatW` | UTF-8 |

Each converts the W result to UTF-8 and reports failure the way its existing
callers already handle (the current sites all fall back to a hand-built ASCII
form when the API returns 0 — that fallback stays).

**Exempt**: code whose output never reaches a UTF-8 consumer — currently the
crash-report writer and the language-name lookup used for matching. Exemptions
are recorded explicitly, not assumed (FR-011).

## C-3 — Content is unchanged

Only the encoding changes. The characters, their order, the choice of separator
and the date/time pattern remain exactly what the regional settings prescribe.
A user who changes their regional settings sees the change take effect as
before.

## C-4 — Storage

| Symbol | Requirement |
|--------|-------------|
| `DecimalSeparator` | at least 16 bytes, UTF-8, null-terminated |
| `ThousandsSeparator` | at least 16 bytes, UTF-8, null-terminated |
| `DecimalSeparatorLen` / `ThousandsSeparatorLen` | length in **bytes**, consistent with the buffer contents |

The lengths are byte counts both before and after; callers `memcpy` by them, so
their meaning must not silently change from characters to bytes or vice versa.

## C-5 — Published to plugins

`CSalamanderGeneral::NumberToStr` and
`CSalamanderGeneral::PointToLocalDecimalSeparator` return UTF-8 after this
change.

| Aspect | Requirement |
|--------|-------------|
| Method signatures | unchanged |
| Virtual table layout / ordering | unchanged |
| Plugin interface version | unchanged (104) |
| `spl_gen.h` | not modified |
| Buffer size expected of the caller | unchanged (`char[50]`) — 16-byte separators still fit a 20-digit number with separators |

Every plugin in this repository that calls either method gets a recorded review
outcome (FR-013).

## C-6 — Display conversion is separate and lenient

| Path | Conversion | On malformed input |
|------|-----------|--------------------|
| File operations, names, paths | strict (`SalU8ToW` / `SalWToU8`) — **unchanged** | fails; the caller surfaces a per-item error |
| Display (information line) | lenient, display-only | substitutes `U+FFFD` for the offending characters and continues |

The lenient variant must be named and documented so it cannot be mistaken for a
general-purpose conversion. It must never be used on a value that will be
written back to a name, a path, or anything persisted.

## Verification

| Contract | How to check |
|----------|--------------|
| C-1, C-2 | No ANSI locale call remains in the classified Group A; each Group C exemption has a recorded reason |
| C-3 | Sizes, dates and times on screen match the regional settings, before and after |
| C-4 | Compile-checked; separator length still measured in bytes |
| C-5 | `spl_gen.h` unchanged in the diff; interface version unchanged; every shipped plugin loads and runs |
| C-6 | Strict helpers' signatures and callers unchanged in the diff |
