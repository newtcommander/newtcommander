// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

// Clipboard CF_HDROP helpers for long (>MAX_PATH) UTF-8 paths (feature 027).
// Pure functions over DROPFILES memory blocks; unit-tested in saltests.

#pragma once

// Builds a CF_HDROP block (GMEM_MOVEABLE | GMEM_DDESHARE): DROPFILES header
// with fWide=TRUE followed by a NUL-separated, double-NUL-terminated list of
// UTF-16 paths converted from the UTF-8 inputs. The DROPFILES format itself
// has no path-length limit, unlike the shell "copy" verb route. Returns NULL
// on invalid UTF-8, empty input, or allocation failure. On success the caller
// owns the handle (SetClipboardData takes ownership when it succeeds).
HGLOBAL SalBuildWideDropFiles(const char* const* utf8Paths, int count);

// Scans a DROPFILES block (wide or ANSI) of 'blockSize' bytes. Returns the
// number of paths and sets *longestLen (may be NULL) to the length of the
// longest path in characters. Returns -1 for a malformed/truncated block
// (never reads past blockSize).
int SalScanDropFiles(const DROPFILES* df, SIZE_T blockSize, int* longestLen);
