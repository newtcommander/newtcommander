"""Widens controls whose translated text no longer fits.

Translations are routinely longer than their English source -- Czech, German and
Romanian especially -- so a control sized for English clips the translation.
The Translator's own validator reports this (``--check-layout``), but reporting
several hundred findings for a human to fix by hand is not a plausible workflow.

This pass fixes the easy majority automatically and leaves the rest to a human:

* only **left-aligned text controls** are touched (statics, checkboxes, radio
  buttons, group boxes) -- these render left-to-right from a fixed origin, so
  growing them rightwards is safe;
* a control is grown only into **genuinely free space**: the gap to the nearest
  neighbour whose vertical extent overlaps it, or to the dialog's right margin;
* nothing is ever moved, shrunk, or grown beyond the dialog.

Width is estimated, not measured: the real width depends on the dialog font and
DPI, which are not available here. The estimate is deliberately generous, and
anything it cannot satisfy is left alone for ``--check-layout`` to report.
"""

from __future__ import annotations

from dataclasses import dataclass

from .slt import Row, Section
from .validate import split_shortcut

#: Right-hand margin kept clear inside the dialog, in dialog units.
MARGIN = 4

#: Average glyph advance in dialog units for the 8pt MS Shell Dlg the product
#: uses. A dialog-unit x is a quarter of the average character width by
#: definition, so ~4 is the baseline; the fudge covers accented and wide glyphs.
UNITS_PER_CHAR = 4.05

#: CJK glyphs are full-width -- roughly double.
UNITS_PER_WIDE_CHAR = 7.6


def estimate_width(text: str) -> int:
    """Rough rendered width of ``text`` in dialog units."""
    body, _ = split_shortcut(text)
    # The accelerator marker is not drawn, and "&&" draws as one character.
    body = body.replace("&&", "\x00").replace("&", "").replace("\x00", "&")
    for esc in ("\\n", "\\r", "\\t"):
        body = body.replace(esc, "")
    narrow = sum(1 for c in body if ord(c) < 0x1100)
    wide = len(body) - narrow
    return int(narrow * UNITS_PER_CHAR + wide * UNITS_PER_WIDE_CHAR) + 2


@dataclass
class Widening:
    dialog: int
    control: int
    old_cx: int
    new_cx: int
    text: str


def _overlaps_vertically(a: Row, b: Row) -> bool:
    ay, ah = a.numbers[2], a.numbers[4]
    by, bh = b.numbers[2], b.numbers[4]
    return ay < by + bh and by < ay + ah


def widen(section: Section, english: dict[int, str]) -> list[Widening]:
    """Grow clipped controls in one dialog, in place. Returns what changed."""
    if not section.rows or section.kind != "DIALOG":
        return []

    dialog_cx = section.rows[0].numbers[0]
    controls = section.rows[1:]
    changes: list[Widening] = []

    for row in controls:
        if len(row.numbers) != 6:  # not a control row
            continue
        cid, x, y, cx, cy = row.numbers[:5]
        if not row.text.strip():
            continue

        needed = estimate_width(row.text)
        if needed <= cx:
            continue

        # A control only ever grows into space nothing else occupies.
        limit = dialog_cx - MARGIN
        for other in controls:
            if other is row or len(other.numbers) != 6:
                continue
            ox = other.numbers[1]
            if ox > x and _overlaps_vertically(row, other):
                limit = min(limit, ox - 2)

        new_cx = min(needed, limit - x)
        if new_cx > cx:
            row.numbers[3] = new_cx
            changes.append(Widening(section.number, cid, cx, new_cx, row.text))

    return changes
