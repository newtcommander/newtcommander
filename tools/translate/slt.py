"""Reader and writer for ``.slt`` translation text archives.

The format is defined by two functions in the Translator's C++ source, and this
module is written to match them exactly:

* writer -- ``CData::ExportAsTextArchive`` (``src/translator/trldata.cpp:1393``)
* reader -- ``CData::ImportTextArchive`` (``src/translator/trldata.cpp:2301``)

Full grammar: ``specs/038-translations-build-integration/contracts/slt-format.md``.

Three properties of the real format are easy to get wrong, so they are spelled
out here:

**Quoted text is not escaped.** ``GetUnicodeItemText`` (``trldata.cpp:1924``)
takes the first ``"`` after the last numeric field, then scans to the *final
character of the line* and requires it to be ``"``. Everything between is the
text, verbatim. A row like::

    10048,1,"Soubor: "%s"\\nChyba: %s"

is therefore valid and contains two interior quotes. Any writer that escapes
them produces a file the reader will mis-parse.

**Escape sequences are literal two-character pairs.** ``\\n``, ``\\t``, ``\\r``,
``\\\\`` appear in the file as backslash + letter. There are no real tab or
newline characters inside a quoted field -- the format is strictly line-based.

**Section order and row count are load-bearing.** The reader walks the module's
parsed resources and requires the file to line up positionally; it never looks an
entry up by ID. A single extra, missing, or reordered row rejects the entire
file. This module therefore preserves order exactly and never sorts anything.
"""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass, field
from pathlib import Path

from . import TRANSLATIONS_DIR

BOM = "﻿"
NEWLINE = "\r\n"

#: Translation states, mirroring ``PROGRESS_STATE_*`` in ``trldata.h``.
STATE_UNTRANSLATED = 0
STATE_TRANSLATED = 1


class SltError(ValueError):
    """Raised when a ``.slt`` file does not match the grammar."""

    def __init__(self, path: Path | str, line_number: int, message: str) -> None:
        super().__init__(f"{path}:{line_number}: {message}")
        self.path = path
        self.line_number = line_number


# ---------------------------------------------------------------------------
# Model
# ---------------------------------------------------------------------------


@dataclass
class Row:
    """One translatable entry.

    ``numbers`` holds the leading integer fields verbatim and in order, so a row
    round-trips regardless of which kind it is:

    ==================  ================================
    Row kind            ``numbers``
    ==================  ================================
    Dialog caption      ``[cx, cy, state]``
    Dialog control      ``[id, x, y, cx, cy, state]``
    Menu item           ``[id, state]``
    String-table item   ``[id, state]``
    ==================  ================================

    ``state`` is always the last number, and ``entry_id`` is the first for every
    kind except a dialog caption (which has no ID of its own -- it belongs to the
    dialog).
    """

    numbers: list[int]
    text: str

    @property
    def state(self) -> int:
        return self.numbers[-1]

    @state.setter
    def state(self, value: int) -> None:
        self.numbers[-1] = value

    @property
    def entry_id(self) -> int | None:
        """First numeric field, or ``None`` for a dialog caption row."""
        return self.numbers[0] if len(self.numbers) != 3 else None

    def render(self) -> str:
        return ",".join(str(n) for n in self.numbers) + f',"{self.text}"'


@dataclass
class Section:
    """A ``[DIALOG n]`` / ``[MENU n]`` / ``[STRINGTABLE n]`` block."""

    kind: str  # "DIALOG" | "MENU" | "STRINGTABLE"
    number: int
    rows: list[Row] = field(default_factory=list)

    @property
    def key(self) -> tuple[str, int]:
        return (self.kind, self.number)

    def render(self) -> list[str]:
        return [f"[{self.kind} {self.number}]", *(r.render() for r in self.rows), ""]


