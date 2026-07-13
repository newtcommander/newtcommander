// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

//*****************************************************************************
//
// salunicode.h
//
// UTF-8 <-> UTF-16 conversion and Unicode-aware file-name helpers.
//
// Internal narrow strings that carry file names and paths use UTF-8
// (feature 004-long-paths-unicode, research.md R1/R4). Conversion to
// UTF-16 happens only at OS boundaries (file APIs, GDI, window text,
// registry). Stored names are NEVER normalized or case-folded by these
// helpers; NFC normalization is applied to transient copies used for
// matching and collation only.
//
// Limitation: names containing unpaired UTF-16 surrogates (legal on
// NTFS, producible only deliberately) are not representable in strict
// UTF-8; conversions fail for them and callers surface the standard
// per-item error (FR-004/FR-014 semantics).
//

//*****************************************************************************
//
// SalU8ToW / SalWToU8
//
// Convert between UTF-8 and UTF-16. Strict conversions: invalid input
// sequences fail (return 0) instead of being silently replaced.
//
// Parameters
//   src: source string (need not be null-terminated when srcLen >= 0)
//   srcLen: length of src in bytes/WCHARs, or -1 for null-terminated
//   buf: destination buffer (may be NULL to query the required size)
//   bufSize: destination capacity in WCHARs/bytes
//
// Return Values
//   Number of WCHARs/bytes written including the terminating null
//   (or required size when buf == NULL); 0 on failure.
//
int SalU8ToW(const char* src, int srcLen, WCHAR* buf, int bufSize);
int SalWToU8(const WCHAR* src, int srcLen, char* buf, int bufSize);

// Allocating variants; caller releases with free(). Return NULL on failure.
WCHAR* SalU8ToWAlloc(const char* src, int srcLen = -1);
char* SalWToU8Alloc(const WCHAR* src, int srcLen = -1);

//*****************************************************************************
//
// SalWToACPLossless
//
// Convert UTF-16 to the system legacy ANSI code page WITHOUT best-fit
// mapping. Used by the legacy plugin adaptation shim (contract §2):
// a lossy result means the item must be refused, never passed altered.
//
// Return Values
//   TRUE when every character was representable; FALSE otherwise
//   (buf receives nothing usable on FALSE).
//
BOOL SalWToACPLossless(const WCHAR* src, int srcLen, char* buf, int bufSize);

//*****************************************************************************
//
// SalNormalizeNFC
//
// Normalize a UTF-16 string to Unicode Normalization Form C.
// Output is a transient value for matching/collation; never write it
// back into stored names.
//
// Return Values
//   Number of WCHARs written including the terminating null (or the
//   estimated required size when buf == NULL); 0 on failure.
//
int SalNormalizeNFC(const WCHAR* src, int srcLen, WCHAR* buf, int bufSize);
WCHAR* SalNormalizeNFCAlloc(const WCHAR* src, int srcLen = -1); // free() the result

//*****************************************************************************
//
// SalIsASCII
//
// Fast path predicate: TRUE when the buffer contains only bytes < 0x80.
// ASCII-only strings are valid UTF-8 and byte-wise comparable, so
// callers keep the legacy comparators for them (research.md R5).
//
BOOL SalIsASCII(const char* s, int len = -1);

//*****************************************************************************
//
// SalNameEquivalent
//
// TRUE when two UTF-8 names are canonically equivalent (their NFC
// forms are binary-identical). Case-SENSITIVE. Byte-identical names
// are always equivalent (fast path, no conversion).
//
BOOL SalNameEquivalent(const char* u8a, const char* u8b);

//*****************************************************************************
//
// SalCompareNamesUTF8
//
// Locale collation of two UTF-8 names for panel sorting (FR-009).
// Canonically equivalent names compare equal here; callers tie-break
// with a binary comparison to keep the order deterministic.
// ASCII fast path is NOT applied here - callers decide (sort.cpp
// keeps its byte-wise comparators when both names are ASCII).
//
// Return Values
//   < 0, 0, > 0  (strcmp convention); on conversion failure falls back
//   to strcmp so sorting never loses items.
//
int SalCompareNamesUTF8(const char* u8a, int aLen, const char* u8b, int bLen, BOOL ignoreCase);

//*****************************************************************************
//
// SalNameEqualCI
//
// Case-insensitive, canonical-equivalence-insensitive equality of two
// UTF-8 names (quick search, Find, mask matching - FR-008).
//
BOOL SalNameEqualCI(const char* u8a, int aLen, const char* u8b, int bLen);
