# Phase 0 Research: Language Build Policy

**Feature**: 039-language-build-policy
**Date**: 2026-07-26

The spec left no NEEDS CLARIFICATION markers. This document records what was
verified in the existing code and the design decisions that follow from it, so
Phase 1 has a factual basis rather than assumptions.

---

## Verified facts

### V1 — The language chooser reads the directory, not a list

`CLanguageSelectorDialog::Initialize` (`src/dialogs2.cpp:928-964`) enumerates
`lang\*.slg` beside the executable with `FindFirstFile` and builds its list from
whatever it finds. Plugins do the same through `src/plugins1.cpp:1789`.

**Consequence**: FR-010 ("the chooser offers exactly the enabled languages")
requires no product code at all. It is a pure consequence of FR-003 — remove the
file, and the language stops being offered. This is why FR-003 is the load-bearing
requirement.

### V2 — A vanished saved language is already handled

`src/salamdr1.cpp:4013-4034`: when the remembered `.slg` fails to load or fails
`IsSLGFileValid`, and it was not chosen during this same run, the product shows
*"File … was not found or is not valid language file. Newt Commander will try to
search for some other language file (.SLG)"*, clears `Configuration.SLGName`, and
jumps back to `FIND_NEW_SLG_FILE` — the chooser.

**Consequence**: FR-011 needs no code either. The Assumptions entry in the spec
("no change to how the product loads or selects languages at runtime is needed")
is confirmed, not assumed. Verification for FR-011 is therefore a *run* check,
not a code change.

### V3 — The plugin policy already does exactly this, and does it early

