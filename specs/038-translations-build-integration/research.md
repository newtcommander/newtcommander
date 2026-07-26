# Phase 0 Research: Translations Build Integration

**Feature**: 038-translations-build-integration
**Date**: 2026-07-26

All findings below were established by reading the actual sources in this
repository (paths and line numbers cited), not from documentation or memory.
The `.slg` production pipeline turned out to be the decisive question, and it
has a non-obvious hard constraint (R2) that shapes the whole design.

---

## R1: How is a translated `.slg` produced?

**Decision**: reuse the existing **Translator** tool (`src/translator/`,
`translator.vcxproj`, already in `salamand.sln`) in its **quiet (headless) command-line
modes**, driven by a build script.

**How it works** — `CData::Save()` (`src/translator/trldata.cpp:317`):

1. `CopyFile(FullTargetFile, *.tmp)` — the target `.slg` **must already exist**
2. `BeginUpdateResource(tmp)`
3. `SaveStrings` / `SaveMenus` / `SaveDialogs` → `UpdateResource(RT_STRING | RT_MENU | RT_DIALOG)`
   (`datastr.cpp:143`, `datamenu.cpp:356`, `datadlg.cpp:2079`)
4. `SaveSLGSignature` + `SaveModuleName` + `VersionInfo.UpdateResource(VS_VERSION_INFO)`
5. `EndUpdateResource` → rotate `.tmp` over the target, keep `.bak`

This is exactly the transformation the feature needs, already written and
long-proven.

**Quiet modes** (`wndframe.cpp:562` `ProcessCmdLineParams`, dispatched at `wndframe.cpp:468-535`):

| Invocation | Effect |
|---|---|
| `translator.exe -quiet-export-slt <dir> <project.atp>` | writes `<dir>\<module>.slt` from the project's *translated* module |
| `translator.exe -quiet-import-slt <dir> <project.atp>` | imports `<dir>\<module>.slt`, then `Data.Save()` + `Data.SaveProject()` |
| `translator.exe -quiet-validate-layout <project.atp>` | layout validation only (`QuietValidate=2`) |
| `translator.exe -quiet-validate-all <project.atp>` | full validation (`QuietValidate=1`) |
| `translator.exe -quiet-export-sizes <dir> <project.atp>` | writes `<module>.sdc` — dialog/control geometry |

**Alternatives rejected**:

- *New C++ resource patcher* — would duplicate the `DLGTEMPLATEEX` / menu /
  string-table serialization already implemented in `datadlg.cpp` (2124 lines),
  `datamenu.cpp`, `datastr.cpp`. Pure risk, no gain. Violates Constitution III.
- *Generate a per-language `.rc` and compile it* — requires a full RC parser
  (macros, `#include`, `DIALOGEX`, control statements). Far harder than it looks,
  and `src/lang/lang.rc` is 2400+ lines of hand-maintained resource script.
- *Python + `ctypes` `BeginUpdateResourceW`* — the Win32 calls are trivial from
  Python, but building the binary `RT_DIALOG` / `RT_MENU` / `RT_STRING` blobs is
  not. Same duplication problem as the C++ option.

---

## R2: `.slt` import is **strictly positional** — the load-bearing constraint

**Finding**: `CData::ImportTextArchive` (`trldata.cpp:2301`) does **not** look
entries up by ID. It walks `DlgData`, `MenuData`, and `StrData` in array order
and requires the file to contain exactly the matching sections and rows, in
exactly the same order:

```cpp
for (i = 0; i < DlgData.Count; i++) {
    ret &= ITACheckSection(buff, lineNumber, L"DIALOG", dialog->ID);
    for (int j = 0; j < dialog->Controls.Count; j++) { ... }   // trldata.cpp:2474-2503
}
```

Any mismatch sets `ret = FALSE` → *"Syntax error in file X on line N"* → the
**entire import is rejected** (`trldata.cpp:2594`). The caller runs a `testOnly`
pass first (`wndframe.cpp:516`) and skips the import outright if it fails.

**Consequence — this is the reason the feature needs a migration step**: the
repository's `.slt` files were exported from **Salamander 4.0 build 180**
(`[EXPORTINFO] VERSION,"4,0,0,180"` in every file) and describe that product's
resource layout. They **cannot** be imported into today's `english.slg` as-is.

