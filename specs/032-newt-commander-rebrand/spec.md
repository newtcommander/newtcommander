# Feature Specification: Newt Commander Application Rebrand

**Feature Branch**: `032-newt-commander-rebrand`
**Created**: 2026-07-23
**Status**: Draft
**Input**: User description: "Cílem nového rozšíření je oddělení projektu od původního open source projektu Open Salamander a přejmenování projektu na Newt Commander. Nový binární soubor pro spuštění hlavní aplikace se bude jmenovat newtcommander.exe, aplikace nebude již zpětně kompatibilní s původním Open Salamanderem, nebude používat stejné záznamy v registrech, nebude používat stejné pojmenování pro nastavení, která by mohla být sdílena s původním projektem. Zásadním pravidlem je, že od této verze budou všechny úpravy, rozšíření a nové funkce realizovány jako vývoj nové verze open source aplikace Newt Commander. V adresáři ./temp/visual_style/ je struktura popisující nový vizuální styl aplikace, tedy především nová ikona v ./temp/visual_style/icon/newt-commander-icon.svg kterou bude nahrazena hlavní ikona exe aplikace a ostatní se musí projevit jako úprava všude kde se zobrazuje vizuální styl, např. pop-up okno About App. Součástí tohoto rozšíření ale není přejmenování zdrojových souborů ani funkcí nebo metod, jedná se o změnu názvu a loga a designu projektu 'navenek' — jak vizuálně pro koncového uživatele, tak pro systém — z pohledu registrů, nastavení atd. Nový projekt je tedy 'Newt Commander', pro exe soubor newtcommander.exe, aktuálně nastavíme jako verzi 0.1.0."

> **Decisions**: A detailed decision questionnaire for this phase lives in
> [questionnaire.md](questionnaire.md). The critical questions Q1–Q3 were resolved
> in the clarification session below; all remaining questionnaire items carry
> documented defaults (see Assumptions) that apply unless overridden.

## Clarifications

### Session 2026-07-23

- Q: Official project web resources (replacing vendor links)? → A: Main website `https://newtcommander.org`; source repository `https://github.com/newtcommander/newtcommander`. The project is open source.
- Q: Copyright attribution pattern in the program (About, file metadata)? → A: Years up to 2026 remain credited to "Open Salamander Authors"; from 2026 onward the credit is "Newt Commander Authors" (i.e., "© 1997–2026 Open Salamander Authors · © 2026 Newt Commander Authors").
- Q: Copyright of plugins authored in this fork? → A: Newly created plugins (SFTP, Markdown Viewer) carry solely "© Newt Commander Authors"; PictView was modified by this project and carries the dual attribution; all other inherited components follow the general year-split pattern.
- Q: First-run behavior on a machine with existing Open Salamander configuration? → A: Strictly fresh start — Newt Commander starts with default settings, offers no import of Open Salamander configuration, and the legacy configuration auto-import mechanism is removed.
- Q: Crash report destination? → A: Upload disabled entirely — crash dumps are stored locally only (under the Newt Commander application-data folder); no network transmission. Users may attach dumps to GitHub issues manually.
- Q: Scope of installer, HTML help, and non-English translations? → A: All deferred to follow-up features. This feature rebrands only the English application resources; it must ensure no Open Salamander-branded installer, help, or translation artifact ships with the default build.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - User Sees a Newt Commander Application (Priority: P1)

A user launches `newtcommander.exe`. Everything they see identifies the product as **Newt Commander, version 0.1.0**: the main window title, taskbar entry, tray icon tooltip, message-box captions, progress windows, the splash screen, the About dialog, and the file's properties in Windows Explorer (icon, product name, version, description). The names "Open Salamander", "Altap Salamander", or "Servant Salamander" never appear as the product's own name — only as an attribution to the upstream project where licensing requires it (About dialog, copyright notices).

**Why this priority**: This is the essence of the rebrand — the product's outward identity. Feature 030 promised (README FR-015) that the application-level rebrand would follow; this story delivers it.

**Independent Test**: Build and launch the application on a clean Windows machine, walk through a standard session (browse, copy, open About, trigger an error message), and inspect the exe in Explorer. Every visible product-name occurrence reads "Newt Commander"; the reported version is 0.1.0.

