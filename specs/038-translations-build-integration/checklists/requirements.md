# Specification Quality Checklist: Translations Build Integration

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

**Validation iteration 1 (2026-07-26)** — 3 open clarifications, all other items passing.

**Validation iteration 2 (2026-07-26)** — all items pass.

Q1–Q3 resolved by the requester and folded into the requirements:

| Question | Decision | Requirements |
|----------|----------|--------------|
| Q1 — untranslated text | Machine-translate the gaps; store as reviewable source, mark machine-origin | FR-008, FR-010–FR-013 |
| Q2 — legacy branding | Rewrite product/vendor names everywhere, drop predecessor web links, keep translator names | FR-018–FR-021 |
| Q3 — shipping set | Ship all languages regardless of coverage, **plus new machine-translated Ukrainian** — 12 total | FR-001, FR-002, FR-009, FR-027, User Story 4 |

Quality notes carried forward:

- Content stays at the "what/why" level. File formats, build tooling, resource
  compilers and the existing translation utility are deliberately not named — the
  spec talks about *translation source*, *language module* and *module* instead.
- Every functional requirement is observable either in the running product or in
  build output. FR-013, FR-021 and FR-026 state a checkable condition rather than
  an intent.
- Success criteria are all countable or binary (12 languages, 99% of session text
  translated, zero clipped dialogs, zero legacy-brand strings, two equivalent
  clean builds).
- Scope boundaries are explicit: enabled plugins only, no help files, no
  translator tooling, no live switching, no commissioned human translations.

Points the planning phase must resolve (implementation-level, correctly excluded
from the spec but material to effort):

- FR-023 separates translation *preparation* from the *build*. Planning must
  decide where machine translation runs and how its output is regenerated when
  English changes — the spec only requires that an ordinary build never invokes it.
- FR-013 (dialog layout) and FR-021 (declined product names) are the two
  requirements most likely to need per-language manual correction passes.
- Ukrainian covers the main application plus all 19 enabled plugins from zero,
  which is the single largest volume item in the feature.
