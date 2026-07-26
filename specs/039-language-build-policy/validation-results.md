# Validation Results: Language Build Policy

**Feature**: 039-language-build-policy
**Date**: 2026-07-26
**Build**: `Release_x64`, `OPENSAL_BUILD_DIR` unset → `E:\Projects\newtcommander\build\`
**Scope**: 20 modules (app + 19 enabled plugins) × 11 registered languages

Every criterion below was run. Observed evidence is recorded verbatim; nothing
is marked passed on inspection alone.

---

## Success criteria

### SC-001 — one file, one line

**PASS.** Every state change in this document was made by editing only
`translations/languages.cfg`. No build script, source file, or project file was
edited to switch a language on or off.

### SC-002 — 9 entries in the chooser, no disabled modules in the output

**PASS.**

```text
total .slg: 180        (was 240)
app lang\ : 9          (8 languages + english.slg)
per-lang-dir counts: 9 files × 20 dirs
disabled leftovers (russian/ukrainian/chinesesimplified anywhere): 0
```

In the running product, **Options ▸ Configuration ▸ Language ▸ `Language...`**
listed exactly 9 entries — Czech, Dutch, English, French, German, Hungarian,
Romanian, Slovak, Spanish — with no Russian, Simplified Chinese or Ukrainian.
Screenshot: `t013-language-chooser.png`.

### SC-003 — removal happens in the same build, no clean build

**PASS.** Starting from a tree containing all 12 languages, a plain
`build.cmd release` (**not** `full`):

```text
Reconcile: removed stale language module …\lang\chinesesimplified.slg
… (60 lines, 3 languages × 20 modules)
Reconcile: 60 stale language module(s) removed
Languages: 8 enabled, 3 disabled
           off: chinesesimplified, russian, ukrainian
 Lang policy   : 8 of 11 languages enabled (languages.cfg)
BUILD SUCCEEDED
after build : 180 .slg
```

A subsequent `build.cmd full release` did not reintroduce them (180, leftovers 0).

### SC-004 — re-enabling restores byte-identical content

**PASS, with a measured caveat that is not caused by the policy.**

*Isolated round trip* (disable → reconcile → re-enable → `build_langs`, no
MSBuild in between, so `english.slg` could not change):

```text
after disable   : 180 .slg
after re-enable : 240 .slg
identical: 240 / 240    differing: 0
```

*The caveat.* An earlier round trip that **did** include a full `build.cmd`
showed 216/240 identical. All 24 differing files were confined to two modules,
`mdview` and `sftp`, and within those two **every one of the 12 languages
differed — including `english.slg` itself**, which `build_langs` never produces
and which was never disabled:

| | |
|---|---|
| differing by module | mdview 12, sftp 12 |
| differing by language | 2 files each, for all 12 languages |
| `english.slg` among them | yes, 2 |
| restored-language files differing | 6 of 60 — exactly those in mdview and sftp |

MSBuild relinked `lang_mdview` and `lang_sftp` during that run (their vcpkg
`applocal.ps1` step re-ran), giving each a new `english.slg` with a different PE
`TimeDateStamp`. Every language module is seeded from `english.slg`, so all 12
derived modules moved with it. This is pre-existing link non-determinism,
affects enabled and disabled languages identically, and is unrelated to
disabling. The isolated round trip above is the measurement of the property
SC-004 actually asserts.

An unplanned third data point: a full rebuild of 220 modules mid-session
reproduced **240/240** byte-identical output against the original baseline.

### SC-005 — every policy error names the offending entry

**PASS.** Each case run through `lang_policy.ps1` against a sandbox registry,
with `-OutputRoot` pointed at the real build tree to confirm nothing is deleted
on failure:

| Case | Exit | Output tree | Message |
|---|---|---|---|
| `enabled = maybe` | 1 | 180 → 180 | `language [russian] has enabled 'maybe' (expected on or off)` (one per affected section) |
| `enabled` line deleted | 1 | 180 → 180 | `language [russian] is missing: enabled` |
| section renamed `[russain]` | 1 | 180 → 180 | `no translations\russain\ directory …` **and** `translations\russian\ exists but is not registered …` |
| duplicate LANGID | 1 | 180 → 180 | `LANGID 1049 used by both [russian] and [slovak] -- must be unique` |
| unregistered directory | 1 | 180 → 180 | `translations\klingon\ exists but is not registered in languages.cfg` |

**End-to-end, the build stops before MSBuild.** With `enabled = yes` in the real
registry, `build.cmd release` exited 1 after printing all 8 errors and
`Language policy check FAILED. Fix translations\languages.cfg and try again.`
The configuration banner was never printed and no MSBuild output appeared
(both checked programmatically: `False`, `False`).

### SC-006 — all languages disabled produces a working English-only product

**PASS.**

```text
Reconcile: 160 stale language module(s) removed
Languages: 0 enabled, 11 disabled
 Lang policy   : 0 of 11 languages enabled (languages.cfg)
  language modules built: 0
  languages skipped (off): 11  -- czech, dutch, french, german, hungarian,
                                  chinesesimplified, romanian, russian, slovak,
                                  spanish, ukrainian