@dataclass
class SltFile:
    """A parsed ``.slt``, preserving everything needed for a byte-exact rewrite."""

    #: ``[EXPORTINFO]`` values in file order. Ignored by the reader
    #: (``trldata.cpp:2404-2412`` skips all four) but the *line count* matters.
    exportinfo: list[tuple[str, str]] = field(default_factory=list)

    #: ``[TRANSLATION]`` values in file order. ``HELPDIR`` and ``SLGINCOMPLETE``
    #: are optional -- only the main application module carries them (they are
    #: emitted under ``_LANG_SALAMANDER``), so presence must be preserved.
    translation: list[tuple[str, str]] = field(default_factory=list)

    sections: list[Section] = field(default_factory=list)

    #: Dialog IDs from the optional trailing ``[RELAYOUT]`` block.
    relayout: list[int] | None = None

    has_bom: bool = True

    # -- convenience ---------------------------------------------------------

    def translation_value(self, key: str) -> str | None:
        for k, v in self.translation:
            if k == key:
                return v
        return None

    def set_translation_value(self, key: str, value: str) -> None:
        """Replace an existing ``[TRANSLATION]`` value, preserving position.

        Does nothing when the key is absent -- optional keys must not be
        introduced into a module that never had them, since the module's own
        resources decide whether they exist.
        """
        for i, (k, _) in enumerate(self.translation):
            if k == key:
                self.translation[i] = (k, value)
                return

    def iter_entries(self):
        """Yield ``(section, row)`` for every translatable entry, in file order."""
        for section in self.sections:
            for row in section.rows:
                yield section, row

    def entry_count(self) -> int:
        return sum(len(s.rows) for s in self.sections)

    # -- serialization -------------------------------------------------------

    def render(self) -> str:
        lines: list[str] = ["[EXPORTINFO]"]
        lines += [f'{k},"{v}"' for k, v in self.exportinfo]
        lines.append("")

        lines.append("[TRANSLATION]")
        for key, value in self.translation:
            # LANGID is the one unquoted value (writer uses "LANGID,%d").
            lines.append(f"{key},{value}" if key == "LANGID" else f'{key},"{value}"')
        lines.append("")

        for section in self.sections:
            lines += section.render()

        if self.relayout is not None:
            lines.append("[RELAYOUT]")
            lines += [str(i) for i in self.relayout]
            lines.append("")

        prefix = BOM if self.has_bom else ""
        return prefix + NEWLINE.join(lines) + NEWLINE

    def to_bytes(self) -> bytes:
        return self.render().encode("utf-8")

    def write(self, path: Path) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(self.to_bytes())


# ---------------------------------------------------------------------------
# Parsing
# ---------------------------------------------------------------------------


def _split_row(raw: str, field_count: int, path, line_number: int) -> Row:
    """Parse ``n1,n2,...,"text"`` with exactly ``field_count`` leading integers.

    The text is taken from the first ``"`` after the last integer through the
    final character of the line, which must also be ``"`` -- matching
    ``GetUnicodeItemText``. Interior quotes and commas are part of the text.
    """
    numbers: list[int] = []
    pos = 0
    for _ in range(field_count):
        comma = raw.find(",", pos)
        if comma < 0:
            raise SltError(path, line_number, f"expected {field_count} numeric fields")
        token = raw[pos:comma]
        try:
            numbers.append(int(token))
        except ValueError:
            raise SltError(path, line_number, f"non-numeric field {token!r}") from None
        pos = comma + 1

    if pos >= len(raw) or raw[pos] != '"':
        raise SltError(path, line_number, "expected quoted text after numeric fields")
    if not raw.endswith('"') or len(raw) - pos < 2:
        raise SltError(path, line_number, "text must be terminated by a quote at end of line")

    return Row(numbers=numbers, text=raw[pos + 1 : -1])


def _split_keyed(raw: str, path, line_number: int) -> tuple[str, str]:
    """Parse ``KEY,"value"`` or ``KEY,<int>`` (LANGID is unquoted)."""
    comma = raw.find(",")
    if comma < 0:
        raise SltError(path, line_number, f"expected KEY,value -- got {raw!r}")
    key, value = raw[:comma], raw[comma + 1 :]
    if value.startswith('"'):
        if not value.endswith('"') or len(value) < 2:
            raise SltError(path, line_number, "unterminated quoted value")
        value = value[1:-1]
    return key, value


