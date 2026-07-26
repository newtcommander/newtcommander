# Specification Quality Checklist: Fix File Name Encoding in Find Results and Name Notices

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

### Iteration 1 — 2026-07-26

Two items failed, both on the same missing decision:

- **"Scope is clearly bounded" — FAIL.** The spec committed to fixing the two
  reported surfaces and to producing an inventory, but never said how far the
  *fixing* extended beyond the two reports. Since the second report arrived
  precisely because an earlier feature fixed one surface and deferred the class,
  this was the decision determining whether the feature converges.
- **"No [NEEDS CLARIFICATION] markers remain" — FAIL.** One open question (sweep
  boundary), recorded in these notes rather than inline so the spec body stayed
  readable.

Deliberately *not* raised as clarifications, because feature 041 already decided
them and reopening would have been churn:

- Behaviour for genuinely uninterpretable characters → `U+FFFD` for the offending
  character only (Assumptions, FR-005).
- Whether localized builds are in scope for verification → yes, and Report 2 is
  only visible there (Assumptions, SC-004).
- Whether the plugin interface may change → no (FR-014).

### Iteration 2 — 2026-07-26

Question answered (option A): fix both reports **and** every main-application
surface the inventory shows to carry the same defect; produce the inventory
first; review plugins and record their state without modifying them.

Spec updated:

- New **Clarifications** section states the boundary, with explicit
  includes/excludes and a reason for each exclusion.
- **User Story 4** widened from "inventory the surfaces" to "find and repair
  every remaining affected surface", with acceptance scenarios covering the
  repairs, the spot-check, the deferrals and the plugin review.
- **FR-009** now requires the inventory to precede the repairs it drives;
  **FR-010** requires every same-defect surface to be fixed and verified;
  **FR-011** requires the plugin review with no plugin modified. Later
  requirements renumbered to FR-012…FR-015, and prose cross-references updated.
- **SC-007** (all identified surfaces verified fixed in the running application)
  and **SC-008** (every plugin in the review record, binaries unchanged) added;
  later criteria renumbered to SC-009…SC-011.
- Assumptions now point at Clarifications for the boundary, and record that an
  inventory larger than expected is grounds to re-cut scope with a real number
  rather than mid-flight.

**Result: all 16 items pass.** Both previously failing items are resolved — the
boundary is stated with reasons, and no clarification markers remain.

### Iteration 3 — 2026-07-26 (`/speckit.clarify` session)

Five further questions asked and integrated. All 16 items still pass; the spec
gained decisions in four areas that were Partial or Missing, and one internal
contradiction was removed.

| # | Question | Answer | Spec changes |
|---|----------|--------|--------------|
| 1 | Recurrence protection beyond one-time manual verification | Automated guard + manual verification | FR-016, FR-017, SC-012, SC-013; Problem Statement deliverable sentence |
| 2 | How the inventory is enumerated and completeness established | Both axes — mechanical pass carries completeness, UI walk cross-checks | FR-009a, FR-009b, SC-006a |
| 3 | May shared display entry points change behaviour for plugins? | No — plugin-visible behaviour bit-identical, main app opts in per call site | FR-014, FR-014a, SC-008a; plugin exclusion note |
| 4 | How codepage independence is proved | Structurally, via FR-002 + the FR-016 guard; no locale changed | SC-010 rewritten, FR-016 extended |
| 5 | How much localized verification each repaired surface needs | Risk-based: composed-with-localized-text → all 9 languages, bare name → once | SC-004 rewritten, FR-009c |

**Contradiction removed.** SC-010 (codepage independence) and FR-015 (verify in
the running application) were mutually unsatisfiable on this machine — the only
way to test the former was to change the system locale, which 041 had already
recorded as not done. Q4 dissolved it by making SC-010 structural rather than
sampled.

**Apparent contradiction clarified.** FR-002 ("not a case to be absorbed by
substitution") read against FR-005 (`U+FFFD` substitution) could look
self-cancelling. FR-002 now states the distinction explicitly: a narrow route is
never acceptable, malformed stored data is what `U+FFFD` is for.

**Consequence worth carrying into the plan.** Q3's answer removes the
single-choke-point fix. Because a shared entry point may not change behaviour for
plugin callers, the main application has to opt in per call site — so the FR-009
inventory is not a report produced alongside the work, it *is* the work list, and
its size determines the size of the feature. FR-009 already requires it to be
produced first for exactly this reason.

### Risks carried into planning

Not blockers for the spec, but the plan should address them explicitly:

- The two reported defects have *opposite* failure modes (lossy substitution vs.
  uninterpreted bytes). A fix aimed at one can mask or worsen the other; both
  need their own verification, not a shared one.
- The Find results Name column has now been "fixed" twice while its underlying
  display route stayed defective. FR-002 exists to forbid a third fix that merely
  widens the surviving character set.
- Feature 041 recorded that a broader change of this kind broke the Find dialog
  and had to be reverted. Localized-build verification is therefore a hard
  requirement here (Assumptions, SC-004), not a final sanity check.
- FR-009's inventory is the one open-ended item in the feature. If it returns a
  large number, the scope decision is re-opened with that number in hand rather
  than absorbed silently.