**Design implication**: the pipeline must be
*export canonical English template → merge legacy translations into it by ID →
import the merged result*, never *import legacy directly*.

---

## R3: `[EXPORTINFO]` is skipped on import — version drift is not itself fatal

`ImportTextArchive` reads the `[EXPORTINFO]` header and **discards all four
lines** (`trldata.cpp:2404-2412`, four `GetUTF8TextLine(...) // skip` calls).
`PROJECTNAME`, `TEXTVERSION`, and `VERSION` are never validated.

So the 4.0-vs-0.1.0 version gap is *not* a blocker by itself — only the
structural mismatch (R2) is. The merge tool can therefore emit a template-shaped
`.slt` carrying legacy translated text and it will import cleanly.

---

## R4: No modal dialog fires when importing into a fresh copy of `english.slg`

The one interactive prompt on the quiet-import path is guarded by
`SLGSignature.IsSLTDataChanged()` (`trldata.cpp:2360`).

`IsSLTDataChanged()` (`trldata.h:608`) returns `FALSE` when `CRCofImpSLT ==
"none"`, and `"none"` is exactly what `english.slg` carries — set in
`src/plugins/shared/versinfo.rc2:63`:

```rc
VALUE "SLGCRCofImpSLT", "none\0"
```

**Therefore**: seeding each target `<language>.slg` as a byte copy of the freshly
built `english.slg` both satisfies R1's precondition (target must exist) and
guarantees the headless import never blocks on a message box.

---

## R5: Version lock is automatic

`IsSLGFileValid` (`salamdr2.cpp:3005`) rejects any `.slg` whose
`VS_FIXEDFILEINFO.dwFileVersionMS/LS` differs from its owning module's. Because
the target `.slg` starts as a **copy of `english.slg`** — which was compiled in
the same build from the same `versinfo.rh2` — `FILEVERSION` matches by
construction. Translator only rewrites the *string* values (`SLGAuthor`,
`SLGWeb`, `SLGComment`, `SLGHelpDir`, `SLGIncomplete`, `SLGCRCofImpSLT`) and
`VarFileInfo\Translation` (the LANGID).

FR-007 (version lock) and FR-026 (fail the build on a rejected module) are
therefore cheap: copy-then-patch cannot drift, and a post-build verifier only
needs to re-read `VS_FIXEDFILEINFO` from each produced `.slg`.

---

## R6: Success is signalled by **exit code 1**

Both quiet paths end with `DestroyWindow(HWindow); ExitProcess(1);` on success —
`wndframe.cpp:365`, `:461`, `:532`. The comment is explicit: *"exit code 1 means
validation succeeded"*.

**Build-script rules**:

- treat `ERRORLEVEL == 1` as **success**, anything else as failure;
- Translator is a GUI app whose error paths call `MessageBox`, which would hang
  a build. Every invocation runs under a **timeout guard**; a timeout is a build
  failure with a clear message, not a hang.

---

## R7: `.atp` project files must be generated

