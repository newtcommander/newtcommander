// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// ****************************************************************************
// Private-key loading helpers (FR-003). The initial release authenticates via
// libssh2's native publickey_fromfile path (PEM/PKCS#8 and, with the WinCNG
// backend, OpenSSH-container keys). This module hosts format detection and the
// ".ppk" handling; ed25519/OpenSSH-container fallbacks are layered in during
// User Story 5 (spike S1 decides the exact split - see research.md R1).
// ****************************************************************************

enum CSFTPKeyFormat
{
    kfUnknown = 0,
    kfPEM = 1,          // -----BEGIN RSA/EC/DSA PRIVATE KEY-----
    kfPKCS8 = 2,        // -----BEGIN [ENCRYPTED] PRIVATE KEY-----
    kfOpenSSH = 3,      // -----BEGIN OPENSSH PRIVATE KEY-----
    kfPuTTY = 4,        // PuTTY-User-Key-File-2/3
};

// Detects the key file format by inspecting its header. Returns kfUnknown when
// the file cannot be read or the header is not recognized.
CSFTPKeyFormat DetectKeyFormat(const char* keyFilePath);

// Returns TRUE if the plugin can currently authenticate with the given key
// format. When it returns FALSE, '*reasonStrId' is set to a resource id
// naming the supported formats (used to reject e.g. .ppk v3 - FR-003).
BOOL KeyFormatSupported(CSFTPKeyFormat fmt, int* reasonStrId);
