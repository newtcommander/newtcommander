# Contract: translation authoring tools (`tools/translate/`)

Covers FR-012 (skip disabled by default) and FR-013 (an explicit way to process
one anyway).

---

## `config.Language`

Gains one field:

```python
@dataclass(frozen=True)
class Language:
    ...
    origin: str          # human | mixed | machine
    enabled: bool        # new -- parsed from "enabled = on|off"
```

---

## `config.load_languages()`

```python
def load_languages(
    path: Path | None = None,
    include_disabled: bool = False,
) -> list[Language]:
```

| Aspect | Behaviour |
|---|---|
| Default | Returns **enabled languages only** |
| `include_disabled=True` | Returns every registered language |
| Validation | Runs over **all** records either way — V1..V5 in data-model.md |
| Missing/invalid `enabled` | `ConfigError` naming the section |

The default is the safe one on purpose. A reader that returned everything would
put the policy in each caller's hands, and an omission in a future caller would
fail *open* — quietly spending translation budget on a language nobody ships.

---

## `merge.py` — the tool the requirement is about

`merge` is what spends DeepL characters, so it is where FR-012 bites.

| Invocation | Languages processed |
|---|---|
| `--all` | Enabled only |
| `--module <m>` | Enabled only |
| `--language <enabled>` | That one |
| `--language <disabled>` | That one, after a notice (FR-013 opt-in) |

Naming a language explicitly **is** the opt-in — no extra flag. A maintainer who
types the name has expressed the intent; a second flag that must always
accompany an explicit name is ceremony people alias away.

Not silent:

```text
note: 'ukrainian' is disabled in languages.cfg -- processing it because you named it explicitly
```

`--language <unregistered>` keeps its existing error.

---

## `rebrand.py` — a deliberate opt-in

Calls `load_languages(include_disabled=True)`.

`rebrand` translates nothing and spends nothing; it is a correctness sweep over
committed source. Skipping disabled languages would let brand residue accumulate
in their `.slt` files and surface the moment one is re-enabled — which would make
FR-005 ("re-enabling requires no re-translation") false in practice. Opting in
explicitly at the call site is the FR-013 mechanism used from code instead of a
command line, and the call carries a comment saying so.

---

## `config.main()` — the matrix printer

Passes `include_disabled=True` and marks state in its table, so
`python -m translate.config` shows the whole registry and what ships:

```text
language              langid  origin    state    with source
--------------------------------------------------------------
czech                   1029  mixed     on            20/20
...
russian                 1049  mixed     off           20/20
```

The header line reports both counts, e.g.
`8 of 11 languages enabled x 20 modules = 160 language modules (+20 english)`.

---

## Summary of the policy across tools

| Tool | Default | Opt-in | Why |
|---|---|---|---|
| `merge` | enabled only | `--language <disabled>` | spends translation budget |
| `rebrand` | all | *(is the opt-in)* | spends nothing; missing residue is a defect |
| `config` (printer) | all | *(is the opt-in)* | its job is to show the policy |
| `build_langs` | enabled only | none — errors instead | a build tool; FR-002 forbids producing disabled modules |
