# Quickstart: Verifying the New README

**Date**: 2026-07-22 | **Spec**: [spec.md](spec.md)

How to check the delivered `README.md` against the spec in a few minutes.

## 1. Content check (SC-001, SC-004)

Open `README.md` and confirm from the first screen alone you can answer:

1. What is the project called? → Newt Commander
2. What is it based on? → Open Salamander (link to the upstream GitHub repo)
3. How is it developed? → agentic programming, Spec-Driven Development, GitHub SpecKit,
   agentic frameworks with the best models of the era (currently Anthropic Fable 5)

Then confirm the old README's historical sections (Origin story, Altap history, Resources
list) are gone, and the branding note (built app still says "Open Salamander") is present.

## 2. Contract check

Walk `contracts/readme-content-contract.md` — every C-xx row passes, every N-xx row has no
match, F-01/F-02 verified with:

```powershell
# no BOM (first bytes must not be EF BB BF), CRLF endings
Format-Hex .\README.md -Count 3
```

## 3. Link check (SC-003)

- `https://github.com/OpenSalamander/salamander` — opens the upstream repository
- `https://github.com/github/spec-kit` — opens GitHub SpecKit
- Relative links exist: `doc/license_gpl.txt`, `doc/third_party.txt`, `AUTHORS`, `plugins.cfg`

## 4. Build-instruction check (SC-002, SC-005)

On a machine with VS2022 (+ C++ Desktop workload):

```batch
build.cmd help          :: usage matches what README documents
build.cmd               :: incremental Debug x64; without OPENSAL_BUILD_DIR set,
                        ::   the script notes the .\build\ default — README said so
```

Optionally the full path a fresh reader would take:

```batch
build.cmd full          :: complete Debug build incl. plugins + runtime data
%OPENSAL_BUILD_DIR%salamander\Debug_x64\salamand.exe   :: application starts
```

Expected: every command behaves exactly as the README describes; the running application
still titles itself Open Salamander — which the README explicitly says to expect.
