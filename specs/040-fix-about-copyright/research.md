# Research: Fix About Dialog Copyright Notice

**Feature**: 040-fix-about-copyright
**Date**: 2026-07-26

All open questions from the specification were resolved during
`/speckit.clarify`. This document records the technical findings that back the
implementation approach, plus the alternatives considered and rejected.

## Finding 1 — Where the defect actually lives

**Observation**: `src/lang/lang.rc:637-638` declares the two copyright lines as
ordinary translatable statics inside `IDD_ABOUT`:

```rc
LTEXT  "Copyright © 1997-2026 Open Salamander Authors",IDC_STATIC_1,10,97,196,8
LTEXT  "Copyright © 2026 Newt Commander Authors",IDC_STATIC_2,10,108,196,8
```

`CAboutDialog` is constructed with `CCommonDialog(HLanguage, IDD_ABOUT, parent)`
(`src/logo.cpp:392`), so the whole template — including these two captions —
comes from the active `.slg` language module, not from the executable. Nothing
in `WM_INITDIALOG` overrides them.

**Consequence**: whatever the translation says is what the user sees. Feature
038 machine-translated the two entries, and the machine rewrote the brand name
in line 1 ("Open Salamander Authors" → "Newt Commander Authors") and the year
(2026 → 2023) in 10 of 11 languages.

**Decision**: the text must stop coming from the language module.

## Finding 2 — The splash screen already solves this, and is the pattern to copy

`IDD_SPLASH` lives in `src/salamand.rc:187-195`, **not** in `lang.rc` — it is
part of the executable's own resources and is never translated. Its two
copyright controls are declared with an empty caption and dedicated IDs:

```rc
LTEXT  "",IDC_SPLASH_COPYRIGHT,8,73,237,8
LTEXT  "",IDC_SPLASH_COPYRIGHT2,8,83,237,8
```

At runtime `CSplashScreen` reads their rectangles, destroys the controls, and
paints `VERSINFO_COPYRIGHT1` / `VERSINFO_COPYRIGHT2` into the background bitmap
(`src/logo.cpp:240-248`).

**Decision**: reuse the *principle* (empty caption in the resource, text
supplied at runtime from a build-time constant) but not the *mechanism*.

**Rationale for not copying the mechanism**: the About dialog's statics are
already wired into its theme handling — `WM_CTLCOLORSTATIC` in
`CAboutDialog::DialogProc` (`src/logo.cpp:504-522`) picks the light/dark text
and background colour for every static in the dialog. Destroying the two
controls and hand-painting them would mean re-implementing that colour logic and
the dialog-font selection for these two lines only. Keeping them as real statics
and calling `SetDlgItemText` costs two lines of code and inherits the existing
theming for free.

**Precedent inside the very same dialog**: `IDC_ABOUT_WWW` is declared in
`lang.rc` with a placeholder caption and then overwritten at runtime
(`src/logo.cpp:492-498`). The approach is already established here.

## Finding 3 — The `.slt` format has no "do not translate" state

`specs/038-translations-build-integration/data-model.md:82-88` documents the
archive format:

| Block | Row shape |
|-------|-----------|
| `[DIALOG <id>]` | first row `cx,cy,state,"caption"`, then `ctrlID,x,y,cx,cy,state,"text"` |

and `state` is only `0` = untranslated, `1` = translated. There is no third
value meaning "never translate this".

**Decision**: express "nothing to translate" by making the *source* string
empty. An entry whose English original is empty has nothing for a translator or
a machine to produce, in any round, forever.

**Verified**: empty-caption controls do keep their row and their geometry —
`101,8,18,189,86,1,""` in `translations/czech/salamand.slt:18` is exactly such a
row. So the positional structure the import depends on is untouched, and the
state stays `1` (consistent with every other empty entry in the archives).

## Finding 4 — Clipping risk (FR-007) is eliminated, not merely mitigated

The commit message for feature 038's About-layout fix (`7903bd9`) records two
facts that settle this:

1. **Geometry is no longer carried between rounds.** "Coordinates always start
   from the template (the current English layout), which is deterministic and
   idempotent." So every language's `x,y,cx,cy` for these two controls is
   whatever English has.
2. **`tools/translate/layout.py` only widens, and only from the text.** "only
   left-aligned text controls … never moving or shrinking anything." With an
   empty source string there is nothing to widen from, so the width stays at the
   template value.

**Consequence**: after this change all 11 languages carry exactly the English
geometry `10,97,196,8` and `10,108,196,8`, and the runtime text is the English
text — which already renders inside that box in the English build today.

**Decision**: no runtime measure-and-resize code is needed. A runtime width-fit
was considered and rejected as speculative complexity: it would guard against a
scenario (a language module carrying narrower geometry for an empty English
string) that the tooling cannot now produce.

