// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// ****************************************************************************
// Per-connection session log (FR-032). Human-readable, never records secrets.
// Thread-safe append; a modeless Logs window (own thread) shows the logs.
// ****************************************************************************

struct CLogData
{
    int UID;
    char* Name;      // connection label, e.g. "user@host:22"
    char* Text;      // heap log buffer
    int TextLen;
    int TextAlloc;

    CLogData();
    ~CLogData();
};

class CLogs
{
protected:
    CRITICAL_SECTION Lock;
    TIndirectArray<CLogData> Data;
    int NextUID;
    HWND LogsWindow; // modeless logs dialog (in its own thread) or NULL

public:
    CLogs();
    ~CLogs();

    // Creates a new log for a connection label; returns its UID (>=0).
    int CreateLog(const char* name);
    // Appends a line to the log identified by 'uid' (a timestamp is prepended).
    void Append(int uid, const char* text);
    void AppendFmt(int uid, const char* fmt, ...);

    // Clears a single log's text.
    void ClearLog(int uid);

    // Snapshot access for the Logs dialog (caller must Enter/Leave around use).
    void Enter() { EnterCriticalSection(&Lock); }
    void Leave() { LeaveCriticalSection(&Lock); }
    int GetCount() { return Data.Count; }
    CLogData* GetAt(int i) { return Data[i]; }
    CLogData* FindByUID(int uid);

    void RegisterWindow(HWND wnd) { LogsWindow = wnd; }
    HWND GetWindow() { return LogsWindow; }

    BOOL Enabled(); // reflects Config.EnableLogging
};

extern CLogs Logs;

// Opens (or activates) the modeless Logs dialog. 'showUID' selects a log.
void ShowLogsDialog(HWND parent, int showUID);