The Translator projects lived outside the repo, under
`%OPENSAL_BUILD_DIR%salamander\translator\Salamand 4.0\projects\<lang>\<module>.atp`
(per `translations/!update_langs_from_translator.bat`, itself marked *"outdated =
needs to be fixed before using again"*).

Format is plain INI-ish text, written by `CData::SaveProject`
(`dataprj.cpp:820`) and parsed by `LoadProject` (`dataprj.cpp:599`):

```ini
[Files]
Original=<path to english.slg>
Translated=<path to <language>.slg>
Include=<path to lang.rh>

[Settings]
ExpandStrings=0
...
```

Mandatory at load time (`OpenProject`, `wndframe.cpp:344`):
`DataRH.Load(FullIncludeFile)` must succeed, so **`Include=` is required** —
`src/lang/lang.rh` for the app, `src/plugins/<name>/lang/lang.rh` for plugins
(verified present for all 19 enabled plugins). `SalMenu`, `IgnoreList`,
`CheckList`, `SalamanderExe` are optional and omitted.

The export filename is derived from the `.atp` basename
(`wndframe.cpp:489-493`), so projects must be named `<module>.atp`.

Generated `.atp` files are build intermediates → generated into
`$(OPENSAL_BUILD_DIR)`, gitignored, never committed.

---

## R8: Layout validation is already available — SC-005 gets a real gate

`-quiet-validate-layout` runs `Data.ValidateTranslation()` and exits 1 only when
`OutWindow.GetErrorLines() == 0` (`wndframe.cpp:357-366`). That is a
machine-checkable clipped/overlapping-control gate for every (language, module)
pair, at zero implementation cost.

`-quiet-export-sizes` additionally dumps dialog and control geometry (`.sdc`),
usable by the merge tool for width-fitting decisions.

---

## R9: Machine translation backend

**Decision**: Python tool under `tools/translate/`, using the official
**`anthropic` SDK** with **`claude-opus-5`**, run **offline** and its output
committed (FR-023). Never invoked by `build.cmd`.

Rationale and mechanics:

- **Model**: `claude-opus-5` — $5 / $25 per MTok, 1M context. Translation
  quality across Czech/Russian/Ukrainian/Chinese matters more here than unit
  cost, and the total spend is small (below).
- **Message Batches API** (`client.messages.batches.create`) — 50% off all token
  usage, up to 100k requests per batch, results within ~1h. Bulk string
  translation is the textbook batch workload: no latency requirement, tens of
  thousands of independent units. **Results arrive in arbitrary order — key by
  `custom_id`, never by position.**
- **Structured outputs** — each request returns a validated object rather than
  free text, so the merge step never has to parse prose out of a reply.
- **Prompt caching** on the shared prefix (glossary, style rules, accelerator/
  placeholder rules, target-language instructions). Opus 5's minimum cacheable
  prefix is 512 tokens, comfortably met.
- **Thinking is on by default on Opus 5** — `max_tokens` caps thinking *plus*
  response text, so per-request `max_tokens` is sized with headroom.

**Volume and cost estimate** (measured from the sources):

| Quantity | Count |
|---|---|
| English strings across the 20 enabled modules | ~3,600 |
| Dialogs / menus | 244 / 36 |
| Translation units per language (strings + controls + menu items) | ~6,500 |
| Existing human-translated rows per language (all 10 identical) | 8,061 |

Work to machine-translate: Ukrainian in full (~6,500 units), the four
untranslated plugins × 10 languages (~2,500), plus post-4.0 drift across the
legacy modules. Order of magnitude **20k–50k units**, ≈5M input / 3M output
tokens. At batch rates ($2.50 / $12.50 per MTok) that is **roughly $50 for a
complete regeneration** — cheap enough that quality, not cost, drives the model
choice.

**Determinism** (FR-024): guaranteed by committing the merged `.slt` files, not
by the model. The build consumes committed text; it never calls the API.

---

## R10: Ukrainian identity

| Field | Value |
|---|---|
| LANGID | **1058** (0x0422, uk-UA) — `[TRANSLATION] LANGID,1058` |
| Folder | `translations/ukrainian/` |
| `AUTHOR` | Newt Commander Project (machine translation) |
| `WEB` | `www.newtcommander.org` |
| `HELPDIR` | `ENGLISH` (no Ukrainian help files) |
| `SLGINCOMPLETE` | project URL — flags it as not human-reviewed (FR-016) |

Existing LANGIDs confirmed from the `.slt` headers: Czech 1029, German 1031,
Russian 1049.

---

## R11: Preserving accelerators, placeholders, and shortcut labels (FR-012)

Legacy translated text shows the exact markers that must survive translation:

- accelerator `&` — `"&Kopírovat\tCtrl+C"`
- shortcut label after `\t` — `"\tCtrl+C"`, `"\tAlt+F4"`
- `printf` placeholders — `%s`, `%d`, `%u`, `%c`, `%x`, `%ld`, and positional forms
- escape sequences `\n`, `\r`, `\t`, `\\`, `\"`

**Decision**: the merge tool validates every machine-produced string against its
English source — multiset of placeholders must match exactly, `\t`-suffix must be
carried verbatim (never translated), and `&` count must match. Violations are
retried with a corrective prompt; persistent failures fall back to the English
source and are reported. Per-dialog and per-menu accelerator-uniqueness is
checked after assembly (SC-006).

---

## R12: Build integration

**Decision**: a new script invoked by `build.cmd` after the language projects
link, iterating (enabled module × shipped language):

1. seed `<OutDir>\lang\<language>.slg` ← copy of `english.slg` (R4)
2. generate `<intermediate>\<language>\<module>.atp` (R7)
3. `translator.exe -quiet-import-slt <sltdir> <module>.atp`, timeout-guarded, expect exit 1 (R6)
4. verify `VS_FIXEDFILEINFO` matches the owning module (R5, FR-026)
5. accumulate the coverage report (FR-015)

Cost: 19 plugins + 1 app = 20 modules × 11 non-English languages = **220 imports**
per full build. Each is a short-lived process; incremental builds skip pairs
whose `.slt` and `english.slg` are both older than the existing `<language>.slg`.

`build.cmd` already counts `.slg` outputs and prints `language modules built: N
(english)` (`build.cmd:296-303`) — that line becomes the natural place to report
per-language results.

---

## R13: Where the residual risk sits

Two requirements cannot be fully automated and will need a human pass:

- **FR-013 (dialog layout)** — the `.slt` row format carries per-control
  geometry (`ctrlID,x,y,cx,cy,state,"text"`), so the merge tool *can* widen
  controls. But growing one control can collide with its neighbour, and the
  correct fix is often to re-flow the dialog. Mitigation: constrain machine
  translations to a length budget derived from the English string, then use
  R8's layout validator as the gate and hand-fix the residue.
- **FR-021 (declined product names)** — Czech, Russian, Ukrainian, and Polish-style
  inflection means a mechanical `Open Salamander` → `Newt Commander` substitution
  inside a sentence can produce the wrong case. Mitigation: substitute
  mechanically, then run the affected strings through the same translation pass
  with the corrected name in context, and diff.

Both are flagged in the checklist as the items most likely to need manual work.

---

## Summary of decisions

| # | Decision |
|---|---|
| R1 | Produce `.slg` via the existing Translator in quiet mode — no new resource writer |
| R2 | Import is positional → a template-based migration step is **mandatory** |
| R3 | `[EXPORTINFO]` is ignored on import → version drift alone is not a blocker |
| R4 | Seed each target `.slg` from `english.slg` → satisfies the copy precondition *and* suppresses the only modal prompt |
| R5 | Version lock is automatic by construction; verify post-build |
| R6 | Exit code **1** = success; every invocation is timeout-guarded |
| R7 | Generate `.atp` projects into the build dir (gitignored); `Include=` is mandatory |
| R8 | `-quiet-validate-layout` provides the SC-005 gate for free |
| R9 | Machine translation: Python + `anthropic` SDK, `claude-opus-5`, Batches API, offline, output committed |
| R10 | Ukrainian = LANGID 1058, `translations/ukrainian/` |
| R11 | Validate placeholders / accelerators / `\t`-shortcuts on every machine-produced string |
| R12 | 220 imports per full build, incremental by timestamp, reported through `build.cmd` |
| R13 | Dialog re-layout and declined product names are the two human-in-the-loop residues |

---

# Addendum: findings from implementing Phase 2 (2026-07-26)

The pipeline was built and run against a real `Release_x64` build. It works, but
getting there uncovered **four blockers that no amount of source reading would
have revealed** — each one surfaced only as a modal dialog or a crash. All four
are now fixed. They are recorded here because three of them are *source*
defects, not tooling gaps, and they explain why SFTP and MDView have never been
translatable.

Diagnosis technique worth reusing: `translator.exe` reports everything through a
GUI, so failures were read by attaching to the process and enumerating its
windows (`GetWindowTextA` — it is an ANSI app, so the wide API returns garbage),
and for the Output pane by reading its `SysListView32` cross-process with
`LVM_GETITEMTEXTW` + `ReadProcessMemory`. That turned "it hangs" into an exact
error string in one step.

## A1: `translator.exe` is not built by a normal build

In `salamand.sln` the translator project has an `ActiveCfg` for `Debug|x64` and
`Release|x64` but **no `.Build.0` entry** — it only builds under the
`Utils (Release)` solution configuration. `build.cmd` therefore never produces
it.

**Resolved**: `build_langs.ps1` builds it on demand from
`translator.vcxproj` (`Release|Win32`) the first time it is needed. The solution
is left untouched, and the cost is paid once.

## A2: `lang.rh` UTF-8 BOM breaks the symbol parser

29 of the 31 `lang.rh` files start with a UTF-8 BOM, because UTF-8-BOM is this
repository's source encoding standard (CLAUDE.md). The Translator's
resource-symbol parser predates that convention, reads the file as ANSI, and
fails with `Syntax error on line 1` → modal dialog → hung build.

**Resolved** in `gen_atp.ps1`: `Include=` points at a BOM-stripped copy
generated into the build tree. No source file changes, repo standard intact.

## A3: `IDC_STATIC` (-1) makes a dialog untranslatable — the real reason SFTP and MDView were never localized

`CDialogData` stores control IDs as `WORD`. `datadlg.cpp:1050` rejects any
`DIALOGEX` control whose 32-bit ID has a non-zero high word with
`32-bit IDs are not supported`. `IDC_STATIC` is `(-1)` = `0xFFFFFFFF`, so **any
dialog using it cannot be exported at all**.

This is why every other translatable module reserves a 40-ID block
(`#define IDC_STATIC_1 <base>` + `#include "statics.rh2"`) and uses
`IDC_STATIC_1..40` — a convention that is enforced nowhere and documented
nowhere, so the two newest plugins did not follow it.

**Resolved** (18 controls):

* `src/plugins/sftp/lang/lang.rc2` — 17 `IDC_STATIC` → `IDC_STATIC_1..7`
  (IDD_CHMOD) and `IDC_STATIC_1..10` (IDD_CONFIG); sftp already reserved 3000–3039.
* `src/plugins/mdview/mdview.rh2` — reserved 3000–3039 and included
  `statics.rh2`; `lang.rc2` 1 use → `IDC_STATIC_1`.

IDs need only be unique **within** a dialog, so the counter restarts per dialog.

## A4: two more SFTP-only defects behind that one

* **`#define IDC_STATIC (-1)` in `sftp/lang/lang.rh`** — the `.atp`'s `Include=`
  file is the Translator's symbol source, and it rejects a parenthesized
  negative value: `SYMBOLS file: invalid ID (IDC_STATIC) on row 7`. One error
  line is enough to skip the export entirely. Removed (it was unused after A3).
  Note the `Unknown identifier: <3000>` lines that remain are *warnings*; zip
  produces them too and exports fine.

* **`ICON IDC_WARNINGICON, ...` crashed the exporter** (`0xC0000005`) partway
  through `IDD_HOSTKEY`. Naming a resource *ordinal* as an ICON control's title
  is what does it; the application's own two ICON controls use `ICON ""` and set
  the image at run time. Changed to match, with `STM_SETICON` in `HostKeyProc`.
  This also fixes a latent runtime bug: `warning.ico` is a resource of
  `sftp.spl`, while the dialog template lives in the `.slg`, so the template-level
  ordinal could not have resolved against the right module anyway.

## A5: `--export-templates` was not idempotent (found by re-running it)

A full re-run failed **0/20**, every module timing out, even though a
single-module run had just succeeded. Cause: `ExportAsTextArchive` refuses to
overwrite silently. Its decision tree treats a copy of `english.slg` as an
"English build", and for that case *"SLT exists -> ask (overwriting unknown
file)"* — a modal dialog, on every module. The single-module test had passed only
because it deleted its `.slt` first.

