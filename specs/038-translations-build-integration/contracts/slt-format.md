# Contract: `.slt` Translation Text Archive

**Status**: **Consumed, frozen.** This grammar is defined by existing code —
`CData::ExportAsTextArchive` (writer) and `CData::ImportTextArchive`
(`src/translator/trldata.cpp:2301`, reader). This feature must produce files the
existing reader accepts; it does not get to change the format.

---

## Encoding

UTF-8 **with BOM** (`EF BB BF`). The reader skips the BOM if present
(`trldata.cpp:2396`) but the writer always emits it — match the writer.

Line terminator `CRLF`. Blank lines are structural separators, not decoration.

---

## The rule that dominates everything: import is positional

`ImportTextArchive` does **not** index by ID. It iterates the module's parsed
resources and demands the file line up exactly:

```cpp
for (i = 0; i < DlgData.Count; i++) {                       // trldata.cpp:2474
    ret &= ITACheckSection(buff, lineNumber, L"DIALOG", dialog->ID);
    for (int j = 0; j < dialog->Controls.Count; j++) { ... }
}
for (i = 0; i < MenuData.Count; i++) { ... }                // :2505
for (i = 0; i < StrData.Count; i++) { ... }                 // :2533
```

Consequences a producer must respect:

1. **Sections appear in fixed order**: all `[DIALOG]`, then all `[MENU]`, then
   all `[STRINGTABLE]`, then the optional `[RELAYOUT]`.
2. **Every** dialog, menu, and string-table block in the module must be present,
   in the module's own order — none may be skipped, added, or reordered.
3. Within a dialog, every control must be present, in order.
4. Menu items whose original text is empty are **omitted** (`wcslen(item->OString) > 0`,
   `trldata.cpp:2517`) — separators do not get rows.
5. String-table slots whose original is `NULL` are **omitted**
   (`StrData[i]->OStrings[j] != NULL`, `:2540`).
6. Any deviation → `ret = FALSE` → *"Syntax error in file X on line N"* message
   box → **the entire import is rejected**, not just the offending entry.

This is why legacy Salamander-4.0 `.slt` files cannot be imported directly, and
why every `.slt` this feature ships is generated from a current-structure
template.

---

## Grammar

```
file        := BOM exportinfo BLANK translation BLANK dialog* menu* strtab* relayout?
```

### `[EXPORTINFO]` — written, then ignored on read

```
[EXPORTINFO]
PROJECTNAME,"<project path>"
TEXTVERSION,"<version text>"
VERSION,"<a,b,c,d>"
```

The reader consumes exactly four lines here and **discards all of them**
(`trldata.cpp:2404-2412`, four `// skip` calls) before requiring a blank line.
So the values are informational only — but the **line count is load-bearing**.
Emit all four.

### `[TRANSLATION]`

```
[TRANSLATION]
LANGID,<int>
AUTHOR,"<text>"
WEB,"<text>"
COMMENT,"<text>"
HELPDIR,"<text>"          ; optional
SLGINCOMPLETE,"<text>"    ; optional
```

`LANGID`, `AUTHOR`, `WEB`, `COMMENT` are required and positional. `HELPDIR` and
`SLGINCOMPLETE` are each detected by key and skipped if absent
(`trldata.cpp:2440-2470`) — the reader is tolerant here, unlike everywhere else.

### `[DIALOG <id>]`

First row is the **dialog itself** (no control ID — it carries the caption);
subsequent rows are controls.

```
[DIALOG 100]
205,72,1,"Vybrat"                       ; cx, cy, state, caption
1150,8,8,56,8,1,"&Vybrat soubory:"      ; ctrlID, x, y, cx, cy, state, text
101,8,18,189,86,1,""
1,19,50,50,14,1,"OK"
```

Geometry is in dialog units and is **round-tripped through the file** — a
producer may adjust `cx` to fit longer translated text (FR-013), and the change
lands in the emitted `RT_DIALOG`.

### `[MENU <id>]`

```
[MENU 6011]
6062,1,"&Kopírovat\tCtrl+C"             ; itemID, state, text
6066,1,"Vybrat &vše\tCtrl+A"
```

### `[STRINGTABLE <n>]`

`<n>` is the **block index** (0-based), not a resource ID. Each block covers 16
consecutive string IDs; the row's first field is the actual string ID.

```
[STRINGTABLE 0]
1011,1,"Název cesty"                    ; strID, state, text
1012,1,"Velikost"
```

### `[RELAYOUT]` — optional trailing section

```
[RELAYOUT]
148
1385
```

Bare dialog IDs, one per line, marking dialogs a human should re-lay-out. Read
at `trldata.cpp:2560-2585`. Nothing else may follow the last required section —
any other trailing content is a syntax error.

---

## Field encoding

| Element | Rule |
|---|---|
| `state` | `0` = untranslated, `1` = translated (`PROGRESS_STATE_UNTRANSLATED` / `_TRANSLATED`, `trldata.h:6`) |
| Text | Always double-quoted, always last on the line |
| `"` inside text | Escaped per `EncodeString`/`DecodeString` (`trldata.h:27-28`) |
| Numeric fields | Decimal, comma-separated, no spaces |

---

## Content that must survive translation

These appear inside quoted text and are **not** free prose. A producer must
carry them through unchanged (FR-012):

| Marker | Example | Rule |
|---|---|---|
| Accelerator | `&Kopírovat` | Exactly one `&` where the English had one; the letter may move, and must stay unique within its dialog/menu |
| Shortcut label | `"…\tCtrl+C"` | Everything from `\t` onward is a key name — **never translate it** |
| `printf` placeholder | `%s` `%d` `%u` `%c` `%x` `%ld` | Same multiset as the English source; order may change only if the format is positional |
| Escape sequence | `\n` `\r` `\t` `\\` `\"` | Preserved verbatim |

Validation is a producer-side responsibility: `ImportTextArchive` will happily
accept a translation that has dropped a `%s`, and the defect surfaces only at
runtime as a malformed message.

---

## Producer checklist

Before writing a `.slt` this feature intends to import:

- [ ] UTF-8 BOM, CRLF line endings
- [ ] Four `[EXPORTINFO]` value lines, then a blank line
- [ ] `[TRANSLATION]` with `LANGID`/`AUTHOR`/`WEB`/`COMMENT` in that order
- [ ] Section order: dialogs → menus → string tables → optional `[RELAYOUT]`
- [ ] Exactly one row per control / menu item / populated string slot, in module order
- [ ] Empty-text menu items and `NULL` string slots omitted
- [ ] Blank line after every section
- [ ] Placeholders, accelerators, and `\t`-shortcuts preserved against the English source
- [ ] Nothing trailing after the last section except `[RELAYOUT]`

The cheapest way to satisfy all of this is to start from a template produced by
`-quiet-export-slt` and replace only the quoted text (and, where needed, control
geometry) — never to assemble a `.slt` from scratch.