def parse(data: bytes, path: Path | str = "<bytes>") -> SltFile:
    """Parse ``.slt`` bytes into an :class:`SltFile`."""
    text = data.decode("utf-8")
    has_bom = text.startswith(BOM)
    if has_bom:
        text = text[1:]

    lines = text.split(NEWLINE)
    # A file always ends with a section terminator blank line plus the final
    # newline, which split() turns into one trailing empty element.
    if lines and lines[-1] == "":
        lines.pop()

    slt = SltFile(has_bom=has_bom)
    i = 0
    n = len(lines)

    def lineno() -> int:
        return i + 1 + (1 if has_bom else 0)

    if i >= n or lines[i] != "[EXPORTINFO]":
        raise SltError(path, lineno(), "file must start with [EXPORTINFO]")
    i += 1
    while i < n and lines[i] != "":
        slt.exportinfo.append(_split_keyed(lines[i], path, lineno()))
        i += 1
    i += 1  # blank

    if i >= n or lines[i] != "[TRANSLATION]":
        raise SltError(path, lineno(), "expected [TRANSLATION]")
    i += 1
    while i < n and lines[i] != "":
        slt.translation.append(_split_keyed(lines[i], path, lineno()))
        i += 1
    i += 1  # blank

    while i < n:
        header = lines[i]
        if not header.startswith("["):
            raise SltError(path, lineno(), f"expected a section header -- got {header!r}")

        if header == "[RELAYOUT]":
            i += 1
            slt.relayout = []
            while i < n and lines[i] != "":
                try:
                    slt.relayout.append(int(lines[i]))
                except ValueError:
                    raise SltError(path, lineno(), "RELAYOUT expects bare dialog IDs") from None
                i += 1
            i += 1
            continue

        try:
            kind, number_text = header[1:-1].split(" ", 1)
            number = int(number_text)
        except ValueError:
            raise SltError(path, lineno(), f"malformed section header {header!r}") from None
        if kind not in ("DIALOG", "MENU", "STRINGTABLE"):
            raise SltError(path, lineno(), f"unknown section kind {kind!r}")

        section = Section(kind=kind, number=number)
        i += 1
        first = True
        while i < n and lines[i] != "":
            # A dialog's first row is the dialog itself: cx,cy,state,"caption".
            # Everything else carries a leading control/item/string ID.
            if kind == "DIALOG":
                field_count = 3 if first else 6
            else:
                field_count = 2
            section.rows.append(_split_row(lines[i], field_count, path, lineno()))
            first = False
            i += 1
        i += 1  # blank
        slt.sections.append(section)

    return slt


def load(path: Path) -> SltFile:
    return parse(Path(path).read_bytes(), path)


# ---------------------------------------------------------------------------
# Round-trip verification (task T007)
# ---------------------------------------------------------------------------


def verify_round_trip(root: Path | None = None) -> int:
    """Parse and re-serialize every committed ``.slt``, asserting byte equality.

    This is the guard for the whole feature: if the writer cannot reproduce a
    file the Translator produced, it cannot be trusted to produce one the
    Translator will read back -- and a mis-shaped ``.slt`` is rejected wholesale
    rather than partially.

    Returns a process exit code (0 on success).
    """
    root = Path(root) if root else TRANSLATIONS_DIR
    files = sorted(root.glob("*/*.slt"))
    if not files:
        print(f"no .slt files found under {root}", file=sys.stderr)
        return 1

    failures: list[str] = []
    entries = 0
    for path in files:
        original = path.read_bytes()
        try:
            slt = parse(original, path)
        except SltError as exc:
            failures.append(f"PARSE  {exc}")
            continue
        rebuilt = slt.to_bytes()
        if rebuilt != original:
            failures.append(f"BYTES  {path}: {_describe_diff(original, rebuilt)}")
            continue
        entries += slt.entry_count()

    print(f"checked {len(files)} files, {entries} entries")
    if failures:
        print(f"\n{len(failures)} FAILED:", file=sys.stderr)
        for f in failures[:20]:
            print("  " + f, file=sys.stderr)
        if len(failures) > 20:
            print(f"  ... and {len(failures) - 20} more", file=sys.stderr)
        return 1
    print("round-trip byte-exact for every file")
    return 0


def _describe_diff(a: bytes, b: bytes) -> str:
    if len(a) != len(b):
        note = f"length {len(a)} -> {len(b)}"
    else:
        note = "same length"
    for i, (x, y) in enumerate(zip(a, b)):
        if x != y:
            lo = max(0, i - 40)
            return f"{note}, first difference at byte {i}: {a[lo:i+20]!r} != {b[lo:i+20]!r}"
    return note


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        prog="translate-slt",
        description="Inspect and verify .slt translation text archives.",
    )
    parser.add_argument(
        "--verify",
        action="store_true",
        help="parse and re-serialize every committed .slt, asserting byte equality",
    )
    parser.add_argument("--root", type=Path, default=None, help="translations directory to scan")
    parser.add_argument("--show", type=Path, default=None, help="summarize a single .slt file")
    args = parser.parse_args(argv)

    if args.show:
        slt = load(args.show)
        print(f"{args.show}")
        print(f"  BOM             : {slt.has_bom}")
        print(f"  [EXPORTINFO]    : {len(slt.exportinfo)} values")
        for k, v in slt.translation:
            print(f"  {k:<16}: {v}")
        kinds: dict[str, int] = {}
        for s in slt.sections:
            kinds[s.kind] = kinds.get(s.kind, 0) + 1
        print(f"  sections        : " + ", ".join(f"{k}={v}" for k, v in sorted(kinds.items())))
        print(f"  entries         : {slt.entry_count()}")
        print(f"  [RELAYOUT]      : {len(slt.relayout) if slt.relayout is not None else 'absent'}")
        return 0

    if args.verify:
        return verify_round_trip(args.root)

    parser.print_help()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
