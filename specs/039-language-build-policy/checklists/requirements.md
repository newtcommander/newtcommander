# Specification Quality Checklist: Language Build Policy

**Purpose**: Validate specification completeness and quality before proceeding to planning
**Created**: 2026-07-26
**Feature**: [spec.md](../spec.md)

## Content Quality

- [x] No implementation details (languages, frameworks, APIs)
- [x] Focused on user value and business needs
- [x] Written for non-technical stakeholders
- [x] All mandatory sections completed

## Requirement Completeness

- [x] No [NEEDS CLARIFICATION] markers remain
- [x] Requirements are testable and unambiguous
- [x] Success criteria are measurable
- [x] Success criteria are technology-agnostic (no implementation details)
- [x] All acceptance scenarios are defined
- [x] Edge cases are identified
- [x] Scope is clearly bounded
- [x] Dependencies and assumptions identified

## Feature Readiness

- [x] All functional requirements have clear acceptance criteria
- [x] User scenarios cover primary flows
- [x] Feature meets measurable outcomes defined in Success Criteria
- [x] No implementation details leak into specification

## Notes

**Validation (2026-07-26) — all items pass, no clarifications raised.**

This is a small, well-bounded feature and the request was specific, so no
[NEEDS CLARIFICATION] markers were warranted. Three decisions were made as
documented assumptions rather than questions, because each has a clearly better
default:

| Decision | Chosen | Why not ask |
|---|---|---|
| Where the switch lives | Extend `translations/languages.cfg` | A second file listing language names would need cross-validation against the registry — the exact failure FR-007 exists to prevent. Same maintainer-facing result: one file, one line per language. |
| What "off" means for source | Not shipped; source retained | Deleting committed translations — including the only Ukrainian translation in existence — to achieve a build-policy outcome would be destructive and clearly not intended by "prozatim" (for now). |
| Authoring tools vs disabled languages | Skip by default, explicit opt-in | Raised as a question in clarification and confirmed by the maintainer; now FR-012 / FR-013. |

**Clarification session 2026-07-26**: one question asked and answered (authoring
tools vs disabled languages → skip by default, explicit opt-in). A second
candidate — whether this feature must also make the installer ship only enabled
languages — was resolved without asking: `src/setup/` carries no per-language
file list, so there is no conflicting second source to reconcile, and installer
packaging remains feature 038's open task T047. Recorded as a scope boundary in
Overview and Assumptions.

Points worth noting for planning:

- **FR-003 is the requirement that makes the feature work at all.** Not building
  a disabled language is insufficient: modules from an earlier build stay in the
  output tree, the product still finds them, and disabling appears to do nothing.
  The existing plugin policy already reconciles output this way, so there is a
  precedent to follow rather than a new mechanism to invent.
- **The rendering defect is recorded, not specified.** The request was explicitly
  "for now" (`prozatim`), so the deliverable is the policy switch. The
  observation that exactly the three non-Latin-script languages are affected is
  recorded as a lead for whoever picks it up, flagged as unverified.
- **SC-004 (byte-identical content after re-enabling) is the real test of
  reversibility** and is cheap to check mechanically.
