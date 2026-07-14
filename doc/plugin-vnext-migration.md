# Migrating Plugins to Interface 104 (UTF-8 Names + Long Paths)

Audience: third-party plugin authors. This guide covers porting a
plugin from SDK interface 103 (Open Salamander 5.0) to interface 104
(5.0 build 184 and later).

Authoritative contract: `specs/004-long-paths-unicode/contracts/plugin-interface-vnext.md`
(including the 2026-07-14 implementation amendment). Design decisions:
`specs/004-long-paths-unicode/research.md` (R1-R10). SDK headers:
`src/plugins/shared/` (`spl_vers.h`, `spl_com.h`, `spl_base.h`,
`spl_gen.h`, and the new `splunicode.h` conversion helpers).

---

## 1. What changed in 104 and why

Interface 104 is the first version in which Salamander handles the
full Windows name space: any Unicode file name (including decomposed
forms and characters beyond the basic plane) and paths up to the OS
maximum of ~32,767 UTF-16 units.

The changes visible to a plugin:

- **Every `char*` name and path crossing the plugin interface — in
  both directions — is now UTF-8.** Structure shapes are unchanged:
  no wide-string re-typing, `CFileData::Name` is still a `char*`
  allocated with `CSalamanderGeneralAbstract::Alloc`. Only the
  encoding contract moved from "active ANSI code page" to UTF-8
  (research.md R1). ASCII-only names are byte-identical to before.
- **`CFileData::NameLen` widened from a 9-bit bitfield (`unsigned :9`)
  to a full 32-bit `unsigned` field.** It is the UTF-8 **byte** length
  of `Name` (`strlen(Name)`), the former `MAX_PATH - 5` cap is gone.
  This changes the size and layout of `CFileData` — **it is an ABI
  break** and the reason the interface version was bumped
  (`spl_com.h`, struct `CFileData`).
- **Paths are no longer bounded by `MAX_PATH`.** A full path may be up
  to ~32,767 UTF-16 units, which is up to `3 * 32767` bytes in UTF-8.
  Any `char buf[MAX_PATH]` holding a *full path* is a latent
  truncation bug.
