// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

//****************************************************************************
//
// Copyright (c) 2023 Open Salamander Authors
//
// This is a part of the Open Salamander SDK library.
//
//****************************************************************************

#pragma once

#define WIN32_LEAN_AND_MEAN // exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <CommDlg.h>
#include <ShellAPI.h>
#include <shlobj.h>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif // _MSC_VER
#include <limits.h>
#include <process.h>
#include <commctrl.h>
#include <ostream>
#include <stdio.h>
#include <time.h>

#if defined(_DEBUG) && defined(_MSC_VER) // without passing file+line to 'new' operator, list of memory leaks shows only 'crtdbg.h(552)'
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

// If DEMOPLUG_COMPATIBLE_WITH_500 is defined, plugin is compiled as compatible with Salamander 5.0
// (otherwise it is compiled for current version of Salamander and later).
// NOTE: the ideal is to define DEMOPLUG_COMPATIBLE_WITH_500 for whole plugin project
//       in Project Settings. DemoPlug defines it here just to make adding/removing easier.
//
// IMPORTANT (plugin interface 104): builds against THIS SDK must NOT define
// DEMOPLUG_COMPATIBLE_WITH_500. Interface 104 widened CFileData::NameLen (9-bit
// bitfield -> full 32-bit field), which changes the CFileData memory layout and
// is an ABI break: a plugin reporting version 103 is refused at load. The ifdef
// mechanism is kept only to show where a two-target plugin would branch; leave it
// undefined so SalamanderPluginGetReqVer() returns LAST_VERSION_OF_SALAMANDER (104).
//#define DEMOPLUG_COMPATIBLE_WITH_500

#ifdef DEMOPLUG_COMPATIBLE_WITH_500
#define SALSDK_COMPATIBLE_WITH_VER 103 // 103 = Open Salamander 5.0 (SDK will be defined to be compatible with version 5.0)
#endif                                 // DEMOPLUG_COMPATIBLE_WITH_500

// UTF-8 <-> UTF-16 helpers (plugin interface 104): every char* name/path crossing
// the Salamander interface is UTF-8, so convert to UTF-16 before calling a W file
// API or drawing a name with a W text API. Include before the spl_* headers.
#include "splunicode.h"

#include "versinfo.rh2"

#include "spl_com.h"
#include "spl_base.h"
#include "spl_arc.h"
#include "spl_gen.h"
#include "spl_fs.h"
#include "spl_menu.h"
#include "spl_thum.h"
#include "spl_view.h"
#include "spl_vers.h"
#include "spl_gui.h"

#include "dbg.h"
#include "mhandles.h"
#include "arraylt.h"
#include "winliblt.h"
#include "auxtools.h"
#include "dialogs.h"
#include "demoplug.h"
#include "demoplug.rh"
#include "demoplug.rh2"
#include "lang\lang.rh"

#ifdef __BORLANDC__
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif // __BORLANDC__