**Acceptance Scenarios**:

1. **Given** a built application, **When** the user inspects the binary in Windows Explorer, **Then** the file is named `newtcommander.exe`, shows the new Newt Commander icon, and its Details tab reports product name "Newt Commander" and version 0.1.0.
2. **Given** a running application, **When** the user looks at the main window title, taskbar, Alt-Tab switcher, and tray tooltip, **Then** all show the Newt Commander name and icon.
3. **Given** a running application, **When** the user opens Help → About, **Then** the dialog shows the new visual identity (icon/lockup per the visual-style package), the name "Newt Commander", version 0.1.0, and a copyright line that credits both the Newt Commander project and the upstream Open Salamander authors.
4. **Given** any error or confirmation dialog raised by the application, **When** the user reads its caption, **Then** the caption says "Newt Commander" (with version where the original showed one), never "Open Salamander".

---

### User Story 2 - Newt Commander Is a Separate Product for the System (Priority: P2)

A user who also has the original Open Salamander installed runs both applications, side by side or alternately. The two products never interfere: Newt Commander stores its configuration under its own registry location, never reads or writes the original's registry entries, uses its own inter-process identifiers (single-instance detection, configuration-save locking, shell-integration channels), and keeps its crash data in its own folder. Uninstalling or reconfiguring one product has no effect on the other.

**Why this priority**: "No backward compatibility, no shared registry entries, no shared settings naming" is the explicit separation mandate. Without it, the rebrand would be cosmetic and the two products could corrupt each other's state.

**Independent Test**: On a machine with an existing Open Salamander 5.0 installation and saved configuration, install and run Newt Commander, change its settings, exit, and verify (a) Open Salamander's registry data is byte-for-byte untouched, (b) Newt Commander's settings live under a Newt Commander registry location, and (c) both applications can run at the same time with fully working single-instance behavior each.

**Acceptance Scenarios**:

1. **Given** a machine with Open Salamander configuration in the registry, **When** Newt Commander runs and saves its settings, **Then** all its data lands under a registry location named after Newt Commander and no key under the original product's locations ("Open Salamander", "Altap", "Salamander") is created, modified, or deleted.
2. **Given** Open Salamander is already running, **When** the user starts Newt Commander (and vice versa), **Then** the second application starts as its own product — neither activates the other's window nor is blocked by the other's single-instance mechanism.
3. **Given** both products have shell integration enabled, **When** the user uses Explorer copy/paste integration or context menus, **Then** each product's shell integration operates independently under its own registered identity.
4. **Given** the application crashes, **When** the crash reporter runs, **Then** it identifies itself as Newt Commander's reporter, stores dumps under a Newt Commander folder, and no crash data is sent to the original vendor's servers.

---

### User Story 3 - New Visual Identity Everywhere (Priority: P3)

A user encounters the new "Split Disc — Extruded" visual identity consistently: the application icon (in Explorer, taskbar, window caption, Alt-Tab) renders crisply at every size Windows uses, following the size guidance of the visual-style package (full detail at large sizes, simplified at small sizes); the About dialog and splash screen use the new logo/lockup artwork adapted to the application's light and dark themes; tray status icon variants follow the new style.

**Why this priority**: The visual identity completes the rebrand but depends on the identity (US1) being in place; a beautiful icon on a product still calling itself Open Salamander would be incoherent.

**Independent Test**: Inspect the icon at 16/24/32/48/256 px in Explorer and the taskbar, open About and the splash screen in both light and dark theme, and compare against `temp/visual_style/` guidance.

**Acceptance Scenarios**:

1. **Given** the built executable, **When** its icon renders at any standard Windows size (16–256 px), **Then** it is the new Newt Commander icon, legible and without scaling artifacts, using the size-appropriate variant defined by the visual-style package.
2. **Given** the About dialog or splash screen, **When** displayed in light or dark application theme, **Then** the new logo/lockup artwork appears in the variant appropriate for that background.
3. **Given** the tray icon feature is enabled, **When** the application shows its tray states, **Then** the status icons derive from the new visual identity while preserving the existing state color distinctions.

---