- **The `\\?\` extended-length prefix is the core's business.**
  Plugins always receive and return paths in display form
  (`C:\dir\...`, `\\server\share\...` — never `\\?\C:\...`). The core
  applies `\\?\` at its own OS boundary (research.md R2). Your plugin
  only needs the prefix for W-API calls it makes itself (section 4).
- **The process ANSI code page is untouched** (research.md R3). Do
  not assume the ACP is UTF-8; `-A` WinAPI functions still interpret
  bytes in the legacy system code page, which is exactly why they must
  not be fed interface strings (section 4).
- Registry/config access through the `SalRegQueryValueEx` family
  (`spl_gen.h`) keeps `char*` signatures; the payload is UTF-8 and the
  core converts at its registry boundary (research.md R9).

## 2. Hard rule: binaries built for interface <= 103 are refused at load

Because the `NameLen` widening changes the in-memory `CFileData`
layout, an old binary given live core structures would read and write
the wrong bytes. There is no partial-marshalling middle ground that is
safe, so the core raises `PLUGIN_REQVER` to 104 (`src/plugins.h`) and
**cleanly refuses to load any plugin whose
`SalamanderPluginGetReqVer` returns less than 104**, with the standard
"plugin too old" message. Nothing is corrupted; the plugin simply does
not load.

**Migration therefore means one thing: recompile against the 104
SDK.** There is no compatibility shim to target. A single binary
cannot span the 103/104 boundary either — the old dual-export trick
(low `SalamanderPluginGetReqVer` + `SalamanderPluginGetSDKVer`,
`spl_base.h`) cannot bridge a structure-layout break. If you must
keep supporting Salamander <= 5.0 build 183, ship two binaries.

Step by step:

1. **Rebuild** against the 104 SDK headers (`src/plugins/shared/`).
2. **Fix `NameLen`-related compile breaks.** `NameLen` is no longer a
   bitfield: code that took its address indirectly, assumed
   `sizeof(CFileData)`, memcpy'd or serialized raw `CFileData`
   records, or clamped names to `MAX_PATH - 5` must be revised. Any
   persisted binary dump of `CFileData` from a 103 build is invalid.
3. **Audit `MAX_PATH` assumptions.** Every buffer that holds a *full
   path* must accept long paths — allocate dynamically or use a
   growable buffer (the core uses `CSalPathBuf` /
   `SAL_MAX_PATH_UTF8 = 3 * 32767 + 1`, see `src/common/salpath.h`).
   `MAX_PATH`-sized buffers remain fine for single name components,
   8.3 `DosName` strings, and drive roots (research.md R10).
4. **Treat all interface strings as UTF-8** — both what you receive
   and what you hand back. Convert to UTF-16 for your own WinAPI
   calls (section 4) and never route interface strings through `-A`
   APIs or ACP-dependent CRT functions.
5. **Return the new version from the entry export:**

```cpp
int WINAPI SalamanderPluginGetReqVer()
{
    return LAST_VERSION_OF_SALAMANDER; // 104: first UTF-8/long-path interface
}
```

Steps 2-4 in detail below.

## 3. Bytes vs. characters

`NameLen` and every other length field on the interface is a **byte
count**, not a character or glyph count.

- One Unicode code point takes 1-4 UTF-8 bytes. A file-name component
  is capped by the OS at 255 UTF-16 units, so a single component may
  need up to **765 bytes** (`3 * 255`) of UTF-8. Size per-component
  buffers accordingly.
- Byte-oriented scanning for ASCII delimiters stays valid: UTF-8
  continuation bytes are always >= 0x80, so `'\\'`, `'.'`, `':'`,
  `'*'`, `'?'` never occur inside a multibyte sequence. `strchr`,
  `strrchr`, and backslash-splitting code keep working unchanged.
- **Never truncate at an arbitrary byte offset** — you can cut a
  multibyte sequence in half and produce invalid UTF-8. Truncate only
  at an ASCII delimiter, or convert to UTF-16 first.
- **UI drawing and measuring must convert to UTF-16.** GDI does not
  honor UTF-8 in `-A` calls; use `DrawTextW`,
  `GetTextExtentPoint32W`, `ExtTextOutW`, and W window-text APIs on
  the converted string. Column widths computed from `NameLen` are
  wrong for any non-ASCII name.

```cpp
// measuring a single file-name component for painting
#include "splunicode.h"

