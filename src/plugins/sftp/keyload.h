// SPDX-FileCopyrightText: 2026 Pavel Stupka
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// ****************************************************************************
// Private-key loading helpers (FR-003). Authentication goes through libssh2's
// publickey_frommemory path on the WinCNG crypto backend, which supports
// classic PEM (RSA, encrypted or not) and OpenSSH-container keys holding an
// RSA or ECDSA key. PKCS#8, PuTTY ".ppk" and ed25519 keys cannot be used and
// are rejected here, before any connection attempt (feature 051).
// ****************************************************************************

enum CSFTPKeyFormat
{
    kfUnknown = 0,
    kfPEM = 1,     // -----BEGIN RSA/EC/DSA PRIVATE KEY-----
    kfPKCS8 = 2,   // -----BEGIN [ENCRYPTED] PRIVATE KEY-----
    kfOpenSSH = 3, // -----BEGIN OPENSSH PRIVATE KEY-----
    kfPuTTY = 4,   // PuTTY-User-Key-File-2/3
};

// Detects the key file format by inspecting its header. Returns kfUnknown when
// the file cannot be read or the header is not recognized.
CSFTPKeyFormat DetectKeyFormat(const char* keyFilePath);

// Returns TRUE if the plugin can authenticate with the given key format. When
// it returns FALSE, '*reasonStrId' is set to a resource id naming the problem
// and the remedy (used to reject .ppk, PKCS#8 - FR-003, feature 051).
BOOL KeyFormatSupported(CSFTPKeyFormat fmt, int* reasonStrId);

// Full up-front check of a key file: format plus, for OpenSSH-container keys,
// the algorithm inside (ed25519 is not available on the WinCNG backend).
// FALSE => '*reasonStrId' names the reason; when it is IDS_ERR_KEYTYPEUNSUP,
// 'typeOut' receives the offending algorithm name for the message (feature 051,
// FR-007). 'typeOut' may be NULL.
BOOL KeyFileSupported(const char* keyFilePath, int* reasonStrId,
                      char* typeOut, int typeOutSize);

// CF-20: TRUE if the key file's header shows it is passphrase-encrypted
// (classic PEM "Proc-Type/DEK-Info", or "BEGIN ENCRYPTED PRIVATE KEY"), or the
// OpenSSH container declares a cipher other than "none" (feature 051 reads the
// container's ciphername, so an encrypted OpenSSH key is now detected too).
BOOL KeyFileLooksEncrypted(const char* keyFilePath);

// Reads the algorithm name declared inside an OpenSSH-container private key
// (e.g. "ssh-rsa", "ecdsa-sha2-nistp256", "ssh-ed25519"). Returns FALSE when
// the file is not an OpenSSH container or the type cannot be determined.
BOOL ReadOpenSSHKeyType(const char* keyFilePath, char* typeOut, int typeOutSize);