**Resolved**: the export mode now deletes the previous `<module>.slt` before
invoking the Translator. Re-verified with all 20 templates already on disk:
**20/20, exit 0**.

The *import* path — the one the build actually uses — was never affected. It
writes the `.slg` through `CData::Save()` (`CopyFile` + `BeginUpdateResource` +
`.bak` rotation, no prompt), and its one prompt is guarded by
`IsSLTDataChanged()`, which stays false because every target is re-seeded from
`english.slg` (`SLGCRCofImpSLT="none"`) before each import. Idempotent by
construction.

## Verified state

* `build_langs.cmd --export-templates` → **20/20 modules**, exit 0, re-run clean
  with templates already present.
* All 20 generated templates parse and re-serialize **byte-exact** through
  `tools/translate/slt.py` (7,637 entries per language).
* `slt.py --verify` → **230/230 committed legacy files** byte-exact, 78,980 entries.
* `verify_slg.ps1` → 20/20 `english.slg` match their owning binary's FILEVERSION.

## Consequence for the plan

US3 gets **easier**: its hardest hidden prerequisite (making SFTP and MDView
structurally translatable at all) is already done. What remains there is
translation content, not resource surgery.

One new item for Phase 8, though: A3 is a project-wide convention with no
enforcement. A dialog added tomorrow with `IDC_STATIC` silently breaks its
module's translation. Worth a build-time check.


