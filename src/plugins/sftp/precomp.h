// SPDX-FileCopyrightText: 2026 Pavel Stupka
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// winsock2 must be included before windows.h so that libssh2 (which pulls in
// winsock2.h) does not clash with the older winsock.h dragged in by windows.h
#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <shlobj.h>
#include <crtdbg.h>
#include <limits.h>
#include <process.h>
#include <ostream>
#include <stdio.h>
#include <time.h>

#if defined(_DEBUG) && defined(_MSC_VER) // without passing file+line to 'new' operator, list of memory leaks shows only 'crtdbg.h(552)'
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#include "versinfo.rh2"

#include "spl_com.h"
#include "spl_base.h"
#include "spl_gen.h"
#include "spl_fs.h"
#include "spl_menu.h"
#include "spl_gui.h"
#include "spl_vers.h"

#include "splunicode.h" // interface 104: UTF-8 <-> UTF-16 helpers for W file APIs

#include "dbg.h"
#include "mhandles.h"
#include "arraylt.h"
#include "winliblt.h"
#include "auxtools.h"

#include "sftp.rh"
#include "sftp.rh2"
#include "lang\lang.rh"
