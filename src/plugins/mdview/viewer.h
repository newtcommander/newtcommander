// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// viewer.h - the mdview viewer window (WebView2 host).

#pragma once

#include "render.h"
#include "htmlgen.h"
#include <string>

class CMdWebHost;

class CViewerWindow : public CWindow
{
public:
    HANDLE Lock;      // signaled once the (possibly temp) source file may be released
    char* Name;       // full UTF-8 path (heap; may exceed MAX_PATH) or NULL
    CMdWebHost* Web;  // the WebView2 rendering surface

    HMENU HSchemeMenu; // the "Color Scheme" submenu (for radio/checkmark updates)

    const MdTheme* Theme;
    MdHtmlResult Html;        // current generated document (served by the host)
    std::wstring DecodedText; // decoded source (UTF-16); kept for re-render/find
    std::wstring FilePathW;   // display path (no \\?\)
    std::wstring DocDir;      // directory of the file (image resolution)
    MdEncoding Encoding;
    wchar_t FindText[256];
    int FindIndex;            // current match for find next/prev
    bool RemoteAllowed;       // per-document remote-image consent (D2)
    bool RenderPending;       // set before the controller is ready

    int EnumFilesSourceUID;
    int EnumFilesCurrentIndex;

public:
    CViewerWindow(int enumFilesSourceUID, int enumFilesCurrentIndex);
    ~CViewerWindow();

    HANDLE GetLock();
    void OpenFile(const char* name, BOOL setLock = TRUE);

protected:
    virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void BuildMenu();
    void RefreshSchemeChecks();
    const MdTheme* EffectiveTheme();
    void Regenerate(const std::wstring& fragment = std::wstring());
    void RenderDocument();
    void SetZoom(int pct);
    void SelectScheme(int idx);
    void CycleScheme(int dir);
    void DoFind(BOOL forward, BOOL prompt);
    void OpenAsText();
    void ActivateLink(const std::wstring& uri);
    void UpdateTitle();
    void EngineFailed();
};