### User Story 4 - The Project Develops Under the Newt Commander Identity (Priority: P4)

A contributor (human or agentic) working on any future feature finds the project's governing documents aligned with the new identity: the project constitution and project documentation state that from version 0.1.0 onward, all modifications, extensions, and new functionality are developed as the open-source application Newt Commander, and that compatibility guarantees are now anchored to Newt Commander's own baseline — the break with Open Salamander 5.0 is a deliberate, documented, one-time decision.

**Why this priority**: The user names this the "fundamental rule" of the feature, but it constrains future work rather than the shipped binary; it can land last.

**Independent Test**: Read the constitution and project docs after the feature: they identify the product as Newt Commander, record the compatibility break with justification, and define the go-forward baseline.

**Acceptance Scenarios**:

1. **Given** the project constitution, **When** a contributor consults its compatibility principle, **Then** it defines Newt Commander 0.1.0 as the new baseline and records the deliberate, documented break with Open Salamander 5.0.
2. **Given** the repository documentation (README, CLAUDE.md project context), **When** a reader checks the rebrand status, **Then** the "application still carries Open Salamander branding" caveat from feature 030 is removed/updated to reflect the completed application-level rebrand.

---

### Edge Cases

- **Existing Open Salamander configuration present**: The original application auto-imports configuration from a chain of legacy registry locations. Newt Commander's first run ignores that configuration entirely — it starts with defaults (per clarification Q1) and must never present the original product's configuration locations as its own.
- **Both products running simultaneously**: Single-instance detection, configuration-save locking, and shell-integration shared channels are name-based; identical names would make the products interfere (activate each other's window, serialize each other's registry writes, or share paste buffers). All such names must differ.
- **Shell integration collision**: The original shell extension registers under a product-specific identity. If Newt Commander registered the same identity, installing/uninstalling either product would break the other's Explorer integration.
- **Crash on a user's machine**: The current crash reporter uploads reports to the original vendor's server. In Newt Commander, uploading is disabled entirely (per clarification Q2) — crash dumps stay local, so a crash must never trigger any network transmission.
- **Small icon sizes**: The visual-style package prescribes simplified icon variants below 48 px and a favicon-style variant at 16 px; several of the files its documentation references are not present in the package and must be produced during implementation.
- **Plugin load failure message**: The message telling the user a plugin requires a newer application version currently names "Open Salamander 5.0 build 184"; after the rebrand it must name Newt Commander and its versioning without breaking plugin binary compatibility.
- **Version number goes "backward"**: The product version changes from 5.0 to 0.1.0. Any logic or user expectation comparing versions across the two products is void by design — the products are separate; nothing in Newt Commander may interpret Open Salamander versions as its own lineage.
- **Live text in logo artwork**: The lockup artwork uses a live font (Archivo) that end users will not have installed; artwork shipped in the product must not depend on fonts being present.

## Requirements *(mandatory)*

### Functional Requirements

**Product identity**

- **FR-001**: The main application binary MUST be named `newtcommander.exe`.
- **FR-002**: Every user-visible surface where the product names itself MUST say "Newt Commander": main window title, taskbar/Alt-Tab entries, tray tooltip, captions of message boxes and progress windows, splash screen, About dialog, and plugin-load failure messages. The strings "Open Salamander", "Altap Salamander", and "Servant Salamander" MUST NOT appear as the product's own name anywhere in the user interface.
- **FR-003**: The product version MUST be 0.1.0 and MUST be reported consistently in the About dialog, splash screen, and the executable's Windows file properties (product name, description, version, publisher, copyright).
- **FR-004**: The application's OS-facing identity (application manifest name and description) MUST identify the product as Newt Commander.
- **FR-005**: All product-branded auxiliary binaries shipped with the application (crash reporter, shell extensions) MUST carry Newt Commander file metadata (product name, description, copyright); their file names MAY remain unchanged.

**Visual identity**

