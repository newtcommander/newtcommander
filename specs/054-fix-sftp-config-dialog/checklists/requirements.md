# Specification Quality Checklist: SFTP — Reachable Settings, Reliable Connect, Tight Dialog Layout

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

- All three defects were **measured** during feature 053's verification and are
  recorded with evidence in `specs/053-sftp-connect-dialog/investigation.md` §9,
  so the causes needed no clarification. The user independently confirmed the
  first one ("it is an invisible window").
- **Clarified 2026-08-06/07** (3 questions, all about the desired outcome rather
  than the cause):
  1. Addresses are attempted **overlapping** (next one starts ~0.25 s after the
     previous, first to answer wins) rather than one-at-a-time with a divided
     budget. Added FR-004 detail, FR-011 (drop the losers), tightened SC-003 to
     "under 2 seconds", and added two edge cases (cancel with several attempts
     open; two answering at once).
  2. The settings window is an **ordinary titled dialog** like every other
     plugin's, not a page inside the application's Configuration window.
     Sharpened FR-001.
  3. The connect dialog's **own width follows the language**. Added FR-012, an
     edge case for a dialog wider than the screen, and an assumption recording
     why the fixed-window alternative was rejected (it would narrow the fields,
     which FR-010 forbids).
- The spec deliberately states the *symptom* the user reported ("the whole
  program freezes") and the outcome required, not the mechanism — the cause is
  known and belongs in the plan, not in requirements a stakeholder must read.
- FR-005 restates feature 051's bounded-total-time guarantee explicitly, so the
  fix for US2 cannot regress the defect that feature was created to fix. That
  tension is the main risk in this feature and is called out in Assumptions.
- US3's requirement is phrased as "size from the language in use" rather than
  naming a mechanism, so the plan is free to choose between measuring at run
  time and any equivalent approach, provided FR-009/FR-010 hold.
- Scope is bounded to the SFTP plugin; generalizing the layout idea across the
  product is named as separate work.
