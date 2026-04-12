# Quickstart: Project Build and Architecture Analysis

**Date**: 2026-03-20
**Branch**: `001-project-build-analysis`

## What This Feature Delivers

A set of Markdown documents in `architecture/` that comprehensively
describe Open Salamander's build system, project structure, dependencies,
compiler options, and code standards. Plus a `CLAUDE.md` file at the
project root for AI assistant context.

## How to Verify the Deliverables

### 1. Check that all documents exist

```
architecture/
├── README.md
├── 01-project-overview.md
├── 02-solution-structure.md
├── 03-build-pipeline.md
├── 04-dependencies.md
├── 05-compiler-comparison.md
├── 06-plugin-architecture.md
├── 07-preprocessor-defs.md
└── 08-code-standards.md

CLAUDE.md  (project root)
```

### 2. Validate solution coverage

Open `architecture/02-solution-structure.md` and verify it lists all
90 projects from `src/vcxproj/salamand.sln`. Cross-reference by
opening the .sln file in a text editor and counting Project entries.

### 3. Validate dependency completeness

Open `architecture/04-dependencies.md` and verify:
- All directories in `src/common/dep/` are listed
- Missing dependencies (pvw32cnv, unrar, OpenSSL) are documented
- Each library has a license field

### 4. Validate compiler comparison

Open `architecture/05-compiler-comparison.md` and verify:
- At least 3 compilers evaluated (MSVC, Clang-cl, MinGW-w64)
- Each has installation instructions
- Each has compatibility verdict with rationale

### 5. Validate CLAUDE.md

Open `CLAUDE.md` and verify:
- Contains project overview
- References architecture/ directory
- Lists current development phase
- An AI assistant reading it can understand the project without
  additional exploration

## How to Use the Documents

- **New developer**: Start with `architecture/README.md`, then
  `03-build-pipeline.md` to set up your environment.
- **AI assistant**: Read `CLAUDE.md` first, then load specific
  architecture documents as needed.
- **Compiler decision**: Read `05-compiler-comparison.md` for the
  full evaluation with trade-offs.
- **Plugin developer**: Read `06-plugin-architecture.md` for the
  plugin API and build patterns.
