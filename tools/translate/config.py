"""Reads ``translations/languages.cfg`` and ``plugins.cfg``; enumerates the matrix.

Two policy files decide the scope of every translation operation:

* ``plugins.cfg`` -- which plugins are compiled and shipped (``name=on|off``).
  Modules that are off must not get language files (spec FR-005).
* ``translations/languages.cfg`` -- which languages ship (``enabled = on|off``),
  and the metadata that lands in each ``.slg``'s VERSIONINFO.

Their product is the (module x language) matrix the build iterates.

``load_languages()`` returns enabled languages only unless asked otherwise, so a
tool that forgets about the policy fails safe rather than spending translation
budget on a language nobody ships (feature 039, FR-012/FR-013).

The equivalent reader for the build scripts is ``src/vcxproj/read_languages.ps1``.
The duplication is across a language boundary, not within one -- batch cannot
read a UTF-8 file containing Cyrillic and CJK author names reliably.
"""

from __future__ import annotations

import sys
from dataclasses import dataclass
from pathlib import Path

from . import LANGUAGES_CFG, PLUGINS_CFG, REPO_ROOT, TRANSLATIONS_DIR

#: The main application module. Not a plugin, so it is not in plugins.cfg.
APP_MODULE = "salamand"


class ConfigError(RuntimeError):
    """Raised when a policy file is missing, malformed, or inconsistent."""


@dataclass(frozen=True)
class Language:
    """One shipped language -- a record in ``languages.cfg``."""

    folder: str
    langid: int
    display_name: str
    author: str
    web: str
    comment: str
    helpdir: str
    origin: str  # human | mixed | machine
    enabled: bool  # feature 039 -- "enabled = on|off"; off means not shipped

    @property
    def is_machine_assisted(self) -> bool:
        """True when the language is not fully human-reviewed.

        Drives ``SLGIncomplete``, which makes the product tell the user the
        translation is incomplete and where to help (spec FR-016).
        """
        return self.origin != "human"

    @property
    def directory(self) -> Path:
        return TRANSLATIONS_DIR / self.folder


@dataclass(frozen=True)
class Module:
    """One separately versioned unit owning its own UI text."""

    name: str
    is_app: bool

    @property
    def rh_path(self) -> Path:
        """The ``Include=`` file every ``.atp`` needs (``DataRH.Load`` gates on it)."""
        if self.is_app:
            return REPO_ROOT / "src" / "lang" / "lang.rh"
        return REPO_ROOT / "src" / "plugins" / self.name / "lang" / "lang.rh"

    def slg_dir(self, out_dir: Path) -> Path:
        if self.is_app:
            return out_dir / "lang"
        return out_dir / "plugins" / self.name / "lang"

    def owner_binary(self, out_dir: Path) -> Path:
        """The binary whose FILEVERSION each of this module's .slg files must match.

        A plugin binary sits inside its own output directory --
        ``plugins/<name>/<name>.spl`` -- not directly under ``plugins/``.
        """
        if self.is_app:
            return out_dir / "tandemcommander.exe"
        return out_dir / "plugins" / self.name / f"{self.name}.spl"


# ---------------------------------------------------------------------------
# Readers
# ---------------------------------------------------------------------------


def _strip_comment(line: str) -> str:
    return line.split("#", 1)[0].strip()


def load_languages(
    path: Path | None = None, include_disabled: bool = False
) -> list[Language]:
    """Parse ``languages.cfg`` and validate it against ``translations/``.

    Returns **enabled languages only** by default (feature 039, FR-012). The
    safe behaviour is the default on purpose: if this returned everything and
    each caller had to remember to filter, the policy would hold only until
    somebody adds a caller that forgets -- and the failure would be silent,
    spending translation budget on a language nobody ships.

    Pass ``include_disabled=True`` to get the whole registry. That is the
    FR-013 opt-in used from code; see ``rebrand.py``, which deliberately sweeps
    disabled languages too.

    Validation always covers **every** record, enabled or not: a malformed
    disabled section is still malformed, and disabling a language must not
    become a way to hide a broken record until the worst moment.
    """
    path = Path(path) if path else LANGUAGES_CFG
    if not path.is_file():
        raise ConfigError(f"language registry not found: {path}")

    languages: list[Language] = []
    current: str | None = None
    fields: dict[str, str] = {}
    required = (
        "langid", "display_name", "author", "web", "comment", "helpdir",
        "origin", "enabled",
    )

    def flush(line_number: int) -> None:
        nonlocal current, fields
        if current is None:
            return
        missing = [k for k in required if k not in fields]
        if missing:
            raise ConfigError(
                f"{path}: language [{current}] is missing {', '.join(missing)}"
            )
        try:
            langid = int(fields["langid"])
        except ValueError:
            raise ConfigError(
                f"{path}: language [{current}] has non-numeric langid {fields['langid']!r}"
            ) from None
        if fields["origin"] not in ("human", "mixed", "machine"):
            raise ConfigError(
                f"{path}: language [{current}] has unknown origin {fields['origin']!r} "
                "(expected human, mixed, or machine)"
            )
        if fields["enabled"] not in ("on", "off"):
            raise ConfigError(
                f"{path}: language [{current}] has enabled {fields['enabled']!r} "
                "(expected on or off)"
            )
        languages.append(
            Language(
                folder=current,
                langid=langid,
                display_name=fields["display_name"],
                author=fields["author"],
                web=fields["web"],
                comment=fields["comment"],
                helpdir=fields["helpdir"],
                origin=fields["origin"],
                enabled=fields["enabled"] == "on",
            )
        )
        current, fields = None, {}

    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = _strip_comment(raw)
        if not line:
            continue
        if line.startswith("[") and line.endswith("]"):
            flush(number)
            current = line[1:-1].strip()
            continue
        if current is None:
            raise ConfigError(f"{path}:{number}: value outside a [language] section")
        if "=" not in line:
            raise ConfigError(f"{path}:{number}: expected key = value -- got {raw!r}")
        key, value = line.split("=", 1)
        fields[key.strip()] = value.strip()
    flush(len(path.read_text(encoding="utf-8").splitlines()))

    if not languages:
        raise ConfigError(f"{path}: no languages defined")

    # Validate the whole registry first, then narrow to what ships.
    _validate_languages(languages, path)
    if include_disabled:
        return languages
    return [lang for lang in languages if lang.enabled]