- **FR-006**: The application icon MUST be replaced with the Newt Commander icon derived from `temp/visual_style/icon/newt-commander-icon.svg`, provided in all sizes Windows consumes (16–256 px), using the size-appropriate variants prescribed by the visual-style package (full ≥ 48 px, simplified 24–48 px, favicon-style ≤ 16 px). Missing raster/simplified variants MUST be produced as part of this feature following the package's guidance.
- **FR-007**: The About dialog and splash screen MUST use the new visual identity (icon/lockup artwork and palette from the visual-style package), with artwork variants appropriate to the application's light and dark themes, and MUST NOT depend on any font being installed on the user's machine.
- **FR-008**: Tray status icon variants MUST derive from the new visual identity while preserving the existing status color distinctions.

**System separation**

- **FR-009**: The application MUST store all configuration under a new registry location named after Newt Commander and MUST NOT read from, write to, enumerate, or delete any registry location of the original product family ("Open Salamander", "Altap", "Salamander" roots).
- **FR-010**: On first run — including on a machine with existing Open Salamander configuration — the application MUST start with default settings. No import of Open Salamander configuration is offered, and the legacy configuration auto-import mechanism (the chain of historical registry roots and its import UI) MUST be removed.
- **FR-011**: All name-based inter-process identifiers — single-instance detection objects, configuration-save locks, shared memory channels, window class names used for cross-process discovery — MUST use new Newt Commander-specific names so that Newt Commander and Open Salamander running on the same machine never detect, block, or signal each other.
- **FR-012**: The shell extension MUST register under a new identity (new class identifier, new registration name, new shared-channel names, rebranded description) so that both products' shell integration can be installed and uninstalled independently without affecting each other.
- **FR-013**: The crash reporter MUST identify itself as Newt Commander's reporter, store crash data locally under a Newt Commander-named application-data folder, and use Newt Commander registry keys. Crash-report uploading MUST be disabled entirely — the reporter MUST NOT transmit crash data over the network to any server (the original vendor's or otherwise); users may attach locally stored dumps to GitHub issues manually.
- **FR-014**: All links and web addresses presented in the user interface that point to the original vendor's resources (product website, forums, download pages) MUST be replaced with Newt Commander project resources — the main website `https://newtcommander.org` and the source repository `https://github.com/newtcommander/newtcommander` — or removed where no equivalent exists.

**Compatibility bounds**

- **FR-015**: The plugin interface MUST remain binary-compatible: plugins built from this repository before and after the rebrand MUST continue to load and run. Only user-visible identity (strings, messages, configuration storage location inherited from the application) changes.
- **FR-016**: Source file names, function/method/class names, internal code identifiers, developer-facing build artifacts (solution/project file names, build environment variable names), and code comments are explicitly OUT of scope and MUST NOT be renamed by this feature.
- **FR-017**: Copyright notices in the program (About dialog, splash screen, binary file metadata) MUST follow the year-split attribution rule: years up to 2026 are credited to "Open Salamander Authors" and years from 2026 onward to "Newt Commander Authors" (e.g., "© 1997–2026 Open Salamander Authors · © 2026 Newt Commander Authors"). All existing license texts remain intact.
- **FR-021**: Plugin copyright metadata MUST reflect actual authorship: plugins newly created in this project (SFTP, Markdown Viewer) carry solely "© Newt Commander Authors"; PictView, modified by this project, carries the dual year-split attribution; all other inherited plugins and components follow the general year-split rule of FR-017.
- **FR-018**: The installer sources, HTML help content, and non-English translations are explicitly DEFERRED to follow-up features. This feature rebrands the English application resources only, and MUST ensure that the default build ships no Open Salamander-branded installer, help, or translation artifact.

**Governance**

- **FR-019**: The project constitution and project documentation MUST be updated to reflect that from version 0.1.0 all development proceeds under the Newt Commander identity, with the compatibility baseline re-anchored to Newt Commander and the break with Open Salamander 5.0 recorded as a deliberate, documented decision.
- **FR-020**: The repository README's caveat that the built application still carries Open Salamander branding (feature 030, FR-015) MUST be updated once this feature completes.

### Key Entities

