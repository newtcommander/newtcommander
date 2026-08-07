# CLI Contract: `translate.merge` re-translation flags

**Feature**: 055-contextual-retranslation · Extends the tooling contract of
`specs/039-language-build-policy/contracts/translate-tooling.md`.

## New flags

### `--redo-machine`

```text
python -m translate.merge --all --redo-machine [--exclude-module NAME]...
```

| Aspect | Contract |
|---|---|
| Selection | Every entry whose `.origin` provenance is `machine` is demoted to a gap before matching, so it is re-sent to the engine **with section context**. Entries with `human` or `skip` provenance are never demoted and never re-sent. |
| Mechanism | Demotion happens in `load_origin()` (same mechanism as `--redo-accelerators`, superset of its effect); `match()`/`collect_gaps()`/`build_slt()` are otherwise unchanged. |
| Missing sidecar | A (language, module) without an `.origin` file has no `machine` entries by definition — the flag is a no-op there (normal gap collection still applies). |
| Idempotence | A second `--redo-machine` run re-sends the same population again (machine provenance is restored on success); without the flag, a re-run keeps the new texts and spends nothing. |
| Composition | Implies the effect of `--redo-accelerators` (its set ⊂ machine set); combining both is allowed and redundant. Combining with `--retranslate` is allowed but `--retranslate` dominates (it ignores origin entirely) — the help text says so. |
| Validation failure | Unchanged policy: `repair()` then `check()`; failure ⇒ English text, state 0, provenance `english_fallback`, listed under "validation failures". The old machine text is not kept. |
| Dry run | `--dry-run --redo-machine` reports gap counts and character estimate per language with **no network access and no writes** (exit 0). |

### `--exclude-module NAME` (repeatable)

| Aspect | Contract |
|---|---|
| Effect | Removes NAME from the module set after enabled-module resolution; the remaining modules keep cross-module `(context, text)` deduplication within each language. |
| Unknown name | `error: unknown or disabled module 'NAME'` on stderr, exit 1 (same wording class as `--module`). |
| Composition | Valid with `--all`, `--language`, `--dry-run`, `--redo-machine`, `--budget`. Combining with `--module X` where X is also excluded empties the set ⇒ error, exit 1. |
| Excluding everything | An empty resulting module set is an error, exit 1. |

## Unchanged guarantees relied upon (regression surface)

1. Output `.slt` reproduces the template structure positionally; only text,
   state, geometry change.
2. `ui-overrides.json` pinned texts win over engine output and are recorded as
   `human`.
3. `.origin` sidecar rewritten to describe the actual outcome of every entry.
4. Budget cap (`--budget N`) stops **before** a language starts, never mid-language.
5. The build (`build.cmd` / `build_langs.cmd`) never invokes any of this.

## Feature-run invocations (the contract consumers)

```bat
:: 1. preview — offline, no writes; numbers must match .origin machine counts
python -m translate.merge --dry-run --redo-machine --exclude-module sftp

:: 2. the run — 8 enabled languages, 19 modules, context-aware refresh
python -m translate.merge --all --redo-machine --exclude-module sftp
```
