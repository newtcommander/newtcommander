# Specification Quality Checklist: SFTP File Context Menu + Owner/Group

**Created**: 2026-07-18 | **Feature**: [spec.md](../spec.md)

## Content Quality
- [X] Problem in user terms (right-click does nothing; need chmod menu + owner/group)
- [X] Technical cause noted (ContextMenu is an empty stub; chown missing) with evidence
- [X] Mandatory sections complete

## Requirement Completeness
- [X] No [NEEDS CLARIFICATION] markers
- [X] Requirements testable (menu appears; chmod applies; owner/group applies recursively)
- [X] Success criteria measurable (against the local test SFTP server)
- [X] Edge cases (multi-select, recursion cancel/skip, permission denied, numeric uid/gid, symlinks)
- [X] Scope bounded (context menu + chmod exposure + new owner/group; not new transfer logic)
- [X] Assumptions/dependencies identified (008/009/017; libssh2 setstat UIDGID; FTP pattern)

## Feature Readiness
- [X] FRs map to the two gaps (menu incl. chmod; owner/group recursive)
- [X] User scenarios cover chmod, owner/group, no-regression
- [X] No implementation leak in success criteria

## Notes
- chmod (drwxrwxrwx) + recursion already exist (ChmodRecursive); the work is
  exposing it in the context menu + adding owner/group (setstat UIDGID) with a
  recursive walk mirroring ChmodRecursive.
- Two audits analyze the FTP context-menu mechanism + attribute/chown flow so
  the SFTP implementation follows the proven pattern (FR-008).
