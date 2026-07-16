// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

class CSFTPSession;

// ****************************************************************************
// File operations (download / upload / delete / chmod). v1 runs them
// synchronously on the calling thread with a cancellable wait window; the
// design leaves room for the worker-thread queue described in research.md.
// Each returns TRUE on full success, FALSE on error/cancel (a message has
// already been shown for genuine errors).
// ****************************************************************************

// Recursively downloads the panel's selected/focused items from 'remoteDir'
// into 'targetLocalPath'. 'move' additionally deletes the source on success.
BOOL SFTPDownloadFromPanel(HWND parent, CSFTPSession* session, int panel,
                           const char* remoteDir, const char* targetLocalPath, BOOL move);

// Uploads files/dirs enumerated by 'next'/'nextParam' (relative to
// 'sourcePath') into remote 'targetRemoteDir'. 'move' deletes local source.
BOOL SFTPUploadToFS(HWND parent, CSFTPSession* session, const char* sourcePath,
                    SalEnumSelection2 next, void* nextParam, const char* targetRemoteDir, BOOL move);

// Recursively deletes the panel's selected/focused items from 'remoteDir'.
BOOL SFTPDeleteFromPanel(HWND parent, CSFTPSession* session, int panel, const char* remoteDir);

// Applies a chmod (and optional mtime) to the panel's selection under
// 'remoteDir'. Opens the chmod dialog; returns TRUE when changes were applied.
BOOL SFTPChangeAttrsFromPanel(HWND parent, CSFTPSession* session, int panel, const char* remoteDir);
