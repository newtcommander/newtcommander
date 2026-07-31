// SPDX-FileCopyrightText: 2026 Pavel Stupka
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Lightweight precomp.h used ONLY by the dev unit-test harness so that
// sftputils.cpp (which begins with #include "precomp.h") can be compiled
// standalone, without pulling in the whole Salamander plugin SDK. The real
// plugin build uses the SDK precomp.h; this shim is copied to "precomp.h"
// in a flat temp dir by build_and_run.cmd.

#pragma once
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
