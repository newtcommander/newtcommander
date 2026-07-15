// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

//*****************************************************************************
//
// wicengine.h
//
// In-process image engine backed by the Windows Imaging Component (WIC).
// Replaces the proprietary PVW32Cnv.dll, which was removed from the
// repository for GPL reasons (feature 006-fix-pictview-plugin). The engine
// fills the existing CPVW32DLL function-pointer table with functions of
// identical signatures, so all viewer call sites stay unchanged.
//
// VIEW-CRITICAL functions are implemented fully (open / info / decode /
// draw / stretch / background / close / error text). Out-of-scope
// operations (save, crop, clipboard, image sequences) are non-crashing
// stubs returning a clean PVCODE. PVChangeImage (lossless 90deg rotation)
// IS implemented so EXIF auto-rotate keeps working.
//
// FileName passed to PVOpenImageEx is UTF-8 (plugin interface 104); the
// engine converts it to UTF-16 with the \\?\ long-path prefix internally.
//

struct CPVW32DLL;

// fills all PV* members of the table with the WIC engine implementations
// (the four plugin-internal helpers GetRGBAtCursor/CalculateHistogram/
// CreateThumbnail/SimplifyImageSequence are assigned by the caller)
void InitWicEngine(CPVW32DLL* table);
