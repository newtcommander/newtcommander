# Tasks: Archive Associations Self-Heal (Feature 016)

**Input**: spec.md, plan.md, research.md.

## Phase 1: Forensics
- [X] T001 Branch 016; reproduce (Enter on ZIP → Explorer)
- [X] T002 Registry forensics: current config, pre-reset backup, Altap 4.0 source → confirm missing plugin associations (research R1)
- [X] T003 Code trace: 010 reset + CheckData cull + install-only creation = the chain (research R2)

## Phase 2: Implement (US1/US2)
- [X] T004 [US1] Self-heal block in CPlugins::CheckData() (plugins2.cpp): add associations for panel-view plugins whose declared extensions have no entry, only for unclaimed extensions; mirror SupportPanelEdit into pack/packer index
- [X] T005 [US2] Idempotent + non-destructive (skip if entry exists; never touch extensions claimed by another entry)

## Phase 3: Verify
- [X] T006 Build Debug + Release clean
- [X] T007 Run fixed Release vs the user's CURRENT broken config → association table healed: added zip;pk3;pk4;jar, 7z, tar…, iso…, cab (plugin refs -1..-5); 6 external rows untouched (6→11 rows)
- [X] T008 Second run → 11 rows unchanged (idempotence confirmed)
- [ ] T009 [US1] User interactive Enter on unicode-test.zip → browses in panel (final confirmation)

## Phase 4: Commit
- [X] T010 Commit + ff main
