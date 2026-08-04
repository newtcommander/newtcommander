# Tandem Commander

Tandem Commander is a fast, keyboard-friendly two-panel file manager for Windows. It is based on [Open Salamander](https://github.com/OpenSalamander/salamander), the GPLv2 open-source release of the long-lived Altap Salamander file manager. Everything about the original project — its history, features, documentation, and community — lives in the upstream repository; this README covers what makes Tandem Commander different and how to build it.

## A New Era of Development

Tandem Commander explores what happens when a mature, quarter-century-old C++ codebase meets the new era of agentic programming. Development follows Spec-Driven Development principles built on [GitHub SpecKit](https://github.com/github/spec-kit): every change begins as a written specification that is clarified, planned, and decomposed into tasks before any code is touched. The implementation itself is carried out by a combination of agentic coding frameworks using the best models available at the time — currently Anthropic Fable 5.

> **A note on naming**: since version 0.1.0 the application itself carries the Tandem Commander identity — the binary is `tandemcommander.exe`, the window titles, About dialog, and icons use the new name and visual style, and configuration lives under its own registry root (`HKCU\Software\Tandem Commander`), fully separate from any Open Salamander installation. Source files, internal identifiers, and the solution name (`salamand.sln`) intentionally keep their upstream names. The HTML help is not yet rebranded.

**Website**: [tandemcommander.org](https://tandemcommander.org) · **Issues**: [github.com/tandemcommander/tandemcommander/issues](https://github.com/tandemcommander/tandemcommander/issues)

## Building

### Prerequisites

- Windows 11 or newer
- [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) (any edition) with the **Desktop development with C++** workload
- Windows 10/11 SDK (the projects use the latest SDK installed with the workload)
- Optional: the `OPENSAL_BUILD_DIR` environment variable to choose the build output directory — the value must end with a backslash (e.g. `D:\Build\TandemCommander\`). When unset, the build defaults to `.\build\` under the repository root.
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

## Release, Code Signing & Installer

Release builds are code-signed and packaged **strictly on demand** — a plain
`build.cmd full release` never signs anything, never contacts a timestamp
server, and behaves exactly like a development build. Signing and installer
packaging are extra arguments:

```batch
build.cmd full release sign          :: complete Release build, then sign every
                                     ::   shipped binary (exe, dll, spl, slg)
build.cmd full release sign setup    :: one-command signed release: signed build
                                     ::   + signed Inno Setup installer
setup\build_setup.cmd                :: unsigned installer from an existing
                                     ::   Release tree (development test)
setup\build_setup.cmd sign           :: sign the Release tree if needed, then
                                     ::   build a signed installer + uninstaller
```

### Additional prerequisites for releases

- The maintainer's code-signing certificate installed in the Windows
  certificate store (current user, `My`); its SHA-1 thumbprint and the
  timestamp authority are committed in
  [`tools/codesign/codesign.cfg`](tools/codesign/codesign.cfg) — the private
  key never enters the repository
- [Inno Setup 7](https://jrsoftware.org/isinfo.php) for the installer
  (`ISCC.exe` is located automatically; it does not need to be on `PATH`)

### How signing works

The signing core is `tools\codesign\sign_release.ps1` (Windows PowerShell
5.1). It sweeps the Release output tree, signs every PE artifact
(`*.exe`, `*.dll`, `*.spl`, `*.slg`) with the configured certificate —
SHA-256 digests, RFC 3161 timestamp — and ends with a verification pass. The
sweep is **idempotent**: files already signed by the configured certificate
are skipped, so re-running after a network hiccup only finishes what is
missing, and a re-run over a fully signed tree completes in seconds. Files
signed by an *older* certificate are re-signed automatically.

```batch
:: sign an existing build without rebuilding:
powershell -NoProfile -ExecutionPolicy Bypass -File tools\codesign\sign_release.ps1 ^
    -Root "build\tandemcommander\Release_x64"

:: audit signing state without modifying anything:
powershell -NoProfile -ExecutionPolicy Bypass -File tools\codesign\sign_release.ps1 ^
    -Root "build\tandemcommander\Release_x64" -VerifyOnly
```

The signed installer is produced by `setup\build_setup.cmd sign`, which first
runs the same sweep (a signed installer can never package unsigned binaries)
and then compiles `setup\tandemcommander.iss` with `/DSIGN=1`, so Inno Setup
signs both the installer and the uninstaller it deploys. The result lands in
`setup\output\`.

Release output trees contain only distribution files: linker byproducts
(`.pdb`, `.lib`, `.exp`) are redirected outside the tree at build time (PDBs
are preserved under the `obj\` intermediate root for crash-dump
symbolication), and the installer excludes those file types independently as
a safety net.

### Certificate rotation

1. Install the new certificate into the Windows certificate store.
2. Update `thumbprint` in `tools\codesign\codesign.cfg` (one line).
3. Re-run any signing command — every artifact still carrying the old
   certificate is re-signed; timestamps keep previously released binaries
   valid after the old certificate expires.

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

Tandem Commander, like the Open Salamander project it derives from, is open-source software licensed under [GPLv2](doc/license/license_gpl.txt) and later. Individual files and libraries carry [different but compatible licenses](doc/third_party.txt). Contributors are listed in [AUTHORS](AUTHORS).
