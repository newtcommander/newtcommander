# Data Model: Name/Buffer Invariants (031)

**Date**: 2026-07-23 | **Plan**: [plan.md](plan.md)

No persisted data changes. This feature is governed by in-memory invariants
around one entity.

## Entity: file-system entry name (`CFileData` name fields)

| Field | Type | Invariant (established by feature 004, unchanged here) |
|-------|------|--------------------------------------------------------|
| `Name` | `char*` (heap, `DupStr`) | UTF-8, byte-exact transcoding of the on-disk UTF-16 name; never normalized, case-folded, or best-fit mapped. |
| `NameLen` | `unsigned` | **Byte** length of `Name` (not characters). Disk enumeration bound: ≤ `SAL_FIND_NAME_U8 - 1` = 779; component-limit bound: ≤ 3 × 255 = **765**. Plugin/archive-supplied data: not guaranteed bounded — must be guarded at use sites. |
| `Ext` | `char*` | Alias **into** `Name` (points past the last `'.'`, or at the terminating NUL when no extension). Therefore `strlen(Ext) ≤ NameLen`, i.e. an "extension" can be up to ~764 bytes. Directories: always empty (points at NUL) unless `SortDirsByExt`. |
| `DosName` | `char*` | 8.3 alternate name, ≤ 12 chars + NUL — never long. |

### Derived length facts (why the crash class exists)

| Name shape | Chars (UTF-16 units) | UTF-8 bytes |
|------------|----------------------|-------------|
| ASCII, max component | 255 | 255 — always fits legacy 260/264 B buffers |
| Czech diacritics (2-byte), e.g. the user's repro | 215 | **330** — overflows 264 B |
| 3-byte BMP (e.g. `ě` NFC, CJK), max component | 255 | **765** — worst case |
| Surrogate pairs (4-byte UTF-8, 2 units each) | 254 (127 pairs) | 508 — bounded by 765 rule |

A name-component buffer therefore MUST hold `SAL_FIND_NAME_U8` (= 3 ×
`MAX_PATH` = 780) bytes; sites using the DWORD-null-terminate idiom
(`*(DWORD*)(buf + len) = 0`) need **+ 4**.

## Buffer sizing rule (the fix contract)

For every buffer receiving a whole name component or extension in the
panel/paint path:

```cpp
char buf[SAL_FIND_NAME_U8 + 4]; // feature 031: worst-case UTF-8 component + DWORD terminator
static_assert(sizeof(buf) >= SAL_FIND_NAME_U8 + 4, "031: name-component buffer must hold worst-case UTF-8 name");
```

### Guarded copy (plugin-supplied names)

```cpp
if (f->NameLen + 4 <= sizeof(buf))
{
    memmove(buf, f->Name, f->NameLen);
    *(DWORD*)(buf + f->NameLen) = 0;
    ... // icon-cache lookup
}
else
    drawSimpleSymbol = TRUE; // graceful fallback — never a truncated lookup key
```

State transition: *over-long name* → *simple symbol rendered*; never
*truncated name* → *lookup under a different identity* (spec FR-004).

### Bounded extension-lowercase loop

```cpp
char* dst = buf;
char* end = buf + sizeof(buf) - 4; // room for the DWORD terminator
char* src = f->Ext;
while (dst < end && *src != 0)
    *dst++ = LowerCase[*src++];
*(DWORD*)dst = 0;
// truncated ext (only possible beyond sizeof(buf)-4 bytes) merely misses
// the association lookup -> common file type / simple icon
```

### Tiles three-region buffer (`GetTileTexts` callers, kept in sync)

```cpp
char buff[(SAL_FIND_NAME_U8 + 4) + 2 * 512]; // name region + size + date
char* out0 = buff;                           // ≤ 766 B (AlterFileName preserves length)
char* out1 = buff + SAL_FIND_NAME_U8 + 4;    // PrintDiskSize output (short)
char* out2 = out1 + 512;                     // date/time output (short)
```

Applied identically at `filesbx1.cpp` (hit-test) and `fileswn0.cpp` (draw)
— the two sites the code marks "keep in sync".
