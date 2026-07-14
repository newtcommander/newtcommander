// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

//*****************************************************************************
//
// pluglegacy.h
//
// String adaptation for plugins built against interface <= 103 (feature 004,
// contracts/plugin-interface-vnext.md par. 2). The core speaks UTF-8; legacy
// binaries expect the system ANSI code page:
//
//   core -> legacy plugin: UTF-8 -> ACP WITHOUT best-fit mapping; any
//     unrepresentable character means the item must be REFUSED with the
//     FR-014 per-item message - never passed through silently altered.
//   legacy plugin -> core: ACP -> UTF-8 (always representable).
//
// Gate call sites with PluginSupportsUTF8(builtForVersion) from plugins.h.
//

// UTF-8 -> system ACP, no best-fit. Returns FALSE when the string contains a
// character not representable in the ACP or does not fit 'bufSize' - the
// caller must refuse the item (FR-014), not truncate.
BOOL PlugLegacyU8ToACP(const char* u8, char* buf, int bufSize);

// ACP -> UTF-8. Returns FALSE only when the result does not fit 'bufSize'.
BOOL PlugLegacyACPToU8(const char* acp, char* buf, int bufSize);

// allocating variant of ACP -> UTF-8; free() the result; NULL on failure
char* PlugLegacyACPToU8Alloc(const char* acp);
