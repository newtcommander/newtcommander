# Implementation Plan: Newt Commander Application Rebrand

**Branch**: `032-newt-commander-rebrand` | **Date**: 2026-07-23 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/032-newt-commander-rebrand/spec.md`

## Summary

Rebrand the built application from "Open Salamander 5.0" to **"Newt Commander 0.1.0"** on every user- and OS-visible surface while leaving source-code identifiers untouched: rename the binary to `newtcommander.exe` via a `TargetName` override, re-anchor all configuration to `HKCU\Software\Newt Commander\0.1` with the legacy import chain removed, rename all cross-process identifiers (single-instance, config lock, shell-extension channels, new CLSID), disable crash-report uploading, replace the icon family with generated rasters of the "Split Disc — Extruded" identity, rebuild the About/splash visuals (nanosvg icon + GDI-drawn wordmark, light/dark theme aware), apply the year-split copyright rule, and align governance documents (constitution, README, CLAUDE.md).

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI; nanosvg (in-repo) for SVG rasterization; Pillow 12.1.1 (dev machine only, asset generation — not a build dependency)
**Storage**: Windows Registry (`HKCU\Software\Newt Commander\0.1`); local filesystem for crash dumps (`%APPDATA%\Newt Commander`)
**Testing**: `build.cmd` Debug x64 build gate; manual runtime verification (icon surfaces, About/splash in both themes, registry write audit, side-by-side coexistence); per-surface checklist in quickstart.md
**Target Platform**: Windows 11+ x64
**Project Type**: Desktop application (existing 76-project MSBuild solution)
**Performance Goals**: N/A (no behavioral/perf change; splash/About rendering stays single-pass GDI/nanosvg)
**Constraints**: No source file/function renames (FR-016); plugin ABI 104 frozen (FR-015); no new build-time dependencies; no font installation (D18); assets reproducible from committed generator (Constitution I)
**Scale/Scope**: ~35 source files touched across app core, salmon, shellext, 3 plugins, lang resources; 5 regenerated icon/SVG asset files; 3 governance documents

## Constitution Check

*GATE: evaluated against Constitution v1.1.0.*

| Principle | Verdict | Notes |
|-----------|---------|-------|
| I. Build Reproducibility | PASS | No manual steps added; icon assets committed as files plus their generator (`tools/brand/`); build.cmd untouched. |
| II. Backward Compatibility | **DELIBERATE BREAK — justified** | The feature's explicit purpose (spec FR-009..FR-013) is severing identity/compatibility with Open Salamander 5.0. The break is user-mandated, documented in the spec, and FR-019 amends the constitution itself (re-anchoring the principle to the Newt Commander 0.1.0 baseline). Plugin ABI compatibility is *preserved* (FR-015). See Complexity Tracking. |
| III. Incremental Modernization | PASS | Rename-in-place strategy; smallest-diff choices (single-root array cut instead of import-code excision; TargetName instead of project rename). |
| IV. Windows Platform Commitment | PASS | Pure WinAPI; no new frameworks. |
| V. Plugin Architecture Preservation | PASS | ABI gate 104 unchanged; plugins get identity via shared version template + app-provided registry subkey; all 18 enabled plugins must load (SC-007). |
| VI. UI Consistency | PASS | About/splash keep DIALOGEX house style; theme awareness uses the app-wide feature-028 API (`IsDarkThemeActive`), an application-wide deliberate decision, not a local hack. |

**Post-design re-check**: unchanged verdicts; the Principle II break remains the single tracked violation, resolved by the constitution amendment shipped inside this feature (T-phase governance tasks) — the amendment lands in the same change set as the code it governs.

## Project Structure

### Documentation (this feature)

```text
specs/032-newt-commander-rebrand/
├── spec.md              # Feature specification (clarified)
├── questionnaire.md     # Decision questionnaire D01–D26 (resolved/defaults)
├── plan.md              # This file
├── research.md          # Phase 0 — decisions R1–R11
├── data-model.md        # Phase 1 — identity map (old → new)
├── quickstart.md        # Phase 1 — build & verification walkthrough
├── contracts/
│   └── branding-identity.md  # Canonical new identifiers (single source of truth)
└── tasks.md             # Phase 2 (/speckit.tasks output)
```

### Source Code (repository root)

```text
src/
├── vcxproj/salamand.vcxproj        # + <TargetName>newtcommander</TargetName>
├── versinfo.rh2                    # company/copyright/description/internal names
├── manifest.xml                    # OS identity + version 0.1.0.0
├── consts.h                        # SALCFG_ROOTS_COUNT 83 → 1
├── mainwnd2.cpp                    # registry roots array → single new root
├── mainwnd1.cpp                    # SALAMANDER_TEXT_VERSION
├── salamdr1.cpp                    # MAINWINDOW_NAME, CMAINWINDOW_CLASSNAME
├── tasklist.cpp                    # IPC object names
├── salamdr5.cpp, callstk.cpp, dialogs*.cpp, mainwnd3.cpp  # literals + URLs
├── logo.cpp                        # About/splash: new artwork, GDI wordmark, theming
├── salmoncl.cpp                    # bug-reporter key/folder/mutex names
├── shexreg.c / shexreg.h           # new CLSID, registration name, shared names
├── salamand.rc2                    # (unchanged refs) res/*.svg + salamand.ico payloads
├── res/                            # salamand.ico, sal_r/g/b.ico, logo.svg, gradspl.svg, gradabt.svg (regenerated)
├── lang/lang.rc, lang/texts.rc2    # 22 + usage-string rebrands, About block
├── salmon/                         # salmon.cpp, config.cpp, upload.cpp (neutered), salmon.rc, manifest.xml
├── shellext/shellext.rc            # metadata
└── plugins/
    ├── shared/spl_vers.h           # 0.1.0 macros + REQUIRE string
    ├── shared/versinfo.rc2         # ProductName "Newt Commander"
    ├── shared/vcxproj/x86.props, x64.props  # SalPath → newtcommander.exe
    ├── */versinfo.rh2              # per-plugin copyright/company (FR-021)
    ├── mdview/viewer.cpp           # WebView2 cache folder
    ├── pictview/pvtwain.cpp, ftp/ftp2.cpp   # literals
    └── filecomp/fcremote/fcremote.cpp       # exe literal

