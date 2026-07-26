# Contract: build-side scripts

Three scripts change. One is new.

---

## `src/vcxproj/lang_policy.ps1` *(new)*

The language equivalent of `gen_plugins_filter.ps1`: validate the policy,
reconcile the output, report the counts.

### Invocation

```powershell
lang_policy.ps1 -Config <path\languages.cfg> `
                -TranslationsRoot <path\translations> `
                [-OutputRoot <path\to\Debug_x64>]
```

Called from `build.cmd` immediately after the plugin policy stage, **before
MSBuild, on every build** — not only full builds. That placement is what makes
SC-003 hold: a plain `build.cmd` after flipping a language off must remove its
modules.

### Behaviour

1. Validate the registry (delegating the parse to `read_languages.ps1`). Any
   error listed in `languages-cfg.md` → print all errors, exit 1.
2. If `-OutputRoot` is given and exists, reconcile every `lang` directory found
   at `<OutputRoot>\lang` and `<OutputRoot>\plugins\*\lang`:
   keep `english.slg` and one `.slg` per enabled language, delete every other
   `.slg`. Print one line per removal.
3. Print the summary line.

`-OutputRoot` missing or not yet created is not an error — reconciliation is a
delete-only pass and a tree that does not exist has nothing stale in it.

### Output

```text
Reconcile: removed stale language module <full path>      (0..n lines)
Reconcile: <n> stale language module(s) removed           (only when n > 0)
Languages: <N> enabled, <M> disabled
           off: <comma-separated folders>                 (only when M > 0)
```

The `Languages:` line is parsed by `build.cmd` for the build banner, the same way
`Plugins: N enabled, M disabled` already is. It satisfies FR-009 / SC-007
together with the banner line.

### Exit codes

| Code | Meaning |
|---|---|
| 0 | Policy valid; output reconciled |
| 1 | Validation error (nothing was deleted) |

---

## `src/vcxproj/read_languages.ps1` *(extended)*

Stays a **pure reader** — it never deletes anything.

### Record format

```text
<folder>|<langid>|<origin>|<enabled>
```

`<enabled>` is `1` or `0`. The field is appended; the three existing fields keep
their positions, so the existing positional consumer in `build_langs.ps1` is
unaffected by the addition itself.

Every registered language is emitted, enabled or not — reconciliation needs the
disabled set and the build needs the enabled set, and one reader serving both
means one place to be wrong.

### Validation

Adds `enabled` to the required-field list and validates its value (V4, V5 in
data-model.md). Existing checks are otherwise unchanged, with one correction:
V3 ("every `translations/<folder>/` is registered") now tests against **every
section encountered**, not only the ones that validated. Previously a section
with a bad field was dropped from the list, so its directory was *also* reported
as unregistered — two errors for one mistake, the second one pointing at a
problem that does not exist. Making `enabled` required made that path easy to
hit, so it is fixed here.

---

## `src/vcxproj/build_langs.ps1` *(extended)*

### Language selection

- Default: build **enabled** languages only.
- `-Language <folder>` naming a **disabled** language: error, do not build.

  ```text
  ERROR: language '<folder>' is disabled in languages.cfg -- enable it there to build it
  ```

  `build_langs` is a build tool: FR-002 forbids producing modules for a disabled
  language, and the policy stage would delete the result on the next build
  anyway. Working on a disabled translation is the authoring tools' job
  (`translate-tooling.md`).
- `-Language <folder>` naming an unregistered language: existing error, unchanged.

### Reporting

The existing summary gains a skipped-languages line when any language is
disabled:

```text
  languages skipped (off): 3  -- chinesesimplified, russian, ukrainian
```

---

## `build.cmd` *(extended)*

### Policy stage

After the existing plugins stage (`build.cmd:110-134`), the same shape for
languages: run `lang_policy.ps1` into a log, echo the log, parse
`Languages: <N>` into `ENABLED_LANGS`, delete the log, and stop the build on a
non-zero exit with:

```text
Language policy check FAILED. Fix translations\languages.cfg and try again.
```

### Banner

One line added under the existing plugin policy line, its label kept short so
the colons line up:

```text
 Plugin policy : 19 plugins enabled (plugins.cfg)
 Lang policy   : 8 of 11 languages enabled (languages.cfg)
```

### Ordering

```text
prerequisites → plugin policy → language policy → MSBuild → [full] populate_runtime → build_langs
```

Reconciliation before MSBuild and production after it is deliberate: the delete
pass has no dependency on build output existing, and the produce pass has every
dependency on it.
