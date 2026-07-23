# Newt Commander

Newt Commander is a fast, keyboard-friendly two-panel file manager for Windows. It is based on [Open Salamander](https://github.com/OpenSalamander/salamander), the GPLv2 open-source release of the long-lived Altap Salamander file manager. Everything about the original project — its history, features, documentation, and community — lives in the upstream repository; this README covers what makes Newt Commander different and how to build it.

## A New Era of Development

Newt Commander explores what happens when a mature, quarter-century-old C++ codebase meets the new era of agentic programming. Development follows Spec-Driven Development principles built on [GitHub SpecKit](https://github.com/github/spec-kit): every change begins as a written specification that is clarified, planned, and decomposed into tasks before any code is touched. The implementation itself is carried out by a combination of agentic coding frameworks using the best models available at the time — currently Anthropic Fable 5.

> **A note on naming**: since version 0.1.0 the application itself carries the Newt Commander identity — the binary is `newtcommander.exe`, the window titles, About dialog, and icons use the new name and visual style, and configuration lives under its own registry root (`HKCU\Software\Newt Commander`), fully separate from any Open Salamander installation. Source files, internal identifiers, and the solution name (`salamand.sln`) intentionally keep their upstream names. The installer and HTML help are not yet rebranded.

**Website**: [newtcommander.org](https://newtcommander.org) · **Issues**: [github.com/newtcommander/newtcommander/issues](https://github.com/newtcommander/newtcommander/issues)

## Building

### Prerequisites

- Windows 11 or newer
- [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) (any edition) with the **Desktop development with C++** workload
- Windows 10/11 SDK (the projects use the latest SDK installed with the workload)
- Optional: the `OPENSAL_BUILD_DIR` environment variable to choose the build output directory — the value must end with a backslash (e.g. `D:\Build\NewtCommander\`). When unset, the build defaults to `.\build\` under the repository root.
- Optional: [Git](https://git-scm.com/downloads), and [PowerShell 7.4+](https://learn.microsoft.com/en-us/powershell/scripting/install/installing-powershell-on-windows) to run the `normalize.ps1` source formatter

### Build commands

Run `build.cmd` from the repository root:

```batch
build.cmd                :: incremental Debug x64 build
build.cmd rebuild        :: full clean + rebuild
build.cmd release        :: Release x64 build
build.cmd full           :: complete build: app + plugins + language modules,
                         ::   plus runtime data files and plugin registration
build.cmd full release   :: complete Release x64 build
```

Arguments can be combined in any order (`build.cmd help` shows the full usage). The set of plugins that is compiled and shipped is controlled by [`plugins.cfg`](plugins.cfg) in the repository root — one `name=on|off` line per plugin.

## Development Process

Features are developed one at a time through the SpecKit workflow: **specify → clarify → plan → tasks → implement**. Each feature lives in the [`specs/`](specs/) directory with its full paper trail — specification, implementation plan, task breakdown, research notes, and contracts — committed alongside the code, so the repository records not only what changed but why. Project-wide rules (build reproducibility, backward compatibility, incremental modernization, Windows platform commitment, plugin architecture preservation) are codified in the project constitution at `.specify/memory/constitution.md`.

## Repository Structure

| Directory | Purpose |
|-----------|---------|
| `src/` | C++ source code: core application, shared libraries (`src/common/`), plugins (`src/plugins/`) |
| `src/vcxproj/` | Visual Studio solution (`salamand.sln`) and project files |
| `specs/` | Spec-Driven Development artifacts: one directory per feature |
| `architecture/` | Architecture documentation: build pipeline, dependencies, plugin API |
| `convert/` | Character conversion tables |
| `doc/` | Licenses and third-party notices |
| `help/` | User manual source (HTML Help) |
| `tools/` | Build utilities |
| `translations/` | UI translations |

See the [`architecture/`](architecture/) documents for a much deeper analysis.

## License

Newt Commander, like the Open Salamander project it derives from, is open-source software licensed under [GPLv2](doc/license/license_gpl.txt) and later. Individual files and libraries carry [different but compatible licenses](doc/third_party.txt). Contributors are listed in [AUTHORS](AUTHORS).
