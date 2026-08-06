# Specification Quality Checklist: SFTP Dialogs — Ephemeral Quick Connect, Empty Bookmarks, Untruncated Localized Texts

**Purpose**: Validate specification completeness and quality before proceeding to planning
**Created**: 2026-08-06
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

- **Revised after clarification (2026-08-06).** The original spec required new
  Czech wording ("Soubor", "Heslo", "Výchozí adresář"). The user corrected the
  brief: translations are not to be reworded — the display must show the
  existing localized text in full. FR-008 was inverted accordingly (texts MUST
  NOT be reworded) and US3 rewritten.
- Two further clarifications are recorded in the spec: the room for the texts
  comes from **widening the dialog** (input fields keep their width), and the
  scope is **every SFTP plugin dialog**, not only the connect dialog.
- Planning-phase measurement (all nine dialogs × eight languages) refined the
  numbers the spec now states: **26 distinct clipped controls in six dialogs**,
  worst a configuration checkbox with 118 units for text needing ~208. It also
  surfaced two display defects the original wording did not cover — a
  run-time-composed password prompt that overflows, and an existing 44-unit
  control overlap in the configuration dialog — both folded into FR-009/FR-010.
- The truncation cause is structural and is recorded in Assumptions: a control
  sized for English with its neighbour immediately to the right leaves the
  build's automatic widening no free space to grow into. That is why a data-only
  regeneration cannot fix it.
- Quick Connect scope is bounded explicitly in Assumptions (only the Quick
  Connect entry loses persistence; bookmarks, host-key trust and other
  settings are untouched) to keep FR-001 testable.
- Purging stale Quick Connect data (FR-004/SC-005) is framed as a security
  fix, not a migration — consistent with the constitution's no-migration
  stance for MINORB releases.
- A product-wide automated guard against truncation is explicitly deferred
  (recorded in Assumptions as possible follow-up), since it would report
  hundreds of pre-existing findings and is its own feature.
