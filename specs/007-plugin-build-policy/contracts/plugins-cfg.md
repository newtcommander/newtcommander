# Contract: `plugins.cfg` (repository root)

The single authority on which plugins the scripted build compiles and
ships. Consumed by `build.cmd` (via `src/vcxproj/gen_plugins_filter.ps1`)
at the start of every run.

## Grammar

```text
file      := line*
line      := blank | comment | entry
blank     := WS*
comment   := WS* '#' any*
entry     := WS* name WS* '=' WS* state WS*
name      := [A-Za-z0-9_][A-Za-z0-9_-]*     ; case-insensitive
state     := 'on' | 'off'                    ; case-insensitive
```

- Encoding: ASCII/UTF-8; CRLF or LF line endings both accepted.
- One entry per remaining plugin — currently 28. `shared` is not a
  plugin and must not appear.
- Order is not significant; alphabetical order is the maintained style.

## Semantic rules

| # | Rule | On violation |
|---|---|---|
| V1 | File must exist at repo root | build stops before compilation; message names the expected path and purpose |
| V2 | Every line must parse per grammar | stop; message cites file, line number, offending text |
| V3 | `name` must match an existing `src/plugins/<name>` directory (case-insensitive) | stop; message names the unknown entry |
| V4 | No duplicate `name` (case-insensitive) | stop; message names the duplicated entry |
| V5 | Every plugin directory must have an entry | stop; message names the unlisted plugin directory |

All violations are detected in a single validation pass and reported
before MSBuild is invoked (SC-004).

## Committed initial content (normative)

28 entries; exactly these 10 `off`, the other 18 `on`:

```text
# Open Salamander plugin build policy.
# Read by build.cmd on every run: only plugins set to "on" are
# compiled and shipped (see specs/007-plugin-build-policy/).
# One line per plugin: <plugin-directory-name>=on|off

7zip=on
automation=off
checksum=on
checkver=off
dbviewer=on
demomenu=off
demoplug=off
demoview=off
diskmap=on
filecomp=on
folders=on
ftp=on
mmviewer=off
nethood=off
peviewer=on
pictview=on
portables=on
regedt=on
renamer=on
tar=on
uncab=on
unchm=off
undelete=on
uniso=on
unmime=off
unole=off
unrar=on
zip=on
```

(Comment wording may differ; the entry set and states are normative.)
