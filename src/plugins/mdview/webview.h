// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// webview.h - CMdWebHost: the locked-down WebView2 rendering surface for
// mdview. All WebView2/COM usage is confined to webview.cpp; this header is
// COM-free so viewer.cpp can hold a CMdWebHost by pointer. See
// specs/021-mdview-html-renderer/contracts/webhost.md.

#pragma once

#include <windows.h>
#include <string>
#include <functional>
#include "htmlgen.h"

struct CMdWebHostImpl; // defined in webview.cpp

class CMdWebHost
{
public:
    struct Callbacks
    {
        std::function<void()> OnReady;                     // controller ready -> render
        std::function<void(const std::wstring&)> OnActivateLink; // navigation gate
        std::function<void()> OnInitFailed;                // env/controller/runtime failure
        std::function<void()> OnProcessFailed;             // renderer crashed
    };

    CMdWebHost();
    ~CMdWebHost();

    // Starts async environment+controller creation parented to 'parent'.
    bool Create(HWND parent, const std::wstring& userDataFolder, const Callbacks& cb);
    bool IsReady() const;
    void Resize(int cx, int cy);
    void SetZoomPercent(int pct);

    // Sets the document served by the interceptor (does not navigate).
    void SetDocument(const MdHtmlResult* doc, const std::wstring& docDir);
    // (Re)loads the current document; optional same-document '#fragment'.
    void Navigate(const std::wstring& fragment = std::wstring());
    void Destroy();

    static bool RuntimeAvailable();

private:
    CMdWebHostImpl* p;
    CMdWebHost(const CMdWebHost&) = delete;
    CMdWebHost& operator=(const CMdWebHost&) = delete;
};
