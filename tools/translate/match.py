"""Entry matching between an English template and a legacy translation.

The template defines the structure that must be reproduced exactly (``.slt``
import is positional). A legacy file describes a *different*, older structure,
so entries are carried over by identity, not by position:

* ``[STRINGTABLE n]`` / ``[MENU n]`` row -> ``(kind, section, entry_id)``
* ``[DIALOG n]`` control row             -> ``("DIALOG", n, control_id)``
* ``[DIALOG n]`` caption row             -> ``("DIALOG", n, "#caption")``

A control ID is unique within its dialog (the Translator requires it), and
string/menu IDs are unique within their section, so the key is unambiguous.

Anything the legacy file does not cover is a gap to be machine-translated;
anything it covers that the template does not is obsolete and dropped
(spec FR-014).
"""

from __future__ import annotations

import re
from dataclasses import dataclass

from .slt import Row, Section, SltFile

CAPTION_KEY = "#caption"

#: Text that carries no translatable content -- empty, or only separators and
#: placeholders. Sending these to a translation service wastes quota and risks
#: mangling them.
_NON_TRANSLATABLE = {"", "-", "--", "---"}

#: Words that must stay English in a user interface. "OK" is the button label
#: every one of these locales uses; DeepL renders it as "Dobře" / "Gut" /
#: "Хорошо", which is correct prose and wrong for a button.
_KEEP_ENGLISH = {"ok", "n/a", "id", "url", "ip", "dns", "ftp", "sftp", "http",
                 "https", "crc", "md5", "sha", "utf-8", "ascii", "ansi"}

#: Resource-editor placeholder names for controls that never display their
#: text (list boxes, progress bars, edit fields). Translating "List1" into
#: "Seznam 1" costs quota and achieves nothing.
_CONTROL_PLACEHOLDER = re.compile(
    r"^(?:List|Progress|Edit|Static|Button|Combo|Check|Radio|Group|Tree|Tab|"
    r"Slider|Spin|Scroll|Animate|Hotkey|Header|Custom|RichEdit)\d+$"
)


def entry_key(section: Section, row: Row, index: int) -> tuple[str, int, object]:
    """Stable identity for one entry, independent of position in the file."""
    if section.kind == "DIALOG" and index == 0:
        return (section.kind, section.number, CAPTION_KEY)
    return (section.kind, section.number, row.entry_id)


def index_entries(slt: SltFile) -> dict[tuple[str, int, object], Row]:
    """Map every entry in a file to its identity key."""
    out: dict[tuple[str, int, object], Row] = {}
    for section in slt.sections:
        for i, row in enumerate(section.rows):
            out[entry_key(section, row, i)] = row
    return out


def is_translatable(text: str) -> bool:
    """False for text a translator must not be asked to touch.

    Excludes empty strings, bare separators, and text made up entirely of
    placeholders or shortcut labels (``%s``, ``\\t``, ``&``), which carry no
    words to translate.
    """
    stripped = text.strip()
    if stripped in _NON_TRANSLATABLE:
        return False
    if stripped.lower() in _KEEP_ENGLISH:
        return False
    if _CONTROL_PLACEHOLDER.match(stripped):
        return False
    # Strip the mechanical parts and see whether any letters remain.
    probe = stripped
    for token in ("\\n", "\\r", "\\t", "\\\\", "&"):
        probe = probe.replace(token, "")
    i = 0
    while i < len(probe):
        if probe[i] == "%" and i + 1 < len(probe):
            i += 2
            continue
        if probe[i].isalpha():
            return True
        i += 1
    return False


@dataclass
class MatchStats:
    """Outcome of matching one (module, language) pair."""

    total: int = 0  # entries in the template
    human: int = 0  # carried over from the legacy translation
    machine: int = 0  # kept from a previous machine-translation run
    gap: int = 0  # need machine translation
    untranslatable: int = 0  # empty / placeholder-only, left as-is
    discarded: int = 0  # legacy entries with no template counterpart

    def as_row(self) -> str:
        return (
            f"total={self.total} human={self.human} machine={self.machine} "
            f"gap={self.gap} skip={self.untranslatable} discarded={self.discarded}"
        )


def match(
    template: SltFile,
    legacy: SltFile | None,
    origin: dict[str, str] | None = None,
) -> tuple[dict, MatchStats]:
    """Decide, per template entry, where its text comes from.

    Returns ``(plan, stats)`` where ``plan`` maps each entry key to
    ``("human", text)``, ``("machine", text)``, ``("gap", english)`` or
    ``("skip", text)``.

    ``origin`` is the previous run's provenance sidecar. It matters because
    merging **overwrites** the file it read: without it, a second run would see
    last run's machine translations sitting at state=1 and promote them to
    "human", silently losing the distinction a reviewer needs. With it, machine
    text is kept (so re-runs cost no quota) but stays labelled, and only
    genuine gaps and previous English fallbacks are sent for translation.
    """
    stats = MatchStats()
    legacy_index = index_entries(legacy) if legacy is not None else {}
    used: set = set()
    plan: dict = {}
    origin = origin or {}

    for section in template.sections:
        for i, row in enumerate(section.rows):
            key = entry_key(section, row, i)
            stats.total += 1
            english = row.text

            if not is_translatable(english):
                plan[key] = ("skip", english)
                stats.untranslatable += 1
                continue

            hit = legacy_index.get(key)
            # A legacy entry counts only if it actually holds text and was
            # marked translated -- an untranslated legacy row is just English.
            has_text = hit is not None and hit.text and hit.state == 1 and hit.text != english
            was = origin.get(f"{key[0]}:{key[1]}:{key[2]}")

            # A translation identical to its English is legitimate for proper
            # nouns and technical tokens ("Histogram", "LZMA", "UTF-16").
            # Without provenance it is indistinguishable from an untranslated
            # row, so only the sidecar's word makes it count -- which stops
            # the same ~150 entries per language being re-sent to the engine
            # on every single run (and re-risking tokens like $(OriginalName)).
            same_text = (
                hit is not None and hit.text == english and hit.state == 1
                and was in ("human", "machine")
            )

            if same_text:
                plan[key] = (was, hit.text)
                if was == "machine":
                    stats.machine += 1
                else:
                    stats.human += 1
                used.add(key)
            elif has_text and was == "machine":
                # Already machine-translated on an earlier run: keep it, do not
                # spend quota again, but do not call it human either.
                plan[key] = ("machine", hit.text)
                stats.machine += 1
                used.add(key)
            elif has_text and was != "english_fallback":
                plan[key] = ("human", hit.text)
                stats.human += 1
                used.add(key)
            else:
                plan[key] = ("gap", english)
                stats.gap += 1

    stats.discarded = len(set(legacy_index) - used) if legacy_index else 0
    return plan, stats
