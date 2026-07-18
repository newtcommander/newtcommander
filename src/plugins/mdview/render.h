// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// render.h - Markdown parsing + theming + RTF generation for mdview.
// Self-contained: no I/O, no network. See specs/020-mdview-plugin/research.md
// (D-R2/D-R6): a pragmatic block+inline parser feeding a static RTF renderer;
// md4c is the documented production upgrade behind this same boundary.

#pragma once

#include <windows.h>
#include <string>
#include <vector>

// --- Color scheme (data-model.md) -----------------------------------------

struct MdSyntax // 9 syntax token classes
{
    COLORREF kw, str, num, cmt, type, fn, op, add, del;
};

struct MdTheme
{
    const char* id;   // stable ASCII id (never localized, never an index)
    int nameStrId;    // IDS_THEME_FIRST + index (localized display name)
    bool dark;
    COLORREF docBg, body, heading, link, linkActive;
    COLORREF quoteText, quoteAccent, codeInlineFg, codeInlineBg;
    COLORREF codeBg, codeText, tableBorder, tableHeadBg, rule;
    COLORREF sel, selText, imgFg, imgBg;
    MdSyntax syn;
};

extern const MdTheme MdThemes[];
extern const int MdThemeCount;                 // == 10
const MdTheme* MdThemeById(const char* id);    // nullptr if unknown
const MdTheme* MdThemeDefault(bool dark);      // paper / graphite
int MdThemeIndex(const MdTheme* t);

// --- Encoding detection (research.md D-R5, FR-050) -------------------------

enum MdEncoding
{
    MDENC_UTF8,
    MDENC_UTF8BOM,
    MDENC_UTF16LE,
    MDENC_UTF16BE,
    MDENC_ANSI,   // legacy fallback (system code page) - show warning bar
    MDENC_BINARY  // not text - caller should hand off to the text/hex viewer
};

// Decodes raw bytes into UTF-16 'out'. Returns the detected encoding.
MdEncoding MdDetectDecode(const BYTE* data, size_t len, std::wstring& out);

// --- Render result --------------------------------------------------------

struct MdLinkEntry
{
    std::wstring url;      // resolved target (may be an internal "#anchor")
    bool internalAnchor;   // true => 'url' is a slug to scroll to
};

struct MdAnchorEntry
{
    std::wstring slug;
    long charPos;          // approximate RichEdit character index
};

struct MdLinkRange
{
    long cpMin, cpMax;     // RichEdit character range of the clickable text
    int linkId;            // index into 'links'
};

struct MdRenderResult
{
    std::string rtf;                    // RTF bytes fed to RichEdit via EM_STREAMIN
    std::vector<MdLinkEntry> links;
    std::vector<MdAnchorEntry> anchors;
    std::vector<MdLinkRange> linkRanges;
};

struct MdRenderLimits
{
    size_t maxDepth = 64;               // FR-092 nesting cap
    size_t maxNodesText = 40u * 1024 * 1024; // output cap safety
};

// Renders 'md' (UTF-16) with 'theme' into 'out'. Pure; never throws on
// malformed input (best-effort, FR-082). 'docDir' is the directory of the
// source file (UTF-16, long-path safe) used only to classify image refs.
void MdRenderMarkdown(const std::wstring& md, const MdTheme& theme,
                      const std::wstring& docDir, MdRenderResult& out,
                      const MdRenderLimits& lim = MdRenderLimits());

// GitHub-style heading slug (Unicode lowercase, keep letters/digits/-/_/space
// then spaces->'-'; Czech diacritics preserved). Exposed for testing.
std::wstring MdSlug(const std::wstring& headingText);

// --- syntax highlighting (highlight.cpp) ----------------------------------
// Color-table indices shared with render.cpp's RTF color table order.
#define MDCF_KEYWORD 15
#define MDCF_STRING  16
#define MDCF_NUMBER  17
#define MDCF_COMMENT 18
#define MDCF_TYPE    19
#define MDCF_FUNC    20
#define MDCF_OP      21

struct HlRun
{
    size_t start, len;
    int colorCf; // one of MDCF_*
};

// Best-effort lexical highlighting for the tier-1 languages (FR-013).
// Unknown/absent language => 'runs' left empty (plain block).
void HighlightCode(const std::wstring& code, const std::string& lang, std::vector<HlRun>& runs);
