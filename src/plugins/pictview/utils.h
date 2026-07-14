// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// see shared/spl_gen.h for the description
// compared to the function in Salamander this stripped-down variant omits the nextFocus parameter
// and support for DefaultDir (paths such as "c:path...")
BOOL SalGetFullName(LPTSTR name, int* errTextID, LPCTSTR curDir);

// sets a dialog control's text from a UTF-8 string (interface 104) via the W API, so file
// names outside the ACP display correctly; falls back to the A call on conversion failure
void SetDlgItemTextU8(HWND hDlg, int idCtrl, const char* u8Text);
