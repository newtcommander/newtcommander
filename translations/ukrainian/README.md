# Ukrainian translation

> **Currently not shipped.** Feature 039 set `enabled = off` for Ukrainian in
> `../languages.cfg`, together with Russian and Simplified Chinese, because all
> three render incorrectly in menus. The defect is recorded in
> `specs/039-language-build-policy/spec.md` and has not been diagnosed. Nothing
> here was removed — the translation below is complete and intact, and
> re-enabling is a one-line change once the rendering is fixed.
>
> The authoring tools skip disabled languages by default, so
> `python -m translate.merge --all` will not touch this directory. To work on
> it deliberately, name it: `python -m translate.merge --language ukrainian`.

Added by feature 038. **No human translator has contributed to this language** —
every string is produced by machine translation from the English original, so
each `.slt` here is entirely `machine` origin (see the `.origin` sidecars).

Registered in `../languages.cfg` as LANGID **1058** with `origin = machine`,
which makes the build set a non-empty `SLGIncomplete` in every Ukrainian `.slg`.
The product then tells the user this translation is not fully human-reviewed and
points them at where to help.

To (re)generate:

```bat
python -m translate.merge --language ukrainian
```

Corrections from a Ukrainian speaker are welcome and take precedence — editing
text in a `.slt` here is enough; the merge step never overwrites a human-marked
entry. Add or remove rows only via a template refresh, since `.slt` import is
positional (see `../../tools/translate/README.md`).
