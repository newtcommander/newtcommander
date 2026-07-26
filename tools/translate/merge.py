"""Builds committed translation source from template + legacy + DeepL.

For one (module, language) pair:

1. the **template** exported from the current ``english.slg`` defines the
   structure -- it is reproduced exactly, because ``.slt`` import is positional;
2. each entry is filled from the **legacy** translation where the ID matches;
3. whatever is left is **machine-translated**;
4. everything is validated, rebranded, and written to
   ``translations/<language>/<module>.slt`` plus an ``.origin`` sidecar.

Quota matters (the DeepL free tier bills characters sent), so gaps are collected
across *all* modules of a language, deduplicated, translated once, and then
distributed. Roughly 10% of the raw volume is repeats -- "OK", "Cancel",
"&Yes" and friends appear in almost every module.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path

from .config import Language, Module, load_enabled_modules, load_languages
from .deepl import TARGET_CODES, Client, DeepLError, load_key
from .match import CAPTION_KEY, entry_key, index_entries, match
from .rebrand import find_residue, rebrand
from .slt import Section, SltFile, load
from .validate import check, duplicate_accelerators, repair

DEFAULT_TEMPLATES = "build/salamander/translator/templates"

HUMAN, MACHINE, FALLBACK, SKIP = "human", "machine", "english_fallback", "skip"


@dataclass
class Coverage:
    """Per-(language, module) outcome -- the coverage report of spec FR-015."""

    language: str
    module: str
    total: int = 0
    human: int = 0
    machine: int = 0
    fallback: int = 0
    skip: int = 0
    discarded: int = 0
    rebranded: int = 0
    invalid: list[str] = field(default_factory=list)
    dup_accel: list[str] = field(default_factory=list)

    @property
    def translatable(self) -> int:
        return self.total - self.skip

    def pct_human(self) -> int:
        return 100 * self.human // self.translatable if self.translatable else 0


def load_origin(
    language: Language,
    module: Module,
    template: SltFile | None = None,
    redo_accelerators: bool = False,
) -> dict[str, str]:
    """Previous run's provenance, if any. Absent on the first run.

    ``redo_accelerators`` demotes previously machine-translated entries whose
    English carries an accelerator back to a gap, so they are translated again.
    Needed once, because the first implementation protected ``&`` as an ignore
    tag -- which split the word it marks and produced fragments like
    ``"Zberegti p &assphrase"``.
    """
    path = language.directory / f"{module.name}.origin"
    if not path.is_file():
        return {}
    try:
        origin = json.loads(path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError):
        return {}

    if redo_accelerators and template is not None:
        single_amp = re.compile(r"(?<!&)&(?!&)")
        for section in template.sections:
            for i, row in enumerate(section.rows):
                k = entry_key(section, row, i)
                kk = f"{k[0]}:{k[1]}:{k[2]}"
                if origin.get(kk) == MACHINE and single_amp.search(row.text):
                    origin[kk] = FALLBACK
    return origin


def _carry_geometry(tpl_dialog: Section, legacy_dialog: Section | None) -> bool:
    """Should this dialog reuse the legacy control geometry?

    A human translator resized controls to fit their longer text, and that work
    is worth keeping. But the legacy layout describes the 4.0 dialog; if the
    dialog has since changed, its coordinates would be wrong. Reuse them only
    when the dialog is still the same size, which is a good proxy for "the
    layout did not change".
    """
    if legacy_dialog is None or not legacy_dialog.rows or not tpl_dialog.rows:
        return False
    # Row 0 of a dialog is the dialog itself: cx, cy, state, caption.
    return legacy_dialog.rows[0].numbers[:2] == tpl_dialog.rows[0].numbers[:2]


def build_slt(
    template: SltFile,
    legacy: SltFile | None,
    language: Language,
    translations: dict[str, str],
    cov: Coverage,
    prev_origin: dict[str, str] | None = None,
) -> tuple[SltFile, dict[str, str]]:
    """Produce the merged ``.slt`` and its per-entry provenance map."""
    plan, stats = match(template, legacy, prev_origin)
    cov.total, cov.skip, cov.discarded = stats.total, stats.untranslatable, stats.discarded

    legacy_index = index_entries(legacy) if legacy is not None else {}
    legacy_sections = {s.key: s for s in legacy.sections} if legacy is not None else {}
    origin: dict[str, str] = {}

    out = SltFile(
        exportinfo=list(template.exportinfo),
        translation=list(template.translation),
        sections=[],
        relayout=list(template.relayout) if template.relayout is not None else None,
        has_bom=template.has_bom,
    )

    # [TRANSLATION] identity comes from languages.cfg. set_translation_value only
    # replaces keys the template already has -- HELPDIR and SLGINCOMPLETE exist
    # for the application module but not for plugins, and inventing them would
    # not match the module's own resources.
    out.set_translation_value("LANGID", str(language.langid))
    out.set_translation_value("AUTHOR", language.author)
    out.set_translation_value("WEB", language.web)
    out.set_translation_value("COMMENT", language.comment)
    out.set_translation_value("HELPDIR", language.helpdir)
    # Always empty: an empty SLGIncomplete is how a .slg declares itself a
    # complete translation (salamdr1.cpp:200), and a non-empty one makes the
    # main window pop up a "translation is incomplete" message box at startup
    # (WM_USER_SLGINCOMPLETE, mainwnd3.cpp:5928). Suppressing that popup is a
    # deliberate product decision; provenance is still tracked per entry in the
    # .origin sidecars and reported by the coverage table, so nothing is lost
    # for a reviewer -- only the startup interruption for the user.
    out.set_translation_value("SLGINCOMPLETE", "")

    for tpl_section in template.sections:
        section = Section(kind=tpl_section.kind, number=tpl_section.number)
        legacy_section = legacy_sections.get(tpl_section.key)
        reuse_geometry = tpl_section.kind == "DIALOG" and _carry_geometry(
            tpl_section, legacy_section
        )

        for i, tpl_row in enumerate(tpl_section.rows):
            key = entry_key(tpl_section, tpl_row, i)
            source, value = plan[key]
            numbers = list(tpl_row.numbers)

            if source == SKIP:
                text, kind = value, SKIP
                numbers[-1] = tpl_row.state
            elif source == MACHINE:
                # Carried over from an earlier machine-translation run.
                text, kind = value, MACHINE
                numbers[-1] = 1
                cov.machine += 1
            elif source == HUMAN:
                text, kind = value, HUMAN
                numbers[-1] = 1
                cov.human += 1
                if reuse_geometry:
                    hit = legacy_index.get(key)
                    if hit is not None and len(hit.numbers) == len(numbers):
                        numbers[:-1] = hit.numbers[:-1]
            else:  # gap
                english = value
                candidate = translations.get(english)
                if candidate:
                    candidate = repair(english, candidate)
                    verdict = check(english, candidate)
                    if verdict.ok:
                        text, kind = candidate, MACHINE
                        numbers[-1] = 1
                        cov.machine += 1
                    else:
                        text, kind = english, FALLBACK
                        numbers[-1] = 0
                        cov.fallback += 1
                        cov.invalid.append(f"{english[:60]!r}: {verdict}")
                else:
                    text, kind = english, FALLBACK
                    numbers[-1] = 0
                    cov.fallback += 1

            rebranded = rebrand(text)
            if rebranded != text:
                cov.rebranded += 1
                text = rebranded

            section.rows.append(type(tpl_row)(numbers=numbers, text=text))
            origin[f"{key[0]}:{key[1]}:{key[2]}"] = kind

        # Windows cycles between controls that share an accelerator instead of
        # activating one, so a duplicate within a dialog or menu is a real
        # keyboard-navigation defect (spec SC-006). Reported, not auto-fixed:
        # choosing a different letter is a judgement call about the wording.
        if section.kind in ("DIALOG", "MENU"):
            dups = duplicate_accelerators([r.text for r in section.rows])
            if dups:
                cov.dup_accel.append(
                    f"{section.kind} {section.number}: {', '.join(dups)}"
                )

        out.sections.append(section)

    return out, origin


def collect_gaps(
    templates: dict[str, SltFile],
    language: Language,
    modules: list[Module],
    redo_accelerators: bool = False,
) -> list[str]:
    """Unique English texts needing translation for this language."""
    texts: set[str] = set()
    for module in modules:
        legacy_path = language.directory / f"{module.name}.slt"
        legacy = load(legacy_path) if legacy_path.is_file() else None
        plan, _ = match(
            templates[module.name],
            legacy,
            load_origin(language, module, templates[module.name], redo_accelerators),
        )
        texts.update(v for (src, v) in plan.values() if src == "gap")
    return sorted(texts)


def run(
    languages: list[Language],
    modules: list[Module],
    templates_dir: Path,
    dry_run: bool,
    key_file: Path | None,
    budget: int | None,
    redo_accelerators: bool = False,
) -> int:
    missing = [m.name for m in modules if not (templates_dir / f"{m.name}.slt").is_file()]
    if missing:
        print(f"error: no template for {', '.join(missing)}", file=sys.stderr)
        print(
            "       run: src\\vcxproj\\build_langs.cmd --export-templates",
            file=sys.stderr,
        )
        return 1
    templates = {m.name: load(templates_dir / f"{m.name}.slt") for m in modules}

    client = None
    if not dry_run:
        try:
            client = Client(key=load_key(key_file))
            usage = client.usage()
            print(f"DeepL quota: {usage.remaining:,} of {usage.limit:,} characters remaining")
        except DeepLError as e:
            print(f"error: {e}", file=sys.stderr)
            return 1

    reports: list[Coverage] = []
    spent = 0

    for language in languages:
        code = TARGET_CODES.get(language.folder)
        if code is None:
            print(f"skip {language.folder}: no DeepL target code", file=sys.stderr)
            continue

        gaps = collect_gaps(templates, language, modules, redo_accelerators)
        est = sum(len(g) for g in gaps)
        print(f"\n=== {language.folder} ({code}) -- {len(gaps)} unique gaps, ~{est:,} chars")

        translations: dict[str, str] = {}
        if gaps and not dry_run:
            if budget is not None and spent + est > budget:
                print(f"  budget cap reached ({budget:,}); stopping before {language.folder}")
                break

            def progress(done: int, total: int) -> None:
                print(f"\r  translating {done}/{total}", end="", flush=True)

            try:
                translations = client.translate_all(gaps, code, progress)
            except DeepLError as e:
                print(f"\n  error: {e}", file=sys.stderr)
                return 1
            print(f"\r  translated {len(translations)}/{len(gaps)}   ")
            # sent_chars is cumulative on the client, so assign -- adding it
            # each round would count earlier languages again and again.
            spent = client.sent_chars

        for module in modules:
            legacy_path = language.directory / f"{module.name}.slt"
            legacy = load(legacy_path) if legacy_path.is_file() else None
            cov = Coverage(language=language.folder, module=module.name)
            merged, origin = build_slt(
                templates[module.name], legacy, language, translations, cov,
                load_origin(language, module, templates[module.name], redo_accelerators),
            )
            reports.append(cov)

            if not dry_run:
                out_path = language.directory / f"{module.name}.slt"
                merged.write(out_path)
                (language.directory / f"{module.name}.origin").write_text(
                    json.dumps(origin, indent=0, sort_keys=True, ensure_ascii=False),
                    encoding="utf-8",
                )

    print_report(reports, dry_run)
    if client is not None:
        print(f"\nDeepL characters sent this run: {client.sent_chars:,}")
        print(f"DeepL quota remaining: {client.usage().remaining:,}")
    return 0


def print_report(reports: list[Coverage], dry_run: bool) -> None:
    print("\n" + "=" * 78)
    print("coverage" + ("  (dry run -- nothing written)" if dry_run else ""))
    print("=" * 78)
    print(f"{'language':<20} {'total':>6} {'human':>6} {'machine':>8} {'fallbk':>7} {'skip':>6} {'human%':>7}")
    print("-" * 78)
    by_lang: dict[str, Coverage] = {}
    for r in reports:
        agg = by_lang.setdefault(r.language, Coverage(language=r.language, module="*"))
        agg.total += r.total
        agg.human += r.human
        agg.machine += r.machine
        agg.fallback += r.fallback
        agg.skip += r.skip
        agg.discarded += r.discarded
        agg.rebranded += r.rebranded
        agg.invalid += r.invalid
        agg.dup_accel += [f"{r.module} {d}" for d in r.dup_accel]
    for agg in by_lang.values():
        print(
            f"{agg.language:<20} {agg.total:>6} {agg.human:>6} {agg.machine:>8} "
            f"{agg.fallback:>7} {agg.skip:>6} {agg.pct_human():>6}%"
        )
    print("-" * 78)
    total_invalid = sum(len(a.invalid) for a in by_lang.values())
    total_dups = sum(len(a.dup_accel) for a in by_lang.values())
    total_rebrand = sum(a.rebranded for a in by_lang.values())
    total_discard = sum(a.discarded for a in by_lang.values())
    print(
        f"rebranded entries: {total_rebrand}   discarded legacy: {total_discard}   "
        f"validation failures: {total_invalid}   duplicate accelerators: {total_dups}"
    )
    if total_invalid:
        print("\nvalidation failures (kept English):")
        shown = 0
        for agg in by_lang.values():
            for msg in agg.invalid:
                if shown >= 15:
                    break
                print(f"  [{agg.language}] {msg}")
                shown += 1


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        prog="translate-merge",
        description="Build translations/<lang>/<module>.slt from template + legacy + DeepL.",
    )
    ap.add_argument("--all", action="store_true", help="every language and module")
    ap.add_argument("--language", help="restrict to one language folder")
    ap.add_argument("--module", help="restrict to one module")
    ap.add_argument("--dry-run", action="store_true", help="no API calls, no writes")
    ap.add_argument("--templates", type=Path, default=None)
    ap.add_argument("--key-file", type=Path, default=None)
    ap.add_argument("--budget", type=int, default=None, help="stop before exceeding N characters")
    ap.add_argument(
        "--redo-accelerators",
        action="store_true",
        help="re-translate machine entries whose English carries an accelerator",
    )
    args = ap.parse_args(argv)

    if not (args.all or args.language or args.module or args.dry_run):
        ap.error("specify --all, --language, --module, or --dry-run")

    languages = load_languages()
    modules = load_enabled_modules()
    if args.language:
        languages = [l for l in languages if l.folder == args.language]
        if not languages:
            print(f"error: unknown language {args.language!r}", file=sys.stderr)
            return 1
    if args.module:
        modules = [m for m in modules if m.name == args.module]
        if not modules:
            print(f"error: unknown or disabled module {args.module!r}", file=sys.stderr)
            return 1

    from . import REPO_ROOT

    templates_dir = args.templates or (REPO_ROOT / DEFAULT_TEMPLATES)
    return run(
        languages, modules, templates_dir, args.dry_run, args.key_file,
        args.budget, args.redo_accelerators,
    )


if __name__ == "__main__":
    raise SystemExit(main())