`build.cmd:110-134` runs `gen_plugins_filter.ps1` **before MSBuild on every
build** — not only on full builds. That script validates `plugins.cfg`, writes
the solution filter, and reconciles the output: `gen_plugins_filter.ps1:112-122`
deletes any directory under `<out>\plugins\` that is not an enabled plugin,
printing `Reconcile: removed stale output <path>`, then prints
`Plugins: N enabled, M disabled`, which `build.cmd:128` parses back out to show
in the build banner.

**Consequence**: there is a precedent to copy line for line, including the
"every build, not just full" placement — which is exactly what SC-003 demands.

### V4 — Language *building* only happens on full builds

`build.cmd:299` calls `build_langs.cmd` from `:populate_runtime`, which runs only
when `BUILD_FULL=1`.

**Consequence**: reconciliation cannot live inside `build_langs.cmd`. If it did,
`build.cmd` (non-full) would leave a disabled language's modules in place and
disabling would appear to do nothing — the exact failure the spec's "Stale
output" edge case describes. Validation and reconciliation must sit in the early
policy stage (V3); only *production* stays in `build_langs`.

### V5 — Output lang directories contain nothing but `.slg`

`build/salamander/Release_x64/lang/` and each of the 19
`plugins/<name>/lang/` directories hold exactly 12 files: `english.slg` plus one
per language. `build_langs.ps1:304-312` already sweeps `*.bak` out of them.

**Consequence**: the reconciliation rule can be stated positively and safely —
*keep `english.slg` and one `.slg` per enabled language, delete every other
`.slg`*. That is stronger than deleting a known disabled list: it also cleans up
a language that was renamed or removed from the registry entirely, matching what
the plugin stage does for unknown directories.

### V6 — Three readers parse the registry, and one is a pure filter

- `src/vcxproj/read_languages.ps1` — build-side reader; emits `folder|langid|origin`
- `tools/translate/config.py::load_languages()` — Python-side reader
- consumers of the Python reader: `config.matrix()`, `config.main()`,
  `merge.main()` (`merge.py:402`), `rebrand.main()` (`rebrand.py:149`)

`build_langs.ps1:132-144` consumes the PowerShell reader's stdout and skips any
line without a `|`, so that reader can gain fields but must not lose its shape.

---

## Decisions

### D1 — The switch is a required `enabled = on|off` field in `languages.cfg`

**Decision**: add `enabled` to every `[language]` section as a **required**
field, values `on` or `off`, matching `plugins.cfg` vocabulary.

**Rationale**: the spec's Assumptions already settled *which file*. Making the
field required rather than defaulting to `on` mirrors `plugins.cfg`, where an
unlisted plugin is an error rather than a silent default (feature 007) — a policy
file whose entries can be silently absent cannot be trusted to describe what
shipped, which is the whole point of FR-007. Both readers already have a
`required` list to extend (`read_languages.ps1:36`, `config.py:108`), so the
error message for a missing field comes for free and already names the section.

**Alternatives considered**:
- *Optional, default `on`* — one less line to write per language, but reintroduces
  the "policy file fails silently" failure the P3 story exists to prevent.
- *A separate `disabled_languages =` list at the top of the file* — a second
  place naming languages, needing cross-validation against the sections. Same
  objection the spec already raised against a second file, in miniature.
- *`ship = yes|no`* — new vocabulary for an existing concept; `on|off` is what a
  maintainer already types in `plugins.cfg`.

### D2 — Validation and reconciliation go in a new `lang_policy.ps1`, called from the policy stage

**Decision**: a new `src/vcxproj/lang_policy.ps1`, invoked from `build.cmd`
immediately after the plugin policy stage, before MSBuild, on **every** build.
It validates the registry, reconciles every output `lang` directory, and prints
`Languages: N enabled, M disabled` for the banner to parse.

**Rationale**: V4 forces the placement, V3 supplies the shape. Keeping it
separate from `read_languages.ps1` keeps that script a pure reader — it is
consumed as data by `build_langs.ps1` (V6), and a reader that also deletes files
is a trap for the next caller.

**Alternatives considered**:
- *Add `-OutputRoot` to `read_languages.ps1`* — fewer files, but conflates a
  parser with a destructive action and puts `Reconcile: …` lines into a stream
  another script parses.
- *Reconcile inside `build_langs.ps1`* — rejected by V4.
- *Extend `gen_plugins_filter.ps1` to cover languages too* — unrelated concerns,
  and it would have to run after MSBuild for plugin lang dirs to exist. They
  don't need to: reconciliation is a delete-only pass, harmless on a tree that
  does not exist yet.

### D3 — `load_languages()` returns enabled languages by default

**Decision**: `Language` gains `enabled: bool`. `load_languages()` gains
`include_disabled: bool = False` and filters to enabled languages by default.
Registry validation (unique LANGIDs, registry ⟷ directory agreement) still runs
over **all** records regardless of the flag.

**Rationale**: FR-012 requires skipping to be the *default*. If the reader
returned everything and each caller had to remember to filter, the policy would
hold only as long as nobody adds a fourth caller — and V6 shows callers already
multiply. Making the safe behaviour the default means an omission fails safe.
Validating the whole file regardless is what keeps FR-007 working: a broken
record in a disabled section is still a broken record.

**Alternatives considered**:
- *`load_languages()` returns all; callers filter* — rejected above.
- *A separate `load_enabled_languages()`* — leaves the unsafe function as the
  obvious-looking one.

### D4 — Naming a disabled language explicitly is the opt-in

**Decision**: `--all` (and any unrestricted run) processes enabled languages
only. `--language <folder>` naming a disabled language processes it, after
printing a one-line notice that it is disabled. No new flag.

**Rationale**: this is what SC-008 states. A maintainer who types the language's
name has already expressed the intent FR-013 asks for; a second `--include-disabled`
flag on top of that is ceremony, and a flag that must accompany an explicit name
is the kind of thing people alias away. The notice keeps it from being silent.

**Alternatives considered**:
- *`--include-disabled` switch* — considered and rejected as above. Worth
  revisiting only if a future tool needs "all languages including disabled" in
  one run; `include_disabled=True` at the call site already covers that in code.

### D5 — `rebrand.py` opts in deliberately; `merge.py` does not

**Decision**: `merge.py` takes the default (enabled only, D4 for the opt-in).
`rebrand.py` calls `load_languages(include_disabled=True)` with a comment
explaining why.

**Rationale**: FR-012's stated purpose is that *effort and budget* are not spent
on a language that will not ship. `merge` spends DeepL characters, so it is
exactly the tool the requirement is about. `rebrand` neither translates nor
spends: it is a correctness sweep over committed source, and skipping disabled
languages would let brand residue accumulate in their `.slt` files and surface
as a failure the moment one is re-enabled — turning FR-005's "no re-translation
required" into a lie. Opting in explicitly at the call site *is* the FR-013
mechanism, exercised in code rather than on a command line.

**Alternatives considered**:
- *Apply the skip to `rebrand` too* — consistent-looking, but trades a real
  correctness property for a symmetry nobody benefits from.
- *Exempt `rebrand` from the policy silently* — same behaviour, no record of the
  decision; the next reader would "fix" it.

### D6 — `build_langs` refuses a disabled language rather than building it

**Decision**: `build_langs.cmd --language <disabled>` fails with
`ERROR: language '<x>' is disabled in languages.cfg`.

**Rationale**: `build_langs` is a build tool, not an authoring tool. FR-002 says
the build produces modules only for enabled languages, and the policy stage would
delete the result on the next build anyway. Failing loudly beats producing
something that silently vanishes. The authoring opt-in (D4) is the supported way
to work on a disabled language.

### D7 — `read_languages.ps1` emits a fourth field rather than filtering

**Decision**: the record becomes `folder|langid|origin|enabled` (`1`/`0`), and
the reader keeps emitting every registered language.

**Rationale**: reconciliation needs the *disabled* set as much as the enabled one,
and `build_langs.ps1` needs the enabled set — one reader serving both means one
place to be wrong. Appending a field is compatible with the existing consumer,
which splits on `|` and indexes positionally (V6).

---

## Out of scope, confirmed

- **The rendering defect** — the spec records it and explicitly does not fix it.
  Nothing in this plan touches font or charset handling.
- **Installer packaging** — `src/setup/` contains no per-language file list
  (verified during clarification), so nothing there can contradict the policy.
  Feature 038's task T047 still owns wiring language files into the installable
  product and will derive the set from this policy.
- **`english.slg`** — compiled from `.rc` sources by MSBuild, never produced by
  `build_langs`, and always kept by reconciliation.