def _validate_languages(languages: list[Language], path: Path) -> None:
    """Enforce unique LANGIDs and folder/registry agreement (spec T009)."""
    seen: dict[int, str] = {}
    for lang in languages:
        if lang.langid in seen:
            raise ConfigError(
                f"{path}: LANGID {lang.langid} used by both "
                f"[{seen[lang.langid]}] and [{lang.folder}] -- must be unique"
            )
        seen[lang.langid] = lang.folder

    configured = {lang.folder for lang in languages}
    if TRANSLATIONS_DIR.is_dir():
        on_disk = {
            p.name
            for p in TRANSLATIONS_DIR.iterdir()
            if p.is_dir() and not p.name.startswith(".")
        }
        missing_dir = sorted(configured - on_disk)
        if missing_dir:
            raise ConfigError(
                f"{path}: no translations directory for {', '.join(missing_dir)} "
                f"-- create translations/<folder>/ or remove the record"
            )
        unregistered = sorted(on_disk - configured)
        if unregistered:
            raise ConfigError(
                f"{path}: translations/{{{','.join(unregistered)}}} exist but are not "
                "registered -- add a record or delete the directory"
            )


def load_enabled_modules(path: Path | None = None) -> list[Module]:
    """Return the app module plus every plugin set to ``on`` in ``plugins.cfg``."""
    path = Path(path) if path else PLUGINS_CFG
    if not path.is_file():
        raise ConfigError(f"plugin policy not found: {path}")

    modules = [Module(name=APP_MODULE, is_app=True)]
    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = _strip_comment(raw)
        if not line:
            continue
        if "=" not in line:
            raise ConfigError(f"{path}:{number}: expected name=on|off -- got {raw!r}")
        name, value = (part.strip() for part in line.split("=", 1))
        value = value.lower()
        if value not in ("on", "off"):
            raise ConfigError(f"{path}:{number}: expected on or off -- got {value!r}")
        if value == "on":
            modules.append(Module(name=name.lower(), is_app=False))
    return modules


def matrix(
    languages: list[Language] | None = None, modules: list[Module] | None = None
):
    """Yield every ``(module, language)`` pair the build must produce."""
    languages = languages if languages is not None else load_languages()
    modules = modules if modules is not None else load_enabled_modules()
    for module in modules:
        for language in languages:
            yield module, language


def has_translation_source(module: Module, language: Language) -> bool:
    return (language.directory / f"{module.name}.slt").is_file()


def main(argv: list[str] | None = None) -> int:
    """Print the configured matrix -- a quick sanity check on the policy files.

    Shows the WHOLE registry with each language's state, since its job is to
    display the policy rather than act on it.
    """
    try:
        registry = load_languages(include_disabled=True)
        modules = load_enabled_modules()
    except ConfigError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    shipped = [lang for lang in registry if lang.enabled]
    print(f"{len(shipped)} of {len(registry)} languages enabled "
          f"x {len(modules)} modules "
          f"= {len(shipped) * len(modules)} language modules "
          f"(+{len(modules)} english)")
    print()
    print(f"{'language':<20} {'langid':>6}  {'origin':<8} {'state':<6} {'with source':>11}")
    print("-" * 57)
    for lang in registry:
        have = sum(1 for m in modules if has_translation_source(m, lang))
        state = "on" if lang.enabled else "off"
        print(f"{lang.folder:<20} {lang.langid:>6}  {lang.origin:<8} "
              f"{state:<6} {have:>7}/{len(modules)}")
    print()
    off = [lang.folder for lang in registry if not lang.enabled]
    if off:
        print(f"not shipped ({len(off)}): {', '.join(off)}")
        print("  their translation source is retained -- re-enabling costs nothing")
    missing = [
        m.name
        for m in modules
        if not any(has_translation_source(m, l) for l in registry)
    ]
    if missing:
        print(f"modules with no translation source at all: {', '.join(missing)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
