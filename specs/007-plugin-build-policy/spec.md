# Feature Specification: Plugin Build Policy — Remove Obsolete Plugins and Introduce a Build-Time Plugin Configuration

**Feature Branch**: `007-plugin-build-policy`
**Created**: 2026-07-16
**Status**: Draft
**Input**: User description: "Nacti specifikaci ze souboru ./features/plugins-policy.md" — the referenced file defines a plugin policy: remove 8 obsolete plugins entirely, introduce a configuration file in the repository root that is read by the build script (`build.cmd`) and controls which plugins are built and shipped, and create that file with all remaining plugins listed and 10 of them disabled.

## Clarifications

### Session 2026-07-16

- Q: What format should the plugin build configuration file use? → A: Plain line-oriented text file — exactly one `name=on` / `name=off` entry per plugin, `#` comment lines and blank lines allowed; directly parseable by the batch build script with no external tooling.
- Q: Does the configuration govern only the scripted build (`build.cmd`), or also solution builds in the Visual Studio IDE? → A: Only the scripted build. Disabled plugins stay in the solution and remain buildable manually from the IDE; the product output is defined by the scripted pipeline. (Removed plugins disappear from the solution entirely.)
- Q: When a plugin is switched off, when must its previously built files disappear from the build output? → A: On the very next `build.cmd` run of any flavor (including incremental) — every run reconciles the output plugins folder and registration list with the configuration, so stale binaries of disabled plugins never linger.
- Q: What happens when a plugin directory exists in the repository but has no entry in the configuration file? → A: Hard error — the build stops before compilation, same as for an unknown or duplicate entry. The configuration must always list every plugin explicitly, so a newly added plugin forces a conscious policy decision.

## Problem Statement

