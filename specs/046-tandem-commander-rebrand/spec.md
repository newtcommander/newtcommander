# Feature Specification: Tandem Commander Rebrand

**Feature Branch**: `046-tandem-commander-rebrand`
**Created**: 2026-08-01
**Status**: Draft
**Input**: User description: "Cilem této úpravy je celkové přejmenování projektu z Newt Commander na Tandem Commander. Nejprve analyzuj vsechny soubory v adresari ./temp/tandem_design/, kde jsou nove vizualy a ikona. Nasledne detailne analyzuj cely kod, build systém a identifikuj všechna místa pro přejmenování. Prostě měníme celý projekt z názvu Newt Commander, na Tandem Commander, resp. exe a adresare z newtcommander.exe na tandemcommander.exe atd."

## Overview

The product currently ships as **Newt Commander** (binary `newtcommander.exe`,
version 0.1.0, internal build 184). This feature renames the entire product
identity to **Tandem Commander** (binary `tandemcommander.exe`) and applies the
new visual brand delivered in `temp/tandem_design/` (orange folder icon, light
and dark wordmark lockups, documented color palette and typography).

This is the same class of change as feature 032 (the Open Salamander → Newt
Commander rebrand) and follows the same ground rules: only user-visible and
OS-visible identity changes; upstream source/project names (`salamand.sln`,
`salamand.vcxproj`, `SALAMANDER_*` constants, file and class names) deliberately
stay untouched.

A repository-wide inventory found the old identity in **~392 tracked files**
(excluding historical `specs/` records), grouped into: central version-info
defines, hardcoded C++ strings, resource files, build system paths, the
installer script, 220 translation archives across 11 languages, brand asset
tooling, and documentation.

## Clarifications

### Session 2026-08-01

- Q: Na jaké cílové URL má rebrand převést webové a GitHub odkazy? → A:
  Mechanické zrcadlení — web `tandemcommander.org` (všechny varianty),
  GitHub `github.com/tandemcommander/tandemcommander` (vč. /issues,
  /releases); doména a GitHub organizace budou založeny.
- Q: Má Tandem Commander při prvním spuštění převzít existující konfiguraci
  Newt Commanderu z registru? → A: Ne — čistý start bez importu; klíč
  `Software\Newt Commander` zůstane nedotčen (precedens feature 032).
- Q: Mají se přejmenovat i čistě interní identifikátory odvozené od staré
  značky (`*_NEWT` makra, `NCDrawWordmark`, `NC_COLOR_*`, `NC*` prefixy)?
  → A: Ano — všechny brand-odvozené identifikátory (NEWT→TANDEM, NC→TC);
  upstream názvy (`SALAMANDER_*`, `salamand*`) zůstávají.
- Q: Která varianta ikony se má použít jako zdroj pro shipped .ico soubory
  (16–256 px)? → A: Full-bleed varianta (`tandem-commander-icon-full-*`)
  pro všechny velikosti; plain varianta se stínem slouží pro About/splash
  artwork.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Renamed application identity (Priority: P1)

A user launches the file manager and everywhere they look — window title,
taskbar, About dialog, splash screen, Task List window, Bug Reporter, file
properties of the executable — the product presents itself as "Tandem
Commander". The executable on disk is `tandemcommander.exe`.

**Why this priority**: The name is the product identity; every other part of
the rebrand (visuals, installer, translations) builds on it. Without it the
rename does not exist.

**Independent Test**: Build the application, run the produced executable, and
inspect the main window title, About dialog, splash screen and the executable's
version properties in Windows Explorer.

**Acceptance Scenarios**:

1. **Given** a fresh build, **When** the output folder is inspected, **Then**
   the main binary is named `tandemcommander.exe` and its version resource
   shows product name "Tandem Commander", company "Tandem Commander Project"
   and original filename "TANDEMCOMMANDER.EXE".
2. **Given** the application is running, **When** the user looks at the main
   window title, splash screen, About dialog and Task List window, **Then**
   every surface reads "Tandem Commander" and no surface reads "Newt
   Commander".
3. **Given** the application is running, **When** the user opens Help → About
   and clicks the website link, **Then** the link text and target use the
   Tandem Commander domain.
4. **Given** the crash reporter starts, **When** its window and version
   properties are inspected, **Then** it presents as "Tandem Commander Bug
   Reporter" and locates the host binary `tandemcommander.exe`.
5. **Given** the application writes to per-user locations, **When** the
   configuration and support folders are inspected, **Then** the registry root
   is `HKCU\Software\Tandem Commander\0.1` and application data folders are
   named "Tandem Commander".

