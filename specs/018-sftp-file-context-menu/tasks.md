# Tasks: SFTP File Context Menu + Owner/Group (Feature 018)

**Input**: spec.md, plan.md, research.md, audit/A|B.md.

## Phase 1: Analysis
- [X] T001 Branch 018; brief; 2 audits (A FTP context-menu mechanism, B FTP attrs/chmod/chown)
- [X] T002 Consolidate audits → research.md (menu build/dispatch API; chmod/chown pattern; SFTP mapping)

## Phase 2: Owner/Group core (US2)
- [X] T003 [US2] CSFTPSession::Chown(path, uid, gid, setUid, setGid) via setstat UIDGID (session.cpp/.h)
- [X] T004 [US2] ChownRecursive + SFTPChangeOwnerFromPanel (operats.cpp/.h) mirroring ChmodRecursive
- [X] T005 [US2] ShowOwnerGroupDialog (uid/gid + recursive) + IDD resource + strings (dialogs.cpp/.h, lang.*)
- [X] T006 [US2] FR-005: leave an unchanged owner/group field intact (partial setstat flags)

## Phase 3: Context menu (US1)
- [X] T007 [US1] Implement CPluginFSInterface::ContextMenu (build popup + dispatch) per the FTP pattern (fs.cpp)
- [X] T008 [US1] Expose Change Attributes (existing chmod path) + Change Owner/Group in the menu
- [X] T009 [US1] Menu reflects selection (files/dirs/count); standard applicable ops per FTP; command IDs (lang.rh)

## Phase 4: Verify
- [X] T010 Build Debug + Release clean
- [ ] T011 [US1/US2/US3] Interactive test vs local test server: right-click → chmod (file/multi/dir-recursive); owner/group (file/dir-recursive); permission-denied path; no regression
- [ ] T012 Commit + ff main

## Notes
- chmod (drwxrwxrwx) + recursion already exist (ChmodRecursive / SFTPChangeAttrsFromPanel);
  reuse as the chown template and expose in the menu.
- chown = direct analogue via libssh2 setstat UIDGID; recursion mirrors ChmodRecursive (no symlink follow).