Open Salamander currently carries 36 plugin directories, all of which
are built and shipped unconditionally. A maintenance analysis
(architecture/10) concluded that several plugins are obsolete beyond
repair — they target dead technologies (Windows Mobile/ActiveSync,
Internet Explorer's MSHTML, floppy-era archive formats) or depend on
proprietary components that cannot ship under GPLv2 (WinSCP's
Embarcadero RTL). Other plugins are of marginal value or are SDK
demos, and building them on every run wastes time and enlarges the
product without benefit.

Today there is **no way to control which plugins are part of a build**
short of editing project files by hand. This feature (1) removes the
permanently dead plugins from the repository, and (2) introduces a
single, version-controlled configuration file in the repository root
— read by the standard build script — that declares, for every
remaining plugin, whether it is built and included in the product.
The initially shipped configuration disables ten marginal/demo
plugins while keeping their source in the repository so any of them
can be brought back by editing one line.

### Plugin Disposition

**Removed entirely** (8 — source, projects, and all build/product references deleted):

| Plugin | What it was |
|----------|-------------------------------------------------------|
| pak | Quake PAK archives — read and write |
| unarj | ARJ archive extraction |
| unlha | LHA/LZH archive extraction |
| unfat | Viewing FAT12/16/32 floppy and disk images |
| wmobile | Windows Mobile/CE access via RAPI/ActiveSync |
| ieviewer | HTML/XML/Markdown viewer built on Internet Explorer (MSHTML) |
| splitcbn | Splitting a file into parts and rejoining them |
| winscp | SFTP/SCP client built on WinSCP (already unbuildable — proprietary runtime not in repo) |

**Kept in repository but disabled in the default build configuration** (10):

| Plugin | What it is |
|------------|---------------------------------------------------|
| unchm | CHM help-file extraction |
| unmime | MIME/e-mail message decoding |
| unole | OLE compound-document viewing |
| mmviewer | Multimedia viewer |
| nethood | Network Neighborhood browsing |
| automation | Scripting/automation host |
| checkver | Check-for-new-version utility |
| demoplug | SDK demo plugin |
| demoview | SDK demo viewer |
| demomenu | SDK demo menu extension |

**Kept and enabled in the default build configuration** (18):
7zip, checksum, dbviewer, diskmap, filecomp, folders, ftp, peviewer,
pictview, portables, regedt, renamer, tar, uncab, undelete, uniso,
unrar, zip.

(8 removed + 10 disabled + 18 enabled = all 36 current plugin
directories; the `shared` directory is build infrastructure, not a
plugin, and is unaffected.)

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Obsolete plugins are gone from the product (Priority: P1)

A maintainer checks out the repository after this change. The eight
removed plugins no longer exist anywhere: no source directories, no
build projects, no entries in the solution or build scripts, no
references in installer data, translations, or project documentation.
A full build completes successfully and its output contains no trace
of them.

**Why this priority**: This is the core cleanup decision already made
by the maintenance analysis. It permanently reduces maintenance
surface and removes components that are unshippable (license) or
target dead platforms. It is valuable on its own even if nothing else
in this feature is delivered.

**Independent Test**: Delete-verify cycle — search the repository for
each removed plugin's name and confirm no functional references
remain; run a full clean build and confirm it succeeds and the output
plugins folder contains none of the eight.

**Acceptance Scenarios**:

1. **Given** a clean checkout of this feature, **When** the repository is searched for the eight removed plugin names, **Then** no source directories, build projects, or build-system references to them are found (historical mentions in changelogs/git history are acceptable).
2. **Given** a clean checkout, **When** a full build is run, **Then** the build succeeds and the output contains no binaries, language files, or registration entries for any of the eight removed plugins.
3. **Given** the built application, **When** the user opens the plugin manager, **Then** none of the eight removed plugins are listed or offered for installation.

---

### User Story 2 - One configuration file controls which plugins are built (Priority: P2)

A maintainer opens a single, human-readable configuration file in the
repository root. Every remaining plugin appears there exactly once
with an explicit enabled/disabled state and a short description. When
the standard build script runs, it reads this file and builds and
ships only the enabled plugins — disabled ones are not compiled, do
not appear in the output plugins folder, and are not registered for
the application to load.

**Why this priority**: This is the lasting mechanism the feature
introduces — it turns plugin membership from a hard-coded property of
the build into reviewable, version-controlled policy. It depends on
nothing in Story 1 but delivers the tooling Story 3's policy needs.

**Independent Test**: With the configuration mechanism in place,
disable an arbitrary plugin, run the build, and confirm it is neither
compiled nor present in the output; re-enable it and confirm it
returns.

**Acceptance Scenarios**:

1. **Given** the configuration file lists a plugin as enabled, **When** the build script runs, **Then** that plugin is compiled and its binary and language file appear in the build output, and it is registered so the application picks it up automatically.
2. **Given** the configuration file lists a plugin as disabled, **When** the build script runs, **Then** that plugin is not compiled, none of its files appear in the build output, and it is not registered for automatic pickup.
3. **Given** a maintainer flips one plugin's state in the configuration and changes nothing else, **When** the build script runs again, **Then** the output reflects the new state without any other modification to the repository.
4. **Given** any build flavor offered by the build script (incremental, rebuild, full, release), **When** it runs, **Then** the same configuration file governs which plugins are built.

---

### User Story 3 - The agreed default policy ships with the repository (Priority: P3)

A developer builds the project out of the box, without touching any
configuration. The build produces exactly the 18 enabled plugins; the
10 marginal/demo plugins are skipped. The application starts cleanly,
its plugin manager lists exactly the 18 enabled plugins, and no
error dialogs about missing or unloadable plugins appear.

**Why this priority**: This is the concrete policy the project has
decided to ship. It depends on the mechanism from Story 2 but is a
separately verifiable deliverable — the checked-in file with the
agreed content.

**Independent Test**: Clean checkout, run the full build with no
local changes, count the plugins in the output and in the running
application's plugin manager: exactly the 18 enabled ones.

**Acceptance Scenarios**:

1. **Given** a clean checkout, **When** the repository-root configuration file is inspected, **Then** it contains an entry for every one of the 28 remaining plugins, with exactly the 10 listed plugins disabled and the other 18 enabled.
2. **Given** a clean checkout, **When** a full build runs with the default configuration, **Then** the output plugins folder contains exactly the 18 enabled plugins (each with its binary and language file) and the registration list contains exactly those 18.
3. **Given** the freshly built application, **When** it starts, **Then** no plugin-related error dialog appears and the plugin manager lists exactly the 18 enabled plugins.

---

### User Story 4 - Configuration problems are reported clearly (Priority: P4)

A maintainer makes a mistake in the configuration file — a typo in a
plugin name, a duplicate entry, a missing file. The build stops
before compiling anything, with a message that names the file and the
offending entry, so the mistake is fixed in seconds instead of being
silently mis-built.

**Why this priority**: Guard rails for the new mechanism. Valuable,
but only after the mechanism (Story 2) exists.

**Independent Test**: Introduce each error class into a scratch copy
of the configuration, run the build, and confirm each produces a
clear, early, actionable failure as specified below.

**Acceptance Scenarios**:

1. **Given** the configuration file is missing from the repository root, **When** the build script runs, **Then** it stops before compiling anything with a message naming the expected file and its purpose.
2. **Given** the configuration contains a name that matches no plugin directory, **When** the build script runs, **Then** it stops with a message naming the unknown entry.
3. **Given** the configuration contains the same plugin twice, **When** the build script runs, **Then** it stops with a message naming the duplicated entry.
4. **Given** a plugin directory exists in the repository but has no entry in the configuration, **When** the build script runs, **Then** it stops before compiling anything with a message naming the unlisted plugin.
5. **Given** an entry whose name differs from the plugin directory only in letter case, **When** the build script runs, **Then** the entry matches (names are matched case-insensitively).

---

### Edge Cases

- **All plugins disabled**: the build must still succeed and produce a working core application with an empty plugin set.
- **Previously built output**: switching a plugin from enabled to disabled must not leave its stale binary in the build output where the application would still pick it up — every `build.cmd` run of any flavor reconciles the output and registration with the current configuration.
- **Existing developer installations**: a developer who previously ran a build containing a now-removed or now-disabled plugin starts the new build; the application must start without error dialogs caused by the plugin's absence, and the absent plugin must simply no longer be offered.
- **Comment and blank lines** in the configuration file must be ignored, so maintainers can annotate the policy.
- **Cross-dependencies**: if any enabled product function turns out to depend on a removed or disabled plugin, the build or the specification of that function must surface this explicitly rather than failing silently at runtime (none are known — plugins are designed to be independent).
- **Language modules**: a plugin's language resource is built and shipped if and only if the plugin itself is; no orphaned language files may appear in the output.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The eight plugins pak, unarj, unlha, unfat, wmobile, ieviewer, splitcbn, and winscp MUST be completely removed from the repository: their source trees, build projects and language-module projects, solution membership, build-script references, installer/setup data, translation sources, and help/documentation references.
- **FR-002**: After the removal, every standard build flavor MUST complete successfully, and no build output (binaries, language files, registration data) may contain or reference any removed plugin.
- **FR-003**: A plugin build configuration file MUST exist in the repository root, under version control, as a plain line-oriented text file: exactly one `name=on` or `name=off` entry per line, with `#` comment lines and blank lines ignored, readable and editable in any text editor.
- **FR-004**: The configuration file MUST contain exactly one entry per remaining plugin (28 entries), each carrying the plugin's name and an explicit enabled/disabled state; entry names MUST match plugin directory names case-insensitively.
- **FR-005**: The standard build script MUST read the configuration file at the start of every run, in all its build flavors (incremental, rebuild, full, release), and MUST compile only the plugins marked enabled.
- **FR-006**: All product outputs of the build MUST honor the configuration: the output plugins folder, the shipped language files, and the plugin registration list generated for automatic pickup MUST include exactly the enabled plugins and nothing else. Every `build.cmd` run of any flavor MUST reconcile these outputs with the configuration, removing previously built files of now-disabled plugins.
- **FR-007**: Disabled plugins' sources MUST remain intact in the repository, and re-enabling a disabled plugin MUST require nothing more than changing its state in the configuration file and rebuilding.
- **FR-008**: The initially committed configuration MUST disable exactly these ten plugins — unchm, unmime, unole, mmviewer, nethood, automation, checkver, demoplug, demoview, demomenu — and enable the remaining eighteen.
- **FR-009**: The build MUST stop before any compilation, with an actionable message naming the file and the offending entry, when the configuration file is missing, contains an entry matching no plugin directory, contains a duplicate entry, or is syntactically unreadable.
- **FR-010**: A plugin directory present in the repository but absent from the configuration MUST stop the build before any compilation, with a message naming the unlisted plugin — the configuration must always cover the full plugin set explicitly.
- **FR-011**: The application built under any valid configuration MUST start and run correctly: no hard dependency on a removed or disabled plugin may remain in the core product, and its plugin manager MUST offer exactly the plugins present in that build.
- **FR-012**: Project documentation that describes the plugin set (plugin counts, plugin catalog, build instructions) MUST be updated to reflect the new plugin disposition and the existence and use of the configuration file.

### Key Entities

- **Plugin build configuration file**: the single repository-root policy document; a plain line-oriented text file with one `name=on|off` entry per plugin, plus `#` comments and blank lines; the sole authority on plugin membership for the build.
- **Plugin**: an optional product component identified by its directory name; owns a source tree, a build project, a language module, and (when built) a binary plus language file in the output and an entry in the registration list.
- **Removed plugin set**: the eight plugins permanently deleted from the repository (see Problem Statement table).
- **Disabled plugin set**: plugins whose entry in the configuration is set to disabled; source retained, nothing built or shipped.
- **Plugin registration list**: the build-generated list that the application reads to discover and auto-register available plugins; must always mirror the enabled set.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A full clean build from a fresh checkout succeeds with the default configuration and its output contains exactly 18 plugins — none of the 8 removed and none of the 10 disabled ones.
- **SC-002**: A repository-wide search for each of the eight removed plugin names finds zero functional references (source, projects, build scripts, installer data, documentation); only historical material (e.g., changelogs) may mention them.
- **SC-003**: Toggling any single plugin on or off requires editing exactly one line in one file, and the very next build run — of any flavor — reflects the change in the output with no other repository modification.
- **SC-004**: Every configuration error class listed in FR-009 and FR-010 (missing file, unknown entry, duplicate entry, unreadable syntax, unlisted plugin) stops the build before any compilation starts, and the error message names the file and offending entry — verified for all five classes.
- **SC-005**: The freshly built application starts with zero plugin-related error dialogs, and its plugin manager lists exactly the 18 enabled plugins.
- **SC-006**: 100% of the build flavors documented for the project (incremental, rebuild, full, release) honor the configuration file, verified by building each flavor with a plugin toggled off.

## Assumptions

- The repository currently contains 36 plugin directories (the `shared` directory is common build infrastructure, not a plugin). After removing 8, the configuration covers the remaining 28.
- winscp has no buildable project in the repository (its proprietary runtime was never included), so its removal is pure repository cleanup and cannot break the build.
- "Complete removal" means removal from the current tree; git history retains the deleted code, so no archival copies or tombstone files are needed.
- The configuration file governs the scripted build pipeline (`build.cmd` and the scripts it drives) — confirmed in Clarifications. Building disabled plugins directly from the IDE remains possible for developers regardless of the configuration; the shipped/product output is defined by the scripted build. Removed plugins, however, disappear from the solution entirely.
- The file syntax is decided (see Clarifications): plain line-oriented `name=on|off` entries with `#` comments and blank lines. Only the concrete file name (e.g., `plugins.cfg`) is chosen during planning.
- Unlisted-plugin handling is a hard build error (FR-010, confirmed in Clarifications) so that the checked-in policy file remains the single explicit authority on what ships and a newly added plugin cannot silently stay out of the product.
- The ten disabled plugins are expected to build correctly today; disabling them is a policy decision (marginal value, demo status), not a workaround for build breakage, and any of them may be re-enabled at any time.
- No runtime/user-facing configuration UI is in scope — the policy is a build-time, maintainer-facing mechanism.
- Existing user data or settings referencing removed plugins are tolerated: the application already handles absent plugins by not offering them; this feature must not introduce new error dialogs for their absence.