tools/brand/                        # NEW: icon SVG sources + gen_icons.py + README
.specify/memory/constitution.md     # v2.0.0 amendment
README.md, CLAUDE.md                # identity updates
```

**Structure Decision**: Rename-in-place inside the existing solution layout. No project/file renames; the only new directory is `tools/brand/` holding the visual-identity sources and the reproducible asset generator.

## Design Phases

### Phase 0 — Research (complete)

See [research.md](research.md): R1 exe rename via TargetName; R2 version macro chain (three-component fix); R3 Pillow-based icon pipeline (spike passed); R4 About/splash nanosvg+GDI redesign with theme branching; R5 single-root registry cut; R6 IPC namespace; R7 shellext CLSID `{A6D5A8E2-D69F-4E03-8396-781909E7A3AE}`; R8 salmon upload neuter; R9 string/URL sweep; R10 version-resource metadata; R11 governance.

### Phase 1 — Design artifacts (this plan)

- [data-model.md](data-model.md) — complete old→new identity mapping with owning files
- [contracts/branding-identity.md](contracts/branding-identity.md) — canonical identifier contract every task must use verbatim
- [quickstart.md](quickstart.md) — build + verification walkthrough (icon surfaces, About light/dark, registry audit, coexistence)

### Phase 2 — Task generation approach (executed by /speckit.tasks)

Order by user story priority with a build-gate after each: (US1) identity — exe name, version macros, manifest, window title, lang strings, version resources; (US2) separation — registry root, IPC names, shellext identity, salmon; (US3) visuals — asset generation, About/splash rework, theming; (US4) governance — constitution, README, CLAUDE.md. Asset generation is parallelizable with US1/US2; `logo.cpp` rework depends on assets.

## Complexity Tracking

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| Constitution II (Open Salamander 5.0 compatibility) broken | The feature's entire purpose is a user-mandated identity/compatibility split from the upstream product; continuity would defeat FR-009..FR-013 | "Compatible rebrand" (shared registry/IPC names) would let the two products corrupt each other's state and misrepresent the fork; the constitution is amended in-feature (FR-019) so the principle and the code change land together |