---

# Addendum 2: DeepL as the translation backend (2026-07-26)

R9 planned an Anthropic-based translator. The user supplied a **DeepL free-tier
key** instead (500,000 characters/month), so `tools/translate/deepl.py` targets
DeepL. For resource strings this turned out to be the better fit anyway:
`tag_handling=xml` + `ignore_tags` protects placeholders *at the API*, rather
than instructing a model and hoping.

Language coverage was verified against `/v2/languages` before any spend: all
eleven targets are supported, Ukrainian (`UK`) and Simplified Chinese (`ZH`)
included.

## Budget

Measured before spending, deduplicating identical source strings across modules:

| | characters |
|---|---:|
| estimate before the run | 359,702 |
| actually sent | **288,472** |
| remaining of 500,000 | 173,873 |

Deduplication saved ~10%; stripping the shortcut label and the accelerator
before sending saved the rest.

## Four defects found by running it, each caught on a small sample first

**A6 -- double HTML escaping destroyed accelerators.** `_protect` escaped the
whole string and *then* ran the token regex over the result, by which point
`&` was already `&amp;`; the regex matched the `&` inside that entity and
escaped it again. `&CRC/SFV` came back as `& amp;CRC/SFV`. Fixed by escaping
each segment exactly once.

**A7 -- the engine inserted a space after a protected fragment.** `&CRC/SFV` ->
`& CRC/SFV`. The `&` count is unchanged, so a naive check passes, but an
accelerator followed by whitespace underlines nothing. Added `repair()` (undo
the space when the English had none) and an `accelerator-space` validator rule.

