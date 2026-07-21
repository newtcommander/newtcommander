# Contract: Salamander long-path clipboard data object (feature 027)

The only externally visible interface this feature touches. Applies ONLY
when the copy/cut selection involves any full path ≥ MAX_PATH (260); below
that, the legacy shell-verb route is used unchanged.

## Formats offered on OleSetClipboard

| Format | Content |
|---|---|
| `CF_HDROP` | `DROPFILES` header, `fWide = TRUE`; offset `pFiles` → wide, NUL-separated, double-NUL-terminated list of **full absolute display-form paths** (no `\\?\` prefix, UTF-16, unlimited length) |
| `CFSTR_PREFERREDDROPEFFECT` | `DWORD` `DROPEFFECT_COPY` (Ctrl+C) or `DROPEFFECT_MOVE` (Ctrl+X) |

After placement, `SetClipCutCopyInfo` registers the private marker format
`SALCF_IDATAOBJECT` ("SalIDataObject") exactly as the legacy route does, so
Salamander's paste selects route B (own engine).

## Consumers

- **Salamander paste (route B)**: parses DROPFILES honoring `fWide`
  (`ProcessClipboardData`), heap UTF-8 records → F5 engine. Fully long-path
  capable. This is the guaranteed round-trip.
- **External programs** (Explorer, etc.): receive a well-formed wide
  CF_HDROP. Whether they can operate on >260-char paths is the consumer's
  own capability — documented on the external-limit list; the data handed
  over is always valid.

## Invariants

1. Paths are display-form UTF-16 (no extended-length prefix) — consumers
   universally expect that shape.
2. The list is exactly the selected items' full names in panel order.
3. `IDataObject::GetData` supports `TYMED_HGLOBAL` for both formats;
   `EnumFormatEtc` advertises both.
4. Cut semantics: the marker + preferred effect MOVE; source deletion
   happens only after successful paste by the consuming engine (Salamander's
   own paste guarantees this; external consumers own their semantics).
5. Sub-260 selections never enter this code path (regression guard).