- **Product identity**: The set of user- and OS-visible attributes naming the product — display name, binary name, version, publisher, copyright, manifest identity, icons. Currently "Open Salamander 5.0" (build 184); becomes "Newt Commander 0.1.0".
- **Configuration root**: The registry location owning all user settings, including per-plugin configuration handed to plugins by the application. Currently a version-scoped key under the original product's name with an auto-import chain of legacy roots; becomes a Newt Commander-owned location with no ties to the legacy chain.
- **IPC namespace**: The family of name-based objects used for single-instance detection, configuration-save locking, shell-integration channels, and cross-process window discovery. Must be disjoint from the original product's namespace.
- **Shell integration identity**: The registered class identifier, registration name, and description under which Explorer integration operates. Must be new and independent.
- **Visual asset set**: The icon family (full/simplified/favicon variants, tray states) and logo/lockup artwork defined by `temp/visual_style/`, adapted for light/dark themes; the source of truth for the new look.
- **Version identity**: Marketing version 0.1.0 plus whatever internal build/config-version identifiers the product carries forward; plugin interface versioning is preserved unchanged.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In a standard session (launch, browse, copy, error dialog, About, exit), 100% of product self-references visible to the user read "Newt Commander" and 0 instances of "Open Salamander"/"Altap" appear outside attribution contexts.
- **SC-002**: On a machine with Open Salamander 5.0 installed and configured, running Newt Commander through a full configure-and-exit cycle causes zero changes to the original product's registry data, and both applications can run concurrently with correct independent single-instance behavior.
- **SC-003**: The shipped binary is `newtcommander.exe`, and Windows Explorer's file properties report product name "Newt Commander" and version 0.1.0.
- **SC-004**: The new icon renders as the application's icon in Explorer, taskbar, Alt-Tab, and window caption at all standard sizes (16, 24, 32, 48, 64, 128, 256 px) with the size-appropriate variant and no visible artifacts.
- **SC-005**: About dialog and splash screen display the new visual identity in both light and dark themes, with correct name, version 0.1.0, and the year-split copyright attribution ("© 1997–2026 Open Salamander Authors · © 2026 Newt Commander Authors").
- **SC-006**: Zero network transmissions to the original vendor's domains originate from the product (verifiable by exercising the crash reporter path).
- **SC-007**: All 18 default-enabled plugins load and function in the rebranded build with no plugin-side changes beyond the shared build inputs (no plugin interface change).
- **SC-008**: A contributor reading the constitution and README after the feature can state the product name, version baseline, and compatibility policy without encountering contradictory statements.

## Assumptions

- **Registry root naming**: A new root mirroring the existing vendor/version structure (e.g., a "Newt Commander" key with a version-scoped subkey) is assumed; exact naming is questionnaire item D01 and will be fixed in planning.
- **Publisher identity**: "Newt Commander Project" is assumed as publisher/company in file metadata (questionnaire D02); copyright follows the year-split attribution rule decided in clarification (D03 — resolved).
- **Canonical project URLs**: `https://newtcommander.org` (main website) and `https://github.com/newtcommander/newtcommander` (repository) replace vendor links (D04 — resolved by clarification).
- **Auxiliary binary file names** (crash reporter, shell extension DLLs, language files) keep their current file names; only their metadata and user-visible strings are rebranded (questionnaire D05). The main exe is the explicitly requested exception.
- **Internal versioning**: The internal build number continues monotonically (no reset) beneath the 0.1.0 marketing version; the plugin interface version is unchanged (questionnaire D07, D10).
- **English resources only**: The rebrand covers the English language resources built by default; the `translations/` tree, installer, and HTML help are out of scope (Q3 — resolved: deferred to follow-up features).
- **Missing visual assets** (simplified icon, favicon variant, PNG rasters, vertical lockups) referenced by the visual-style package's own documentation are produced during implementation from the provided SVGs, following the package's size guidance (questionnaire D14).
- **Text in shipped artwork** is converted to outlines or rasterized — no font installation is introduced (questionnaire D18).
- **Splash screen is retained** with the new identity rather than removed (questionnaire D19).
- **Shell extension support is retained** under a newly generated identity (questionnaire D13).
- **Developer-facing names are stable**: `OPENSAL_BUILD_DIR`, `salamand.sln`, project file names, and the `specs/` history remain untouched per FR-016 (questionnaire D24).
- **Constitution amendment is part of this feature** (FR-019), since the user designates the identity change as the project's fundamental go-forward rule (questionnaire D25).
