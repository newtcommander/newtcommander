// SPDX-FileCopyrightText: 2026 Pavel Stupka
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// ****************************************************************************
// Host-key trust store (TOFU). The plugin keeps its own store in the registry
// (clarification #3: OpenSSH known_hosts is not consulted in v1). No host key
// is ever accepted without user interaction (FR-006).
// ****************************************************************************

class CSFTPHostKey
{
public:
    char* Host;        // normalized lowercase
    int Port;
    char* KeyType;     // e.g. "ssh-ed25519", "rsa-sha2-256"
    char* KeyBlobB64;  // base64 of the full public-key blob (exact-match compare)
    char* FingerprintSHA256; // cached "SHA256:..." display form
    FILETIME Added;

    CSFTPHostKey();
    ~CSFTPHostKey();
};

enum CSFTPHostKeyMatch
{
    hkmUnknown = 0,  // no record for host:port
    hkmMatch = 1,    // record exists and blob matches
    hkmMismatch = 2, // record exists but blob differs
};

class CSFTPHostKeyList : public TIndirectArray<CSFTPHostKey>
{
public:
    CSFTPHostKeyList() : TIndirectArray<CSFTPHostKey>(10, 10) {}

    // Compares a presented key blob against the store. On hkmMismatch, the
    // stored fingerprint is returned in 'storedFp' (size 'storedFpSize').
    CSFTPHostKeyMatch Match(const char* host, int port, const char* keyBlobB64,
                            char* storedFp, int storedFpSize);

    // Adds or replaces the trusted key for host:port.
    BOOL Trust(const char* host, int port, const char* keyType,
               const char* keyBlobB64, const char* fingerprint);

    CSFTPHostKey* Find(const char* host, int port);
};

extern CSFTPHostKeyList HostKeys;

// Base64-encode a binary blob; returns a heap string (free with free()).
char* Base64Encode(const unsigned char* data, int len);

// Formats a raw SHA-256 hash (32 bytes) as the OpenSSH "SHA256:base64" form.
void FormatSHA256Fingerprint(const unsigned char* hash, char* buf, int bufSize);
// Formats a raw MD5 hash (16 bytes) as colon-separated hex.
void FormatMD5Fingerprint(const unsigned char* hash, char* buf, int bufSize);
