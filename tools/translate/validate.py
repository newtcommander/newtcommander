"""Checks that a translation preserved everything non-textual.

`.slt` text is not free prose. It carries markers the resource compiler and the
running product depend on, and a translation service will happily drop or
reorder them:

===================  ==========================  ==========================
Marker               Example                     Rule
===================  ==========================  ==========================
Accelerator          ``&Kopírovat``              same count; letter may move
Shortcut label       ``"...\\tCtrl+C"``           everything from ``\\t`` is a
                                                 key name -- never translated
printf placeholder   ``%s %d %u %c %x %ld``      same multiset
Escape sequence      ``\\n \\r \\t \\\\``            preserved verbatim
===================  ==========================  ==========================

`ImportTextArchive` accepts a translation that dropped a ``%s`` without
complaint; the defect only shows up at run time as a malformed message. So this
is a producer-side responsibility.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field

#: printf-style conversions, including width/precision/length modifiers and the
#: positional ``%1$s`` form.
_PLACEHOLDER = re.compile(r"%(?:\d+\$)?[-+ #0]*\d*(?:\.\d+)?(?:hh|h|ll|l|L|z|j|t)?[diouxXeEfgGaAcspn%]")

#: Two-character escape sequences as they appear literally in a .slt.
_ESCAPE = re.compile(r"\\[nrt\\\"]")


@dataclass
class Problem:
    kind: str
    detail: str


@dataclass
class Result:
    ok: bool = True
    problems: list[Problem] = field(default_factory=list)

    def fail(self, kind: str, detail: str) -> None:
        self.ok = False
        self.problems.append(Problem(kind, detail))

    def __str__(self) -> str:
        return "; ".join(f"{p.kind}: {p.detail}" for p in self.problems)


def split_shortcut(text: str) -> tuple[str, str]:
    """Split ``"&Copy\\tCtrl+C"`` into ``("&Copy", "\\tCtrl+C")``.

    The shortcut label is a key name (``Ctrl+C``, ``Alt+F4``) and must survive
    translation byte-for-byte -- translating "Ctrl" into another language would
    describe a key that does not exist.
    """
    i = text.find("\\t")
    if i < 0:
        return text, ""
    return text[:i], text[i:]


def split_accel(text: str) -> tuple[str, str | None]:
    """Remove the accelerator marker, returning ``(clean_text, letter)``.

    Protecting ``&`` with an ignore tag does not work, because the marker sits
    *inside* a word: ``"Save p&assphrase"`` becomes ``p`` + ``<x>&</x>`` +
    ``assphrase``, and the engine then translates two meaningless fragments --
    the observed output was a literally untranslated ``"Zberegti p &assphrase"``.
    Worse, a repositioned tag can leave the marker stranded at the end of the
    string or quoted as if it were a word.

    So the marker is stripped entirely, the whole word is translated as a word,
    and the accelerator is re-applied afterwards by :func:`reinsert_accel`.
    ``&&`` (a literal ampersand) is left alone.
    """
    i = 0
    while i < len(text):
        if text[i] == "&":
            if i + 1 < len(text) and text[i + 1] == "&":
                i += 2
                continue
            if i + 1 < len(text):
                return text[:i] + text[i + 1 :], text[i + 1]
            return text[:i], None  # trailing '&' underlines nothing
        i += 1
    return text, None


def reinsert_accel(text: str, letter: str | None) -> str:
    """Re-apply the accelerator to translated text.

    Prefers the original letter so keyboard habits survive where the translated
    word happens to contain it; otherwise falls back to the first letter of the
    string, which always yields a working accelerator. Collisions within a
    dialog are reported separately rather than guessed at here -- this function
    has no view of its neighbours.
    """
    if not letter:
        return text
    lowered = text.lower()
    for m in re.finditer(re.escape(letter.lower()), lowered):
        if m.start() > 0 and text[m.start() - 1] == "&":
            continue  # already part of a literal '&&'
        return text[: m.start()] + "&" + text[m.start() :]
    m = re.search(r"[^\W_]", text, re.UNICODE)
    pos = m.start() if m else 0
    return text[:pos] + "&" + text[pos:]


def repair(english: str, translated: str) -> str:
    """Undo mechanical damage a translation engine introduces around markers.

    The one that actually happens: an accelerator at the start of a string comes
    back as ``"& CRC/SFV"`` instead of ``"&CRC/SFV"`` -- the engine inserts a
    space after the protected fragment. ``&`` followed by whitespace underlines
    nothing, so the accelerator is dead, and a plain ``&``-count check does not
    notice because the count is unchanged.

    Only applied where the English had no space after its ``&``, so a source
    string that legitimately contains ``"& "`` is left alone.
    """
    if re.search(r"&\s", english):
        return translated
    # Keep literal "&&" intact; only a single & followed by space is repaired.
    return re.sub(r"(?<!&)&(\s+)(?=\S)", "&", translated)


def check(english: str, translated: str) -> Result:
    """Verify a translation against its English source."""
    r = Result()

    if not translated.strip():
        r.fail("empty", "translation is empty")
        return r

    # A quoted field runs to the end of its line, so a newline would split the
    # row in two and desynchronise the whole positional file.
    if "\n" in translated or "\r" in translated:
        r.fail("newline", "translation contains a real newline")

    # Interior quotes are legal (the parser only needs the last character to be
    # a quote), but a trailing one would be eaten as the terminator.
    if translated.endswith('"'):
        r.fail("trailing-quote", "translation ends with a quote")

    en_ph = sorted(_PLACEHOLDER.findall(english))
    tr_ph = sorted(_PLACEHOLDER.findall(translated))
    if en_ph != tr_ph:
        r.fail("placeholders", f"{en_ph} -> {tr_ph}")

    en_esc = sorted(_ESCAPE.findall(english))
    tr_esc = sorted(_ESCAPE.findall(translated))
    if en_esc != tr_esc:
        r.fail("escapes", f"{en_esc} -> {tr_esc}")

    _, en_sc = split_shortcut(english)
    _, tr_sc = split_shortcut(translated)
    if en_sc != tr_sc:
        r.fail("shortcut", f"{en_sc!r} -> {tr_sc!r}")

    en_amp = english.count("&") - 2 * english.count("&&")
    tr_amp = translated.count("&") - 2 * translated.count("&&")
    if en_amp != tr_amp:
        r.fail("accelerator", f"{en_amp} -> {tr_amp}")

    # An accelerator must sit immediately before the character it underlines.
    # "& C" keeps the & count intact but underlines nothing.
    if not re.search(r"&\s", english) and re.search(r"(?<!&)&\s", translated):
        r.fail("accelerator-space", "'&' followed by whitespace")

    return r


def duplicate_accelerators(texts: list[str]) -> list[str]:
    """Accelerator letters used more than once within one dialog or menu.

    Windows resolves a duplicate accelerator by cycling between the controls
    instead of activating one, so a duplicate is a real keyboard-navigation
    defect (spec SC-006).
    """
    seen: dict[str, int] = {}
    for text in texts:
        body, _ = split_shortcut(text)
        i = 0
        while i < len(body) - 1:
            if body[i] == "&":
                if body[i + 1] == "&":  # literal ampersand
                    i += 2
                    continue
                seen[body[i + 1].lower()] = seen.get(body[i + 1].lower(), 0) + 1
                break
            i += 1
    return sorted(k for k, v in seen.items() if v > 1)
