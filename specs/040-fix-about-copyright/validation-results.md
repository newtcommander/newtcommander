# Validation Results: Fix About Dialog Copyright Notice

**Feature**: 040-fix-about-copyright
**Date**: 2026-07-26
**Build**: Debug x64, `build.cmd full`, BUILD SUCCEEDED, 19 plugins, 180 language modules

All verification was performed against the running application. The two About
dialog copyright statics were read directly with `GetWindowText` (exact bytes,
not OCR), and the dialog was captured with `PrintWindow`.

## Requirement coverage

| ID | Requirement | Result | Evidence |
|----|-------------|--------|----------|
| FR-001 | Exactly two copyright lines, one above the other | PASS | `IDC_STATIC_1` at y=97, `IDC_STATIC_2` at y=108 |
| FR-002 | Line 1 = `Copyright © 2026 Newt Commander Authors` | PASS | read back verbatim in all 9 languages |
| FR-003 | Line 2 = `Copyright © 1997-2026 Open Salamander Authors` | PASS | read back verbatim in all 9 languages |
| FR-004 | English regardless of UI language, byte-identical | PASS | 9/9 languages exact-match (`-ceq`) |
| FR-005 | Content never sourced from a language module | PASS | T018 poison test — see below |
| FR-006 | One self-contained build-time definition per holder | PASS | `VERSINFO_COPYRIGHT_NEWT` / `_OPENSAL` in `src/versinfo.rh2` |
| FR-007 | No clipping, wrapping, truncation or overlap | PASS | screenshots, light + dark theme |
| FR-008 | Rest of the About dialog unchanged | PASS | 12 controls, same IDs/geometry, dialog 299x184 |
| FR-009 | Empty caption in English resource and every archive | PASS | `lang.rc` + 11 archives |
| FR-009a | No non-empty copyright row in any archive | PASS | grep returns 0 (was 22) |
| FR-010 | Applies to disabled languages too | PASS | zh-CN, ru, uk archives corrected |
| FR-011 | Splash screen in the same order | PASS | splash screenshot |
| FR-012 | `LegalCopyright` byte-identical | PASS | version resource of the built exe |

## Success criteria

| ID | Criterion | Before | After |
|----|-----------|--------|-------|
| SC-001 | Languages showing a wrong notice | 11 of 11 | **0 of 11** |
| SC-002 | "Open Salamander Authors" present in About | 0% | **100%** |
| SC-003 | Years `2026` / `1997-2026` | wrong in 10 of 11 | **correct in all** |
| SC-004 | Corrupted archive changes the display | yes | **no** (poison test) |
| SC-004a | Non-empty copyright rows across archives | 22 | **0** |
| SC-005 | Both lines readable, light + dark | — | **PASS** |
| SC-006 | Year update = one define per holder | 3 scattered defines | **`versinfo.rh2` only** |
| SC-007 | Splash and About agree on order | disagreed | **agree** |

## Per-language results (final build)

`RESULT: PASS` means both lines matched the expected literals exactly,
case- and byte-sensitive.

| Language | Line 1 | Line 2 | Verdict |
|----------|--------|--------|---------|
| English | `Copyright © 2026 Newt Commander Authors` | `Copyright © 1997-2026 Open Salamander Authors` | PASS |
| Czech | identical | identical | PASS |
| German | identical | identical | PASS |
| French | identical | identical | PASS |
| Dutch | identical | identical | PASS |
| Hungarian | identical | identical | PASS |
| Romanian | identical | identical | PASS |
| Slovak | identical | identical | PASS |
| Spanish | identical | identical | PASS |

**9 / 9 PASS, 0 FAIL.**

Chinese (Simplified), Russian and Ukrainian are disabled by the language build
policy and produce no module, so there is nothing to display-check. Their
archives were corrected (FR-010), so re-enabling any of them ships the correct
notice.

## Key evidence

### 1. The runtime override works against genuinely corrupted archives

Phase 3 was verified **before** any translation archive was touched. At that
point `translations/czech/salamand.slt` still read:

```text
1150,10,97,196,8,1,"Copyright © 1997-2023 Newt Commander Authors"
1151,10,108,196,8,1,"Autorská práva © 2026 Autoři Newt Commander"
```

and the built `czech.slg` carried that text — yet the dialog displayed the
correct English notice. That is FR-005 demonstrated against the real defect,
not a synthetic one.

### 2. Poison test (T018)

Arbitrary text was injected into the Czech archive's two rows, the language
module was rebuilt, and the application was run under Czech:

```text
injected: "POISON LINE ONE -- should never be displayed"
          "OTRAVA RADEK DVA -- nikdy se nesmi zobrazit"

LINE1 = [Copyright © 2026 Newt Commander Authors]
LINE2 = [Copyright © 1997-2026 Open Salamander Authors]
RESULT: PASS
```

The rows were then restored to empty and the poison confirmed absent from the
rebuilt `czech.slg`.

### 3. The notice is gone from the translation pipeline

After the change, the regenerated English template
(`build/newtcommander/translator/templates/salamand.slt`) reads:

```text
1150,10,97,196,8,1,""
1151,10,108,196,8,1,""
```

There is no source text for a translator or a machine to translate, in this
round or any future one. A `merge --dry-run` for Czech reports 35 unrelated
gaps and 0 validation failures.

### 4. Archive integrity

All 11 archives: UTF-8 BOM present, CRLF exclusively, **3444 rows each** —
identical to the T001 baseline. `git diff --stat` over `translations/` shows
`22 insertions(+), 22 deletions(-)` across 11 files: exactly two rows per file,
nothing else.

## Changed files

| File | Change |
|------|--------|
| `src/versinfo.rh2` | `VERSINFO_COPYRIGHT1/2` → `VERSINFO_COPYRIGHT_NEWT` / `_OPENSAL`, both self-contained; `VERSINFO_COPYRIGHT` untouched |
| `src/logo.cpp` | About: two `SetDlgItemText` calls in `WM_INITDIALOG`; splash: two `PaintText` calls swapped |
| `src/lang/lang.rc` | `IDD_ABOUT`: both copyright captions blanked, comment added above the dialog |
| `translations/*/salamand.slt` (11) | two copyright rows blanked |
| `CLAUDE.md` | copyright rule now points at `versinfo.rh2` |

## Observations outside this feature's scope

- In the dark theme the About dialog's web link (`IDC_ABOUT_WWW`) renders in a
  dark blue that is hard to read against the navy background. Pre-existing,
  unrelated to the copyright notice, not changed here.
- `CLAUDE.md` states 18 plugins ship by default; the build reports 19 enabled in
  `plugins.cfg`. Pre-existing documentation drift, not touched.
