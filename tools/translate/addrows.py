"""Adds rows that a dialog gained, into the committed ``.slt`` files.

Sibling of :mod:`translate.relayout`, for the other half of the same problem.
``relayout`` refreshes geometry when a dialog's controls move; this one handles a
dialog that gained or lost controls, which ``relayout`` refuses to touch because
the row counts no longer line up.

Why not just run ``merge``. ``merge`` re-translates every entry that is still an
English fallback, not only the new ones — measured on this module: 8 gaps per
language when only 2 rows were actually added. Re-translating the other 6 is an
unrequested change to shipped text. This pass instead copies **only** the rows
that are missing, in template order, taking their text from the template
(i.e. English) and marking them untranslated so a later ``merge`` or a human can
fill them in deliberately.

The `.slt` reader/writer is positional and the Translator rejects a file whose
rows do not line up with the module's resources, so "insert at the right index"
is the whole job. Rows are matched by control id within a section.

Usage::

    python -m translate.addrows --module sftp --dry-run
    python -m translate.addrows --module sftp
    python -m translate.addrows --module sftp --language czech
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from . import REPO_ROOT
from .config import ConfigError, load_enabled_modules, load_languages
from .slt import STATE_UNTRANSLATED, load


def default_templates_dir() -> Path:
    return Path(REPO_ROOT) / "build" / "tandemcommander" / "translator" / "templates"


def _row_id(row) -> object:
    """Identity of a row within its section: the control id, or None for a caption."""
    # a dialog caption row carries [cx, cy, state]; a control row [id, x, y, cx, cy, state]
    return row.numbers[0] if len(row.numbers) == 6 else None


def sync_file(template_path: Path, target_path: Path, write: bool) -> tuple[int, list[str]]:
    """Insert rows the target is missing, in template order. Returns (added, notes)."""
    template = load(template_path)
    target = load(target_path)
    tpl_sections = {s.key: s for s in template.sections}
    added = 0
    notes: list[str] = []

    for section in target.sections:
        tpl = tpl_sections.get(section.key)
        if tpl is None or len(tpl.rows) == len(section.rows):
            continue
        if len(tpl.rows) < len(section.rows):
            notes.append(f"{section.key}: target has MORE rows than the template "
                         f"({len(section.rows)} vs {len(tpl.rows)}) - not handled, skipped")
            continue

        have = {_row_id(r) for r in section.rows if _row_id(r) is not None}
        rebuilt = []
        src = iter(section.rows)
        current = next(src, None)
        for tpl_row in tpl.rows:
            rid = _row_id(tpl_row)
            if rid is not None and rid not in have:
                # a genuinely new control: take the template's row verbatim, but
                # mark it untranslated so it is visible as needing a translation
                new_row = type(tpl_row)(numbers=list(tpl_row.numbers), text=tpl_row.text)
                new_row.numbers[-1] = STATE_UNTRANSLATED
                rebuilt.append(new_row)
                added += 1
                notes.append(f"{section.key}: + id {rid} {tpl_row.text!r}")
                continue
            if current is not None:
                rebuilt.append(current)
                current = next(src, None)
        while current is not None:  # keep anything left over rather than dropping it
            rebuilt.append(current)
            current = next(src, None)
        section.rows = rebuilt

    if write and added:
        target.write(target_path)
    return added, notes


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--module", required=True, help="module whose dialog gained controls")
    ap.add_argument("--language", action="append", dest="languages",
                    help="restrict to this language folder (repeatable)")
    ap.add_argument("--dry-run", action="store_true", help="report, write nothing")
    ap.add_argument("--templates", type=Path, default=None)
    args = ap.parse_args(argv)

    templates_dir = args.templates or default_templates_dir()
    template_path = templates_dir / f"{args.module}.slt"
    if not template_path.is_file():
        print(f"error: no template at {template_path}", file=sys.stderr)
        print("       run: src\\vcxproj\\build_langs.cmd --export-templates --module "
              f"{args.module}", file=sys.stderr)
        return 1

    try:
        modules = {m.name for m in load_enabled_modules()}
        languages = load_languages(include_disabled=True)
    except ConfigError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    if args.module not in modules:
        print(f"error: {args.module} is not an enabled module", file=sys.stderr)
        return 1
    if args.languages:
        wanted = set(args.languages)
        languages = [l for l in languages if l.folder in wanted]
        if not languages:
            print("error: no matching language", file=sys.stderr)
            return 1
    # Default: every registered language, like `rebrand` and `relayout`. A
    # structural change must reach the disabled languages too, or re-enabling one
    # later fails on a row-count mismatch.

    total = 0
    print(f"template: {template_path}" + ("  (dry run -- nothing written)" if args.dry_run else ""))
    for language in languages:
        target = language.directory / f"{args.module}.slt"
        if not target.is_file():
            continue
        added, notes = sync_file(template_path, target, not args.dry_run)
        total += added
        print(f"  {language.folder:20s} {added} row(s) added")
        for n in notes:
            print(f"      {n}")
    print(f"\nrows added: {total}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
