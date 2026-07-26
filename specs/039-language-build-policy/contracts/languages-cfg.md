# Contract: `translations/languages.cfg`

The maintainer-facing interface of this feature. One file, one section per
language, one line to flip.

## Format

```ini
[<folder>]
langid       = <integer>
display_name = <text>
author       = <text>
web          = <text>
comment      = <text>
helpdir      = <text>
origin       = human | mixed | machine
enabled      = on | off
```

- Encoding: UTF-8 **without BOM**.
- `#` starts a comment; it runs to end of line and may follow a value.
- Whitespace around `=` is insignificant. Section names are folder names under
  `translations/`.
- Field order within a section is insignificant. `enabled` is placed last by
  convention so it reads as the policy decision after the description.

## The `enabled` field

| Value | Meaning |
|---|---|
| `on` | Built for every enabled module; shipped. |
| `off` | Not built. Any existing module for it is removed from the output on the next build. Translation source is untouched. |

Required in every section. Any other value is an error.

## Guarantees

1. Changing one `enabled` value and rebuilding is sufficient to add or remove a
   language from the product. No other file changes. (SC-001)
2. Setting a language `off` never modifies `translations/<folder>/`. (FR-004)
3. Setting it back `on` restores byte-identical modules. (SC-004)
4. All languages `off` is legal and yields an English-only product. (FR-008)
5. English is not represented here and is never affected.

## Errors

Every error names the specific entry at fault and stops the build before MSBuild
runs.

| Condition | Message shape |
|---|---|
| Missing field(s) | `language [<folder>] is missing: enabled` |
| Bad `enabled` value | `language [<folder>] has enabled '<value>' (expected on or off)` |
| Bad `origin` value | `language [<folder>] has unknown origin '<value>' (expected human, mixed, or machine)` |
| Non-numeric `langid` | `language [<folder>] has non-numeric langid '<value>'` |
| Duplicate LANGID | `LANGID <n> used by both [<a>] and [<b>] -- must be unique` |
| Registered, no directory | `no translations\<folder>\ directory -- create it or remove the [<folder>] record` |
| Directory, not registered | `translations\<folder>\ exists but is not registered in languages.cfg` |
| No sections at all | `no languages defined` |

Validation covers **every** record, enabled or disabled.

## Current policy (feature 039)

```ini
enabled = on    czech, german, french, dutch, hungarian, romanian, slovak, spanish
enabled = off   chinesesimplified, russian, ukrainian
```

The three disabled ones render incorrectly in menus (known defect recorded in
spec.md, not fixed here). Re-enabling is a one-line change per language once it
is fixed.