BUILD SUCCEEDED
lang dirs: 20   total .slg: 20   distinct names: english.slg
```

The product launched and ran in English — menu bar `Left  Files  Edit  Commands
Plugins  Options  Right  Help`, against `Levý  Soubory  Upravit  Příkazy
Pluginy  Možnosti  Pravý  Nápověda` in the same build with Czech present.
Screenshot: `t026-english-only.png`.

### SC-007 — the build states what it built and what it skipped

**PASS.** Banner line ` Lang policy   : 8 of 11 languages enabled (languages.cfg)`
plus, from `build_langs`, `  languages skipped (off): 3  -- chinesesimplified,
russian, ukrainian` and, from the policy stage, `           off:
chinesesimplified, russian, ukrainian`.

### SC-008 — tooling skips disabled languages, naming one opts in

**PASS.**

| Command | Result |
|---|---|
| `translate.merge --all --dry-run` | coverage table lists exactly 8 languages: czech, german, french, dutch, hungarian, romanian, slovak, spanish |
| `translate.merge --language ukrainian --dry-run` | `note: 'ukrainian' is disabled in languages.cfg -- processing it because you named it explicitly`, then processes it (7,637 entries) |
| `translate.merge --language klingon --dry-run` | `error: unknown language 'klingon'` |
| `translate.rebrand` | scanned **84,007** entries = 11 × 7,637, i.e. all languages including the three disabled — the deliberate exception (research.md D5) |

---

## Functional requirements not covered by a success criterion

### FR-004 — translation source untouched

**PASS.** After all the disable/enable cycles above,
`git status --short translations/` reported exactly one modified file:
`translations/languages.cfg`. None of the 290 `.slt` files or their `.origin`
sidecars changed.

### FR-010 — the chooser offers exactly the enabled languages

**PASS.** See SC-002. No product code was changed: the chooser enumerates
`lang\*.slg` from disk (`src/dialogs2.cpp:928`), so removing the file removes
the language.

### FR-011 — a user whose language disappeared is guided, not failed

**PASS.** With the product configured for Russian and Russian disabled, startup
showed:

> File `E:\…\Release_x64\lang\russian.slg` was not found or is not valid
> language file. Newt Commander will try to search for some other language
> file (.SLG).

and then recovered automatically to Czech, matching the Windows display
language. Screenshot: `t014-missing-language.png`. No crash, no silent English
fallback. (The chooser opens instead when no shipped language matches the user's
locale — `src/salamdr1.cpp:3974-4001`.) No product code was changed.

### FR-013 opt-in via `build_langs`

**Refused by design, as specified.**

```text
> build_langs.cmd release --language ukrainian
ERROR: language 'ukrainian' is disabled in languages.cfg -- enable it there to build it   (exit 1)

> build_langs.cmd release --language klingon
ERROR: language 'klingon' is not registered in languages.cfg                              (exit 1)
```

---

## Defect found and fixed during validation

**Cascading validation errors.** A section failing field validation was dropped
from the parsed registry, after which the V3 check reported its
`translations/<folder>/` directory as *unregistered* — a second error pointing
at a problem that did not exist. Deleting one `enabled` line produced six error
lines instead of one. Making `enabled` a required field made this path easy to
reach, so `read_languages.ps1` now tests V3 against every section encountered
rather than only the ones that validated. After the fix, deleting the `enabled`
line from `[russian]` reports exactly:

```text
ERROR: language [russian] is missing: enabled
```

---

## Not run

Nothing. All eight success criteria and the four requirements above were
executed against a real build and, where applicable, the running product.

Out of scope by decision, and therefore not attempted:

- **The menu rendering defect** in the three non-Latin-script languages. It is
  recorded in `spec.md` and is not diagnosed or fixed by this feature.
- **Installer packaging.** `src/setup/` carries no per-language file list, so
  nothing there contradicts the policy; wiring language files into the
  installable product remains feature 038's open task T047.
