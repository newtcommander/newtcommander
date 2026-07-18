# Specification Quality Checklist: SFTP Connect Window

**Created**: 2026-07-18 | **Feature**: [spec.md](../spec.md)

## Content Quality
- [X] Problem stated in user terms (can't save/edit bookmarks; password forgotten)
- [X] Technical cause in Problem Statement is evidenced (QuickConnect not persisted; dead Copy handler; no Save-edits action)
- [X] Mandatory sections complete

## Requirement Completeness
- [X] No [NEEDS CLARIFICATION] markers
- [X] Requirements testable (persistence round-trip; explicit lifecycle actions; no plaintext at rest)
- [X] Success criteria measurable (restart persistence, no dead controls, registry has no plaintext)
- [X] Edge cases identified (quick vs bookmark, master pw on/off, blank-field reuse, cancel)
- [X] Scope bounded (connect window + secret persistence; not SSH/transfer layers)
- [X] Assumptions/dependencies identified (008/009/010; password-manager API)

## Feature Readiness
- [X] FRs map to the two reported problems + verification of key handling
- [X] User scenarios cover password persistence, bookmark lifecycle, keys
- [X] No implementation leak in success criteria

## Notes
- Root causes independently confirmed by the orchestrator (QuickConnect absent
  from Save/LoadConfiguration; IDB_COPYBOOKMARK has no handler; no update-bookmark
  action); the three parallel audits (A password flow, B UX, C crypto/keys)
  consolidate into research.md and may add defects (FR-009 covers them).
- SFTP needs a live server, so full interactive proof is the user's; autonomous
  verification = builds + registry round-trip of a saved entry + code review.