WCHAR wName[256]; // component <= 255 UTF-16 units + null
if (SplU8ToW(fd->Name, wName, _countof(wName)) != 0)
{
    SIZE sz;
    GetTextExtentPoint32W(hDC, wName, (int)wcslen(wName), &sz);
    // ...
}
```

## 4. File I/O inside the plugin

**`CreateFileA(utf8Path, ...)` is wrong twice over.** The `-A` entry
points interpret the bytes in the legacy system code page — a UTF-8
path with any non-ASCII byte names a different (usually nonexistent)
file, silently. And `-A` functions never reliably support paths beyond
`MAX_PATH`, regardless of manifests or registry settings.

The correct pattern for every file API you call yourself:
**UTF-8 -> UTF-16, apply the extended-length prefix, call the W
API.** The 104 SDK ships this as header-only helpers in
`src/plugins/shared/splunicode.h`:

| Helper | Purpose |
|--------|---------|
| `SplU8ToW` / `SplWToU8` | strict UTF-8 <-> UTF-16 into a caller buffer (return units written incl. the null, 0 on failure) |
| `SplU8ToWAlloc` / `SplWToU8Alloc` | allocating variants, `free()` the result |
| `SplU8ToWExtAlloc` | UTF-8 display-form path -> heap UTF-16 extended-length path for W file APIs |
| `SplIsASCII` | fast-path predicate (ASCII-only strings need no conversion) |

All conversions are strict (`MB_ERR_INVALID_CHARS` /
`WC_ERR_INVALID_CHARS`): invalid input fails instead of being
silently replaced — mirror that in any conversion you write yourself.

Extended-length prefixing rules (what `SplU8ToWExtAlloc` and the
core's `SalPathToWExtAlloc` in `salpath.cpp` implement):

```
C:\dir\file             ->  \\?\C:\dir\file
\\server\share\file     ->  \\?\UNC\server\share\file   (UNC: replace "\\" with "\\?\UNC\")
\\?\anything            ->  unchanged (already extended)
```

Two consequences of `\\?\` you must respect (research.md R2):

- The prefix **disables OS path normalization** — `.` and `..`
  segments and forward slashes are NOT resolved by the OS anymore.
  Paths the core hands you are already absolute and normalized; if
  you build a path yourself from other input, run it through
  `GetFullPathNameW` (on the un-prefixed form) before prefixing.
  `SplU8ToWExtAlloc` deliberately returns relative paths unprefixed —
  resolve them first if they can exceed `MAX_PATH`.
- Never show a `\\?\` path to the user or store it in configuration;
  strip the prefix when converting back for display.

Putting it together:

```cpp
#include "splunicode.h"

HANDLE OpenFileU8(const char* u8Path, DWORD access, DWORD share, DWORD disposition)
{
    WCHAR* wPath = SplU8ToWExtAlloc(u8Path); // \\?\-prefixed, long-path capable
    if (wPath == NULL)
    {
        SetLastError(ERROR_INVALID_NAME);
        return INVALID_HANDLE_VALUE;
    }
    HANDLE h = CreateFileW(wPath, access, share, NULL, disposition,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD err = GetLastError();
    free(wPath);
    SetLastError(err); // preserve the API's error across free()
    return h;
}
```

Apply the same pattern to `FindFirstFileW`, `DeleteFileW`,
`MoveFileExW`, `CreateDirectoryW`, `GetFileAttributesW`, ... — and
convert names coming back (e.g. `WIN32_FIND_DATAW::cFileName`) to
UTF-8 with `SplWToU8` before they enter interface structures.

Do not use the `longPathAware` manifest as your long-path strategy:
it only works when a machine-wide registry opt-in is also set, which
you cannot rely on. The `\\?\` + W-API route works everywhere
(research.md R2).

## 5. Name fidelity: never alter what you pass through

Windows treats canonically equivalent but differently composed names
(NFC "é" = `U+00E9` vs. NFD "e" + `U+0301`) as **distinct directory
entries**. Salamander preserves and distinguishes them, and so must
your plugin:

- **Never normalize, case-fold, trim, or otherwise rewrite name bytes
  you pass through** (FR-006). The bytes you receive are the bytes
  you must hand to the OS and back to the core.
- Two entries whose names are canonically equivalent can legally
  coexist; both must remain individually addressable, and they are
  *not* an overwrite conflict with each other (FR-007).
- **Matching driven by user input** (masks, search, name lookups)
  must treat canonically equivalent forms as equal (FR-008): convert
  both sides to UTF-16, normalize *transient copies* to NFC, then
  compare — never write the normalized form back anywhere.

```cpp
// case-insensitive, canonical-equivalence-insensitive name match
// (transient NFC copies only; wA/wB converted from UTF-8 as in section 4)
WCHAR nfcA[512], nfcB[512];
if (NormalizeString(NormalizationC, wA, -1, nfcA, _countof(nfcA)) > 0 &&
    NormalizeString(NormalizationC, wB, -1, nfcB, _countof(nfcB)) > 0)
{
    match = CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                            nfcA, -1, nfcB, -1, NULL, NULL, 0) == CSTR_EQUAL;
}
```

Pure-ASCII fast path: when both strings contain only bytes < 0x80,
your existing byte-wise comparison is already correct — keep it and
skip the conversions (this is what the core does, research.md R4/R5;
see `SalNameEqualCI` / `SalNameEquivalent` in `src/common/salunicode.h`).

## 6. Archive plugins: convert at the format boundary

Entry names inside archives are stored in whatever encoding the format
prescribes (CP437, an archiver-recorded code page, a UTF-8 flag, an
explicit Unicode field, ...). Interface 104 makes the rule explicit:
**convert between the format's encoding and UTF-8 exactly at the
format boundary** — when reading headers and when writing them — and
keep everything between those two points UTF-8.

Listing/extracting (format encoding -> UTF-8):

```cpp
// returns FALSE when the stored bytes do not decode in 'codePage' ->
// report the item through the skip/error callback, continue with the rest
BOOL EntryNameToU8(const char* raw, UINT codePage, char* u8, int u8Size)
{
    WCHAR w[1024];
    if (MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, raw, -1,
                            w, _countof(w)) == 0)
        return FALSE;
    return WideCharToMultiByte(CP_UTF8, 0, w, -1, u8, u8Size, NULL, NULL) != 0;
}
```

Packing (UTF-8 -> format encoding) is where names can be
**unrepresentable**. Convert without best-fit mapping and check for
lossy results — a silently mangled name is data corruption:

```cpp
// FALSE when the name cannot be stored losslessly in 'codePage'
BOOL U8ToEntryName(const char* u8, UINT codePage, char* raw, int rawSize)
{
    WCHAR w[1024];
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, u8, -1,
                            w, _countof(w)) == 0)
        return FALSE;
    BOOL usedDefault = FALSE;
    if (WideCharToMultiByte(codePage, WC_NO_BEST_FIT_CHARS, w, -1,
                            raw, rawSize, NULL, &usedDefault) == 0)
        return FALSE;
    return !usedDefault;
}
```

When conversion fails or would be lossy, **do not abort the whole
operation and do not store an approximation**: report the single item
through the existing skip/error callbacks (the `DIALOG_SKIP` /
`DIALOG_SKIPALL` dialog family, `spl_gen.h`) with a message naming
the item, and continue with the remaining items. This matches the
per-item refusal UX the core itself uses (FR-014). If the format has
a native Unicode representation (e.g. ZIP's UTF-8 flag, general
purpose bit 11), prefer storing that instead of failing.

## 7. Checklist

| # | Item | Where |
|---|------|-------|
| 1 | Rebuild against the 104 SDK; no 103 binary loads anymore | `src/plugins/shared/` |
| 2 | `SalamanderPluginGetReqVer` returns `LAST_VERSION_OF_SALAMANDER` (104) | section 2 |
| 3 | `NameLen` handled as 32-bit UTF-8 byte count; no `MAX_PATH - 5` cap; no raw `CFileData` serialization from 103 | sections 1, 2 |
| 4 | No fixed `char[MAX_PATH]` buffer holds a full path; components/8.3/roots may keep it | section 2, research.md R10 |
| 5 | All interface strings treated as UTF-8, both directions | section 1 |
| 6 | No interface string reaches an `-A` WinAPI or ACP-dependent CRT call | section 4 |
| 7 | Own file I/O: `SplU8ToWExtAlloc` (or equivalent: UTF-8 -> UTF-16, `\\?\` / `\\?\UNC\` prefix) + W API | section 4 |
| 8 | No `\\?\` path shown to users or persisted | section 4 |
| 9 | Drawing/measuring converts to UTF-16 and uses W text APIs | section 3 |
| 10 | Names passed through byte-for-byte; no normalization or case-folding of stored names | section 5 |
| 11 | User-input matching normalizes transient copies to NFC before comparing | section 5 |
| 12 | Archives: encoding conversion only at the format boundary, lossless check on pack | section 6 |
| 13 | Unrepresentable/undecodable entry names -> per-item skip/error callback, operation continues | section 6 |

Plugin-side helpers: `src/plugins/shared/splunicode.h` (header-only,
ships with the 104 SDK). Reference implementations to mirror:
`src/common/salunicode.h` (conversion, NFC matching),
`src/common/salpath.h` + `salpath.cpp` (display form vs.
extended-length form), `src/common/salfileio.h` (W-API file-I/O
wrappers).
