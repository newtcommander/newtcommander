# Specification Quality Checklist: Fix UI Text Encoding

**Created**: 2026-07-26 | **Feature**: [spec.md](../spec.md)

## Content Quality
- [x] No implementation details (languages, frameworks, APIs)
- [x] Focused on user value and business needs
- [x] Written for non-technical stakeholders
- [x] All mandatory sections completed

## Requirement Completeness
- [x] No [NEEDS CLARIFICATION] markers remain
- [x] Requirements are testable and unambiguous
- [x] Success criteria are measurable
- [x] Success criteria are technology-agnostic
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

All 16 pass. Three clarifications were resolved from precedent rather than asked,
because features 041 and 042 already decided them: the `U+FFFD` policy, the
plugin boundary, and all-9-language verification for composed surfaces.

Scope was bounded by measurement, not estimate: three independent surveys ran
before the spec was finalised, which is why the "Scale, measured" section carries
real per-site counts instead of an envelope.

Two findings changed the shape of the feature after the initial draft:
- The user's mid-flight report that F5/F6 are affected turned out to be the same
  `CTruncatedString` family as F2 — 10 sites, one shared repair.
- `NumberToStr`/`PrintDiskSize` splice a UTF-8 locale separator, so every
  formatted number reaching a legacy sink is affected. That was not in the
  original reports and widens the regression surface materially.