**A8 -- ignore tags are honoured but repositioned.** `&Close	Esc` came back as
`	Esc schliessen`: the key name intact, but now ahead of the translated verb.
Fixed by never sending the shortcut label -- split it off, translate the body,
reattach locally.

**A9 -- protecting `&` splits the word it marks.** `Save p&assphrase` was sent
as `p` + `<x>&</x>` + `assphrase`, so the engine translated two meaningless
fragments and returned them untranslated. This one is structural: the marker
sits *inside* a word. Fixed by stripping the accelerator entirely, translating
whole words, and re-applying the marker afterwards -- preferring the original
letter where the translation contains it, else the first letter.

Re-translating the affected entries cost 48,966 characters and needed a
`--redo-accelerators` flag, because the `.origin` sidecar had already recorded
them as machine-translated and would otherwise have kept them.

## A10: the rebrand pass must not touch accelerators it did not move

Applying A9's strip/re-insert unconditionally inside `rebrand()` relocated the
accelerator in **1,145 already-translated entries** that had no brand mention
at all (`"Porovnat a&tributy"` -> `"Porovna&t atributy"`) -- re-insertion picks
the first occurrence of the letter, which is not where a human put it.

`rebrand()` now runs the rules plainly first and returns that when it matches,
so unrelated text is byte-identical; the strip/retry path is used only when
nothing matched, which is the case an accelerator inside the product name
(`"Salamand&er"`) actually needs.

The damage was repaired from git: for every entry the sidecar marks `human`,
the text was recomputed from the committed pre-merge file. Verified afterwards
-- 60,017 human entries match the git reference exactly, 0 differences.

## A11: the layout validator is opt-in, not part of the build

`-quiet-validate-layout` exits cleanly only when it finds **nothing**
(`wndframe.cpp:361`); with findings it falls through to interactive mode and
sits until the timeout guard kills it. Every reported module therefore costs a
full 30 s. Running it for all 220 pairs put the build on track for ~2 hours, so
it moved behind `--check-layout`. Import alone runs the full matrix in a few
minutes.
