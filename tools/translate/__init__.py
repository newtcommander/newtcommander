"""Offline translation tooling for Newt Commander (feature 038).

This package is **never** invoked by the build. It is hand-run by a maintainer
to produce translation source, whose output is committed to ``translations/``
and consumed by ``src\\vcxproj\\build_langs.cmd`` like any other source file.
That separation is what keeps ``build.cmd`` offline, reproducible, and free of
an API dependency (spec FR-023, FR-024).

Three stages -- see ``specs/038-translations-build-integration/quickstart.md``:

1. **Refresh templates** (``build_langs.cmd --export-templates``): export a
   current-structure English ``.slt`` per module from the freshly built
   ``english.slg``.
2. **Merge** (:mod:`translate.merge`): fill each template entry from the legacy
   translation where the ID matches, machine-translate the rest, validate,
   rebrand, and write ``translations/<language>/<module>.slt``.
3. **Build** (``build_langs.cmd``): import each ``.slt`` into a copy of
   ``english.slg`` via ``translator.exe``.

The structural constraint that shapes all of this: ``CData::ImportTextArchive``
(``src/translator/trldata.cpp``) matches entries **positionally**, not by ID, and
rejects the whole file on the first mismatch. Translation source must therefore
always be regenerated against the current English resources -- it can never be
hand-maintained against a stale layout.
"""

from __future__ import annotations

from pathlib import Path

__version__ = "0.1.0"

#: Repository root, derived from this file's location (``tools/translate/``).
REPO_ROOT = Path(__file__).resolve().parents[2]

#: Committed translation source.
TRANSLATIONS_DIR = REPO_ROOT / "translations"

#: Shipped-language registry.
LANGUAGES_CFG = TRANSLATIONS_DIR / "languages.cfg"

#: Plugin build policy -- decides which modules are in scope.
PLUGINS_CFG = REPO_ROOT / "plugins.cfg"

__all__ = [
    "REPO_ROOT",
    "TRANSLATIONS_DIR",
    "LANGUAGES_CFG",
    "PLUGINS_CFG",
    "__version__",
]
