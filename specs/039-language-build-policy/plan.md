# Implementation Plan: Language Build Policy

**Branch**: `039-language-build-policy` | **Date**: 2026-07-26 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/039-language-build-policy/spec.md`

## Summary

Add an `enabled = on|off` field to each language record in
`translations/languages.cfg`, and make the build honour it: produce language
modules only for enabled languages, and delete from the output tree any `.slg`
that does not belong to one. Disable Simplified Chinese, Russian and Ukrainian,
whose menus render incorrectly, without touching their committed translation
source.

The approach copies the plugin build policy (feature 007) rather than inventing a
mechanism. Two findings shape it:

- **The product needs no code change.** The language chooser enumerates
  `lang\*.slg` from disk (`src/dialogs2.cpp:928`), and the "remembered `.slg`
  disappeared" path already recovers by re-running the chooser
  (`src/salamdr1.cpp:4017`). Removing the file *is* removing the language.
  FR-010 and FR-011 are verification tasks, not implementation tasks.
- **Reconciliation must run on every build, not only full ones.** Language
  modules are produced from `:populate_runtime`, which only runs on `build.cmd
  full`. If removal lived there, a plain `build.cmd` after disabling a language
  would leave its modules in place and disabling would appear to do nothing. So
  validation + removal go in the early policy stage alongside `plugins.cfg`, and
  only production stays in `build_langs`.

Everything else follows: one new script, three extended scripts, one Python
reader change, one config file change. No C++ is touched.

## Technical Context

**Language/Version**: Windows Batch (`build.cmd`), Windows PowerShell 5.1 (build scripts), Python 3.13 (offline authoring tools). No C++ change.
**Primary Dependencies**: MSBuild / VS2022 v143, `translator.exe` (both unchanged and unaffected)
**Storage**: `translations/languages.cfg` — UTF-8 without BOM, INI-style; the sole policy record
**Testing**: the build is the harness — policy errors must fail it before MSBuild; plus scripted output assertions and in-product run checks (see quickstart.md)
**Target Platform**: Windows 11 x64
**Project Type**: desktop application build pipeline + offline developer tooling
**Performance Goals**: the policy stage adds a directory scan over 20 `lang` directories — must not be measurable against a build measured in minutes
**Constraints**: no absolute paths in scripts (`OPENSAL_BUILD_DIR`); must take effect without a clean build; must not modify anything under `translations/`
**Scale/Scope**: 11 registered languages × 20 enabled modules = up to 220 language modules; 8 enabled → 160 after this feature

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design.*

| Principle | Assessment | Verdict |
|---|---|---|
| I. Build Reproducibility | Strengthened. Which languages ship becomes a committed policy file read by the build, replacing "delete files by hand after every build" — which was the only existing way and is by definition not reproducible. Still one command, no manual steps, output still under `OPENSAL_BUILD_DIR`. | ✅ PASS |
| II. Backward Compatibility | Baseline is Newt Commander 0.1.0. Three languages stop shipping — a deliberate, documented change to stop users seeing broken text, with the defect recorded in spec.md. Users on those languages hit the product's existing recovery path (`salamdr1.cpp:4017`), not a failure. No plugin API or ABI change. No registry change. | ✅ PASS |
| III. Incremental Modernization | Small, additive, revertible: one required field, one new script, three extended. No adjacent refactoring. New comments in English. | ✅ PASS |
| IV. Windows Platform Commitment | Batch + PowerShell 5.1 + existing Python tooling. No new dependencies, no cross-platform layer, no licence impact. | ✅ PASS |
| V. Plugin Architecture Preservation | Plugin lang directories are reconciled the same as the app's, so the policy holds uniformly. No plugin interface touched. | ✅ PASS |
| VI. UI Consistency | No dialog, control, or resource template is modified. The chooser's contents change because its input directory changes. | ✅ PASS (N/A) |

**Result**: no violations, before or after design. Complexity Tracking omitted —
nothing to justify.

## Project Structure

### Documentation (this feature)

```text
specs/039-language-build-policy/
├── spec.md                       # /speckit.specify + /speckit.clarify output
├── plan.md                       # This file
├── research.md                   # Phase 0 — verified facts V1..V6, decisions D1..D7
├── data-model.md                 # Phase 1 — the policy record and its validation
├── quickstart.md                 # Phase 1 — change it, verify it, work on a disabled language
├── contracts/
│   ├── languages-cfg.md          #   the maintainer-facing file format
│   ├── build-scripts.md          #   lang_policy.ps1, read_languages.ps1, build_langs.ps1, build.cmd
│   └── translate-tooling.md      #   load_languages() default + the FR-013 opt-in
├── checklists/
│   └── requirements.md           # spec quality checklist (16/16)
└── tasks.md                      # Phase 2 output (/speckit.tasks — NOT created here)
```

### Source Code (repository root)

```text
translations/
└── languages.cfg                 # MODIFIED  + "enabled = on|off" in all 11 sections;
                                  #             3 set off (FR-006)