---

### User Story 2 - New visual brand applied (Priority: P2)

The user sees the new Tandem Commander artwork: the orange folder icon on the
executable, in the taskbar, window caption and Explorer; the About dialog and
splash screen use the new artwork, palette and two-tone wordmark ("Tandem" in
navy/light, "Commander" in orange) per the delivered design.

**Why this priority**: The rename is publicly credible only with the matching
visual identity; the design assets are already delivered and waiting.

**Independent Test**: Build, then inspect the executable icon at multiple sizes
in Explorer, the window/taskbar icon, the splash screen and the About dialog.

**Acceptance Scenarios**:

1. **Given** a fresh build, **When** the executable is viewed in Explorer at
   16–256 px, **Then** the new orange folder icon (full-bleed variant) renders
   crisply and consistently at every size.
2. **Given** the application starts, **When** the splash screen and About
   dialog appear, **Then** they show the new artwork and the two-tone "Tandem
   Commander" wordmark using the delivered palette (dark and light backgrounds
   per the design rules).
3. **Given** the brand tooling is run on the updated sources, **When** its
   verification mode runs, **Then** all shipped icon files and the About/splash
   artwork are regenerated from the new design without manual pixel editing.

---

### User Story 3 - Build and installer produce Tandem Commander outputs (Priority: P3)

A developer runs the standard build and installer packaging; all output paths,
shortcuts and the setup package carry the new name, and installation lands in
"Tandem Commander" locations.

**Why this priority**: Developers and release packaging must produce consistent
artifacts, but this only matters once the binary itself is renamed.

**Independent Test**: Run `build.cmd full release`, then compile the installer
script and run the produced setup on a clean machine.

**Acceptance Scenarios**:

1. **Given** a full build, **When** the build tree is inspected, **Then**
   outputs live under a `tandemcommander\` build directory and the generated
   launcher shortcut is named for Tandem Commander.
2. **Given** the installer script is compiled, **When** the output is
   inspected, **Then** the package is named `tandemcommander-0.1.0-x64-setup`
   and installing it creates `Program Files\Tandem Commander`, Start menu and
   desktop entries named "Tandem Commander", with the new icon.
3. **Given** the product identity changed, **When** the installer is built,
   **Then** it uses a newly generated application identifier so it does not
   upgrade-in-place over a Newt Commander installation.
4. **Given** an incremental debug build after the rename, **When** it
   completes, **Then** intermediates and outputs land in the renamed build
   tree with no stray `newtcommander\` directories recreated.

---

### User Story 4 - Translations and documentation follow the rename (Priority: P4)

A user running any of the 11 translated languages sees "Tandem Commander"
(correctly inflected where the language declines the name), and a contributor
reading the repository documentation finds the new identity throughout.

**Why this priority**: Completes the rename across every shipped language and
the contributor-facing docs; depends on the English strings being final.

**Independent Test**: Switch the UI language to Czech (and spot-check the other
languages), open the About dialog and menus; read the root README and project
docs.

**Acceptance Scenarios**:

1. **Given** any shipped language, **When** UI strings containing the product
   name are displayed, **Then** they read "Tandem Commander", including
   inflected forms (e.g. Czech "Tandem Commanderu").
2. **Given** the translation registry and tooling, **When** they are inspected,
   **Then** the per-language website fields and tooling constants use the
   Tandem Commander identity.
3. **Given** the repository, **When** README, CLAUDE.md, AUTHORS, the project
   constitution and architecture docs are read, **Then** they describe Tandem
   Commander; historical feature specs under `specs/` remain unchanged as a
   paper trail.

---

### Edge Cases

- **Existing Newt Commander user configuration**: the new registry root means a
  previous Newt Commander configuration is not read. The application starts
  with fresh defaults; no import is offered (consistent with the feature 032
  one-time-break precedent — see Assumptions).
- **Old and new builds running simultaneously**: single-instance mutexes,
  shared-memory and event names are renamed, so an old Newt Commander build
  and a new Tandem Commander build treat each other as unrelated applications.
  This is accepted behavior.
- **Previously exported data files**: files whose text headers name the old
  product (exported FTP server types, checksum files, bug reports) must still
  be readable — if any import path validates such a header, it must accept
  files written under the old name.
- **Generated self-extracting archives**: SFX defaults (vendor, website) baked
  into newly created archives use the new identity; archives created earlier
  keep the old text and remain functional.
- **Icon legibility below 32 px**: the design ships two icon variants (with
  margin, and full-bleed) plus per-size renders; the full-bleed variant is the
  source for all shipped icon sizes (clarified 2026-08-01), and its per-size
  renders must be used (not downscales of the 1024 px master) so small sizes
  stay legible.
- **Stray build-system references**: build path renames must change together
  (output directories, intermediate-directory rewriting, exe-path property
  sheets), otherwise intermediates silently land in the shipped tree; a full
  clean rebuild must verify no `newtcommander` path is recreated.
- **Shell extension identity**: OS-visible IPC object names and the registry
  value the shell extension uses are renamed; the shell extension and the main
  application must be updated in the same release so they keep finding each
  other.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Every user-visible and OS-visible occurrence of the product name
  "Newt Commander" MUST become "Tandem Commander": window titles and class
  names, About dialog, splash screen, dialog captions, menu and string
  resources, message-box texts, version resources, manifests, crash reporter,
  Task List, and plugin descriptions/version resources (all 30 plugin version
  files and the shared version template).
- **FR-002**: The main binary MUST be produced as `tandemcommander.exe`, and
  every component that locates or names the binary MUST be updated (crash
  reporter host lookup, file-comparator remote launcher, translator restart
  process match, build verification scripts, usage/help strings referencing
  `NEWTCOMMANDER.EXE`).
- **FR-003**: The per-user configuration registry root MUST become
  `HKCU\Software\Tandem Commander\0.1` (and the Bug Reporter key
  `Software\Tandem Commander\Bug Reporter`), with all in-code references —
  including the literal substring matching used for configuration-root
  display — updated consistently. No values under `Software\Newt Commander`
  are read or written.
- **FR-004**: Per-user file-system locations named for the product (roaming
  application-data folder, bug-report path, WebView2 profile folder) MUST use
  "Tandem Commander".
- **FR-005**: All website and repository URLs MUST move to the Tandem
  Commander equivalents: `newtcommander.org` → `tandemcommander.org` (all
  scheme/`www` variants) and `github.com/newtcommander/newtcommander` →
  `github.com/tandemcommander/tandemcommander` (including `/issues` and
  `/releases` links), across code, resources, plugin home pages, translation
  archives and the language registry.
- **FR-006**: OS-visible named objects derived from the brand MUST be renamed
  to Tandem Commander equivalents: single-instance and registry mutexes,
  process-list shared memory and events, crash-reporter mutexes, shell
  extension shared-memory/mutex/event names and its registry value name,
  manifest assembly identities, and the scanner (TWAIN) application identity.
- **FR-007**: Copyright notices MUST keep their current holders and years
  unchanged ("Open Salamander Authors" for years up to 2026, Pavel Stupka from
  2026), and the holder MUST remain defined in exactly one place; only
  brand-derived identifier names around it may be renamed.
- **FR-008**: The build system MUST output to a `tandemcommander\` build
  directory tree: all output-directory property sheets, the
  intermediate-directory rewriting rule (which MUST change in lockstep, per
  its own KEEP IN SYNC warning), exe-path property sheets, the main project's
  target name, build/clean/populate/sign/help scripts, and the generated
  launcher shortcut.
- **FR-009**: The installer MUST be fully rebranded: script file renamed to
  `tandemcommander.iss`; application name, publisher URLs, default install
  directory `{autopf}\Tandem Commander`, shortcuts, source paths from the
  renamed build tree, output package name `tandemcommander-0.1.0-x64-setup`,
  and a newly generated AppId GUID so it is a distinct product identity.
- **FR-010**: The shipped brand assets MUST be regenerated from the delivered
  design in `temp/tandem_design/`: the master icon sources in the brand
  tooling directory replaced with the new **full-bleed** icon renders (all
  sizes, using the delivered per-size renders), all four shipped icon files
  (application, crash reporter, setup, uninstall) regenerated through the
  existing tooling, the About/splash artwork replaced (the margin/shadow
  variant serves as artwork source), and the reference vector files renamed
  to the new brand. The tooling's verification mode MUST pass afterwards.
- **FR-011**: The About dialog and splash screen wordmark MUST render "Tandem
  Commander" two-tone per the design ("Tandem" in the text color, "Commander"
  in brand orange), using the palette from the design README for dark and
  light backgrounds.
- **FR-012**: All 220 translation archives across the 11 languages, the
  language registry (per-language website fields and the machine-translation
  author credit), and the translation tooling constants (product, project,
  website, binary-name replacement targets, template paths, user-agent) MUST
  carry the new identity, preserving grammatical inflections in translated
  text (the name is replaced in place; language-specific suffixes remain
  attached).
- **FR-013**: Text headers written into user files (bug-report header,
  exported FTP server-type header, checksum-file comment, SFX archive
  vendor/website defaults) MUST use the new identity; any import path that
  checks such a header MUST continue to accept files produced under the old
  name.
- **FR-014**: Repository documentation MUST be updated to the new identity:
  root README, CLAUDE.md, AUTHORS, the project constitution and the
  architecture documents (including build-tree diagrams). Historical records
  under `specs/` MUST remain unchanged.
- **FR-015**: After the rename, a case-insensitive repository search for the
  old brand tokens ("Newt Commander", "newtcommander") in tracked files MUST
  return no hits outside `specs/` history — while upstream-derived,
  deliberately retained names (`salamand*`, `SALAMANDER_*`, salmon, shexreg,
  and incidental non-brand matches such as `newText`) remain untouched.
- **FR-016**: The product version MUST remain 0.1.0 (internal build 184); the
  rename introduces no version or plugin-interface change, and plugins built
  before the rename against interface version 105 MUST still load.
- **FR-017**: All internal brand-derived identifiers MUST be renamed alongside
  the strings: `NEWT`-suffixed/`NEWT`-bearing macro names (e.g. the
  copyright-holder and copyright macros), the wordmark drawing helper, the
  `NC_COLOR_*` palette constants and `NC`-prefixed IPC/identifier prefixes
  become their `TANDEM`/`TC` equivalents. Upstream-derived names
  (`SALAMANDER_*`, `salamand*`, salmon, shexreg) are explicitly excluded.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A repository-wide case-insensitive search for the old brand name
  in tracked files yields zero occurrences outside the historical `specs/`
  directory.
- **SC-002**: A clean full release build completes successfully and produces
  `tandemcommander.exe` (plus all 18 enabled plugins and 8 enabled languages)
  under the renamed build tree, with no `newtcommander` path recreated.
- **SC-003**: 100% of user-visible identity surfaces (window title, splash,
  About, Task List, Bug Reporter, executable properties, Explorer icon)
  present "Tandem Commander" with the new artwork on first launch.
- **SC-004**: The installer package installs to "Tandem Commander" locations
  with new-brand shortcuts and icons on a clean system, the installed
  application starts, and uninstall removes what was installed.
- **SC-005**: In every shipped language, the About dialog and spot-checked
  menu/dialog strings show the correctly inflected new name; the English and
  translated string sets contain no old-brand text.
- **SC-006**: Existing behaviors tied to renamed OS objects keep working in a
  single-build world: second launch activates the running instance, the shell
  extension communicates with the application, and the crash reporter
  attaches — verified manually after the rename.
- **SC-007**: Files exported by a pre-rename build (FTP server types) import
  successfully into the renamed build.

## Assumptions

- **Web and repository targets mirror the rename** *(confirmed in
  clarification)*: `tandemcommander.org` and
  `github.com/tandemcommander/tandemcommander` are the intended targets and
  will be registered; the rename maps URLs mechanically.
- **No configuration migration** *(confirmed in clarification)*: consistent
  with the feature 032 precedent (deliberate one-time break, no legacy import
  chain), the renamed application starts with fresh defaults and does not read
  `Software\Newt Commander`. The old Newt Commander installation, if present,
  is left untouched (separate AppId, separate Program Files directory).
- **Version and interface unchanged**: 0.1.0 / build 184 / plugin interface
  105 carry over; the rename is not a release bump.
- **Copyright unchanged**: the year-split rule and holder (Pavel Stupka, 2026+)
  are not affected by the rename; "Open Salamander Authors" notices stay.
- **Upstream names stay**: source files, functions, classes, project/solution
  names (`salamand.sln`, `salamand.vcxproj`, `SALAMANDER_*`, salmon, salspawn,
  shexreg CLSID) keep their upstream names — the same rule as feature 032. The
  shell-extension CLSID is not brand-derived and is not changed.
- **Design source of truth**: `temp/tandem_design/` (icon SVG + PNG renders at
  16–1024 px in two variants, light/dark lockups, DESIGN_README.md with
  palette, typography and usage rules) is final and needs no further design
  work; the full-bleed variant is the icon source (confirmed in
  clarification). The repository directory is already named `tandemcommander`.
- **Historical artifacts keep the old name**: `specs/030-*`, `specs/032-*` and
  other past specs, git history and old branches are records and stay as-is.
