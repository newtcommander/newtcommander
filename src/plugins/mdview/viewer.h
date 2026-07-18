// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// viewer.h - the mdview viewer window (RichEdit host).

#pragma once

#include "render.h"
#include <string>

class CViewerWindow : public CWindow
{
public:
    HANDLE Lock;   // signaled once the (possibly temp) source file may be released
    char* Name;    // full UTF-8 path (heap; may exceed MAX_PATH) or NULL
    HWND HRich;    // the RichEdit child

    HMENU HSchemeMenu; // the "Color Scheme" submenu (for radio/checkmark updates)

    const MdTheme* Theme;
    MdRenderResult Render;
    std::wstring FilePathW; // display path (no \\?\)
    std::wstring DocDir;    // directory of the file (for image classification)
    MdEncoding Encoding;
    wchar_t FindText[256];

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
    void RenderDocument();
    void SetZoom(int pct);
    void SelectScheme(int idx);
    void CycleScheme(int dir);
    void DoFind(BOOL forward, BOOL prompt);
    void OpenAsText();
    void ActivateLinkByCp(long cp);
    void UpdateTitle();
};
