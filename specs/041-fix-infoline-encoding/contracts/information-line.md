# Contract: Information Line Display

**Feature**: 041-fix-infoline-encoding
**Type**: UI contract (desktop application)

What the line below a panel must show. Written so a reviewer can check it on
screen without reading code.

## C-1 — The name is the reference

The item name in the information line MUST equal the name the file system
stores, and therefore MUST equal the name the panel above shows for the same
item — character for character.

The panel is the practical reference: if the two disagree, the information line
is wrong.

## C-2 — Fields are independent

The line is built from several fields (name, size, date, time, attributes; the
arrangement is user-configurable). **No field may affect how another field is
rendered.** A field the application cannot represent may degrade only itself.

This is the heart of the defect: today the size field's separator determines
whether the *name* is readable.

## C-3 — Unrepresentable characters

A character that genuinely cannot be represented appears as the standard
replacement character `U+FFFD` (`�`).

- Only the offending character is replaced.
- The rest of that field still displays.
- Every other field still displays.
- Nothing is silently dropped.

This matches what Windows Explorer shows for the same name, so the two agree.

## C-4 — Locale formatting

Size, date and time are formatted exactly as the operating system's regional
settings prescribe, **and** are themselves displayed correctly — including
separators that are not plain ASCII, such as the non-breaking space Czech uses
for thousands.

Changing the regional settings changes the formatting, as it does today. No
setting in the application overrides it, and none is added.

## C-5 — Selection summary

When items are selected, the same line shows the summary ("N bytes in M files,
K directories selected"). It MUST be fully readable in every shipped
user-interface language — the localized words and the formatted number in one
line, both correct.

The same summary text appears in the Find dialog and must be correct there too.

## C-6 — Truncation

When the line is too narrow, it truncates with an ellipsis. Truncation MUST NOT
split a character — including a character made of a surrogate pair. A
replacement character in the line must always mean the name really contains
something unrepresentable (C-3), never that the line was cut in the wrong place.

## C-7 — What must not change

| Item | Required state |
|------|----------------|
| Which fields the line can show | unchanged |
| Default arrangement of fields | unchanged |
| User's ability to configure the arrangement | unchanged |
| Formatting rules (separators, date pattern) | unchanged — they come from the regional settings |
| Panel columns, directory line, size-reporting dialogs | no regression |
| Behaviour when a plugin composes the line itself | no regression |
| Responsiveness of panel navigation | no perceptible change |

## Verification

| Contract | How to check |
|----------|--------------|
| C-1 | Focus the reported file; compare the line against the panel above |
| C-2 | Same accented name at 999 bytes and at over 1 000 000 bytes — both correct |
| C-3 | A name containing an unrepresentable character shows `�` for it alone |
| C-4 | Sizes and dates match the regional settings; check one locale whose separators are not ASCII |
| C-5 | Select several files in each shipped language; read the summary. Repeat in the Find dialog |
| C-6 | Narrow the window until the line truncates; no split characters |
| C-7 | Visual sweep of the panel columns, directory line and size dialogs; hold an arrow key through a large directory |