**Residual check**: the task list still includes a visual verification across
languages, because "cannot now produce" is an argument, not an observation.

## Finding 5 — Constant restructuring vs. the `LegalCopyright` invariant

`src/versinfo.rh2:19-24` currently holds:

```c
#define VERSINFO_COPYRIGHT   "Copyright © 1997-2026 Open Salamander Authors, © 2026 Newt Commander Authors"
#define VERSINFO_COPYRIGHT1  "Copyright © 1997-2026 Open Salamander Authors"
#define VERSINFO_COPYRIGHT2  "© 2026 Newt Commander Authors"
```

`VERSINFO_COPYRIGHT` is consumed by `src/plugins/shared/versinfo.rc2:57` as the
`LegalCopyright` version-resource value; FR-012 requires it to stay byte-identical.
`VERSINFO_COPYRIGHT1/2` are consumed only by `src/logo.cpp:240,245` — confirmed
by a whole-tree grep, so they can be renamed freely.

`VERSINFO_COPYRIGHT2` deliberately omits the word "Copyright" because it was
written as the tail of the concatenated `LegalCopyright` sentence. FR-002 and
FR-006 require both display lines to be self-contained, so the split has to be
redone.

**Decision**: replace `VERSINFO_COPYRIGHT1/2` with two self-contained, clearly
named display constants and leave `VERSINFO_COPYRIGHT` as its own literal:

```c
#define VERSINFO_COPYRIGHT          "Copyright © 1997-2026 Open Salamander Authors, © 2026 Newt Commander Authors"
#define VERSINFO_COPYRIGHT_NEWT     "Copyright © 2026 Newt Commander Authors"
#define VERSINFO_COPYRIGHT_OPENSAL  "Copyright © 1997-2026 Open Salamander Authors"
```

**Alternative considered — derive everything from year atoms**:

```c
#define VERSINFO_YEARS_OPENSAL "1997-2026"
#define VERSINFO_YEARS_NEWT    "2026"
#define VERSINFO_COPYRIGHT_OPENSAL "Copyright © " VERSINFO_YEARS_OPENSAL " Open Salamander Authors"
#define VERSINFO_COPYRIGHT  VERSINFO_COPYRIGHT_OPENSAL ", © " VERSINFO_YEARS_NEWT " Newt Commander Authors"
```

This would make the year literally single-source across metadata *and* display.
**Rejected**: it makes `LegalCopyright` depend on multi-level macro expansion
inside `rc.exe`, trading a documented one-line maintenance note for a build-time
risk in a file included by every version resource in the tree. FR-012's
byte-identity requirement is safest served by leaving that string a literal.
The existing "update these together" comment is retained and updated instead.

## Finding 6 — Naming: why not give the About statics dedicated IDs

The two controls use `IDC_STATIC_1` (1150) and `IDC_STATIC_2` (1151), generic
IDs reused across dozens of dialogs. Introducing `IDC_ABOUT_COPYRIGHT1/2` would
read better.

**Rejected for this feature.** The IDs are unique *within* `IDD_ABOUT`, so
`SetDlgItemText` targets them unambiguously; the reuse is only across dialogs.
Changing them would mean new entries in `lang.rh` plus rewriting the ID column
in 11 translation archives — the same edit surface, for cosmetic benefit, on a
bug fix. Constitution principle III (incremental modernization: "do not refactor
adjacent untouched code in the same change") points the same way.

## Finding 7 — Build and verification path

- `build.cmd full` is required, not a plain incremental build: language modules
  (`.slg`) are produced only on a full build
  (`specs/039-language-build-policy`), and this change edits translation
  archives.
- `OPENSAL_BUILD_DIR` is not set in this environment, so the build defaults to
  `.\build\` — acceptable, and the constitution only forbids hardcoded absolute
  paths in the scripts themselves.
- 8 of 11 languages are `enabled = on`. The 3 disabled ones (Chinese
  Simplified, Russian, Ukrainian) still get their archives edited (FR-009a,
  FR-010) but produce no module to inspect.

## Summary of decisions

| # | Decision | Drives |
|---|----------|--------|
| 1 | Text set at runtime via `SetDlgItemText`, controls kept | FR-004, FR-005 |
| 2 | Empty caption in `lang.rc` and in all 11 `.slt` archives | FR-009, FR-009a |
| 3 | Two self-contained display constants; `VERSINFO_COPYRIGHT` untouched | FR-006, FR-012 |
| 4 | Swap the two `PaintText` calls in the splash | FR-011, SC-007 |
| 5 | No runtime resize; rely on template geometry + visual check | FR-007 |
| 6 | Keep `IDC_STATIC_1/2` | Constitution III |
