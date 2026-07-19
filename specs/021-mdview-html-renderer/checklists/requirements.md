# Specification Quality Checklist: mdview HTML Rendering Surface

**Purpose**: Validate specification completeness and quality before proceeding to planning
**Created**: 2026-07-19
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

- **"HTML rendering surface" is the feature's essential nature, not a leaked
  implementation choice.** The requirement to render embedded HTML faithfully
  (FR-020) and to display rich structures (tables, images) is stated as a
  user outcome. The *specific* rendering engine (OS HTML engine vs. in-tree
  HTML engine) is deliberately deferred to the plan phase; concrete
  candidates (and their trade-offs) live only in the Assumptions/Dependencies
  as references to `analysis/html-renderer.md`, not in the Functional
  Requirements or Success Criteria.
- **Security invariants (FR-050..FR-057)** are intentionally detailed despite
  being somewhat technical: they are non-negotiable carry-overs from feature
  020 and define what "safe" means for an untrusted-document viewer.
- **Governance gate (FR-070)**: implementation is explicitly gated on
  ratifying the feature-020 Decisions Q1/Q2 amendments. This is captured as a
  requirement and a dependency rather than a `[NEEDS CLARIFICATION]` marker,
  because the user has already directed the rendering-quality goal and engine
  preference; formal confirmation is expected in `/speckit.clarify`.
- **Clarify phase completed (Session 2026-07-19, 3 questions).** Resolved:
  (1) rendering-surface direction = browser-class OS HTML engine (WebView2 /
  Evergreen), so the FR-070 Q2 amendment is required [OQ-1]; (2) raw-HTML =
  render all natively, no sanitizer, safety by engine lockdown [OQ-2,
  FR-022]; (3) single HTML rendering backend, v1 RTF/RichEdit removed, fall
  back to text viewer [OQ-4, FR-038a]. Remote-image "no global always-allow"
  [D2] was already fixed in the spec (FR-012) and not re-asked. Deferred to
  plan (lower impact): concrete performance budget (SC-008), SVG rendering
  specifics (native under the locked engine), the remaining analysis open
  questions OQ-3/OQ-5..OQ-9.
- All checklist items pass. Spec is ready for `/speckit.plan`.
