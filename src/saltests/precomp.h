// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

// Thin precompiled-header stand-in for the saltests unit-test host.
// The shared src/common sources include "precomp.h"; inside the main
// application that resolves to src/precomp.h, here to this file.

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h> // SHELLEXECUTEINFO (used by the shared salfileio layer)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