src/vcxproj/
├── lang_policy.ps1               # NEW       validate + reconcile output + report counts
├── read_languages.ps1            # MODIFIED  require/validate "enabled";
                                  #             emit folder|langid|origin|enabled
└── build_langs.ps1               # MODIFIED  build enabled only; refuse a disabled
                                  #             -Language; report skipped count

build.cmd                         # MODIFIED  language policy stage after the plugin
                                  #             stage; banner line

tools/translate/
├── config.py                     # MODIFIED  Language.enabled;
                                  #             load_languages(include_disabled=False);
                                  #             matrix()/main() show state
├── merge.py                      # MODIFIED  enabled only; explicit --language <disabled>
                                  #             opts in, with a notice
├── rebrand.py                    # MODIFIED  include_disabled=True, with the reason
└── README.md                     # MODIFIED  document the policy and the opt-in

architecture/03-build-pipeline.md # MODIFIED  the policy stage now covers languages too
CLAUDE.md                         # MODIFIED  language build policy alongside plugins.cfg
```

**Structure Decision**: no new source tree. This is a build-pipeline feature, so
it lives where the existing build policy lives — `build.cmd` plus
`src/vcxproj/*.ps1` — with the policy data beside the translations it governs.
The one new file, `lang_policy.ps1`, is deliberately a sibling of
`gen_plugins_filter.ps1` and does the same three things for languages that that
script does for plugins: validate, reconcile output, report counts.

## Implementation order

Ordered so each step is independently verifiable and the P1 story lands first.

1. **Policy data** — add `enabled` to all 11 sections; set 3 to `off`.
   Immediately breaks both readers (required field), which is the point: the
   validation work is now forced rather than optional.
2. **Readers** — `read_languages.ps1` and `config.py` accept and validate the
   field. Build parses again; nothing yet honours the value.
3. **Reconciliation** — `lang_policy.ps1` + the `build.cmd` policy stage.
   **This is the MVP**: at this point disabling a language actually removes it,
   on any build. (US1 / SC-002 / SC-003)
4. **Production** — `build_langs.ps1` builds enabled languages only and refuses a
   disabled `-Language`. Without step 3 this alone would be insufficient; with
   it, it avoids the pointless build-then-delete cycle.
5. **Validation messages** — the FR-007 error set, verified one by one. (US3)
6. **Tooling** — `config.load_languages` default, `merge` opt-in, `rebrand`
   include, printer state column. (FR-012 / FR-013 / SC-008)
7. **Verification** — re-enable round trip with hashes (US2 / SC-004), all-off
   build (SC-006), in-product chooser and stranded-user checks (FR-010 / FR-011).
8. **Docs** — quickstart is written; update `tools/translate/README.md`,
   `architecture/03-build-pipeline.md`, `CLAUDE.md`.

## Risks

| Risk | Mitigation |
|---|---|
| Reconciliation deletes something it should not | The rule is positive — keep `english.slg` + enabled languages, delete other `*.slg` — and scoped to `lang` directories only. Verified: those directories contain nothing but `.slg` (research.md V5). `english.slg` is never produced by `build_langs` and always kept. |
| Making `enabled` required breaks an unmigrated config | Intentional and immediate: the failure names the section and the missing field. Migration is 11 lines, done in step 1 of the same change. |
| The 4th field breaks `build_langs.ps1`'s record parsing | It splits on `|` and indexes positionally; appending is compatible. Covered by a task that runs the build after step 2 and before step 3. |
| A disabled language's `.slt` rots and re-enabling fails later | `rebrand` deliberately keeps sweeping disabled languages (research.md D5), and re-enabling is verified by hash round-trip (SC-004) rather than assumed. |
| Someone "fixes" the asymmetry between `merge` and `rebrand` | Recorded as decision D5 with its rationale, and restated in `contracts/translate-tooling.md` and in the `rebrand` call-site comment. |
