// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// render.cpp - self-contained Markdown -> RTF renderer for mdview.
// Pragmatic block+inline parser (documented subset of CommonMark 0.31.2 +
// the four GFM extensions; md4c is the production upgrade). No I/O, no network.

#include "precomp.h"
#include "render.h"
#include <cwctype>

#include "mdview.rh2" // IDS_THEME_FIRST

// ==========================================================================
// Color schemes (contrast-corrected per specs/020-mdview-plugin/analysis/visual.md;
// stock Solarized/Nord body & comment values are deliberately darkened/lightened)
// ==========================================================================

#define TH(idx) (IDS_THEME_FIRST + (idx))

const MdTheme MdThemes[] = {
    // --- light ---
    {"paper", TH(0), false,
     RGB(0xFF, 0xFF, 0xFF), RGB(0x33, 0x33, 0x33), RGB(0x11, 0x11, 0x11), RGB(0x0B, 0x61, 0xC9), RGB(0x0A, 0x4F, 0xA0),
     RGB(0x55, 0x55, 0x55), RGB(0xC0, 0xC0, 0xC0), RGB(0xB0, 0x00, 0x20), RGB(0xF0, 0xF0, 0xF0),
     RGB(0xF4, 0xF4, 0xF4), RGB(0x24, 0x29, 0x2E), RGB(0xC0, 0xC0, 0xC0), RGB(0xEC, 0xEC, 0xEC), RGB(0xDD, 0xDD, 0xDD),
     RGB(0xCC, 0xE4, 0xFF), RGB(0x00, 0x00, 0x00), RGB(0x66, 0x66, 0x66), RGB(0xF0, 0xF0, 0xF0),
     {RGB(0x00, 0x00, 0xC0), RGB(0xA3, 0x15, 0x15), RGB(0x09, 0x86, 0x58), RGB(0x1E, 0x7A, 0x1E), RGB(0x26, 0x7F, 0x99), RGB(0x79, 0x5E, 0x26), RGB(0x33, 0x33, 0x33), RGB(0x22, 0x86, 0x3A), RGB(0xCB, 0x24, 0x31)}},
    {"softgray", TH(1), false,
     RGB(0xF2, 0xF3, 0xF5), RGB(0x2B, 0x2B, 0x2B), RGB(0x11, 0x11, 0x11), RGB(0x0B, 0x57, 0xB0), RGB(0x09, 0x45, 0x8C),
     RGB(0x50, 0x50, 0x50), RGB(0xB8, 0xBC, 0xC2), RGB(0xA6, 0x1B, 0x2B), RGB(0xE4, 0xE6, 0xE9),
     RGB(0xE4, 0xE6, 0xE9), RGB(0x24, 0x29, 0x2E), RGB(0xB8, 0xBC, 0xC2), RGB(0xE0, 0xE3, 0xE7), RGB(0xCF, 0xD3, 0xD8),
     RGB(0xC5, 0xDD, 0xF7), RGB(0x00, 0x00, 0x00), RGB(0x5A, 0x5F, 0x66), RGB(0xE4, 0xE6, 0xE9),
     {RGB(0x00, 0x00, 0xC0), RGB(0xA3, 0x15, 0x15), RGB(0x09, 0x70, 0x50), RGB(0x1E, 0x6E, 0x1E), RGB(0x1F, 0x6F, 0x8C), RGB(0x79, 0x5E, 0x26), RGB(0x2B, 0x2B, 0x2B), RGB(0x1F, 0x7A, 0x34), RGB(0xC0, 0x22, 0x2E)}},
    {"warmsepia", TH(2), false,
     RGB(0xF6, 0xEE, 0xDD), RGB(0x4A, 0x3B, 0x2C), RGB(0x2E, 0x20, 0x18), RGB(0x23, 0x5A, 0x8C), RGB(0x1A, 0x46, 0x70),
     RGB(0x6B, 0x5A, 0x45), RGB(0xC9, 0xB8, 0x96), RGB(0x8C, 0x2B, 0x2B), RGB(0xED, 0xE2, 0xCC),
     RGB(0xEE, 0xE3, 0xCC), RGB(0x4A, 0x3B, 0x2C), RGB(0xC9, 0xB8, 0x96), RGB(0xE8, 0xDC, 0xC0), RGB(0xD8, 0xC9, 0xA8),
     RGB(0xE3, 0xD2, 0xA0), RGB(0x00, 0x00, 0x00), RGB(0x7A, 0x68, 0x50), RGB(0xED, 0xE2, 0xCC),
     {RGB(0x1B, 0x4F, 0x8C), RGB(0x9B, 0x2C, 0x2C), RGB(0x2E, 0x6A, 0x3F), RGB(0x5A, 0x50, 0x3A), RGB(0x2B, 0x63, 0x7A), RGB(0x7A, 0x5A, 0x26), RGB(0x4A, 0x3B, 0x2C), RGB(0x2E, 0x6A, 0x3F), RGB(0xA6, 0x2A, 0x2A)}},
    {"solarlight", TH(3), false,
     RGB(0xFD, 0xF6, 0xE3), RGB(0x4A, 0x5F, 0x66), RGB(0x07, 0x36, 0x42), RGB(0x1F, 0x6F, 0xA8), RGB(0x18, 0x5A, 0x8A),
     RGB(0x58, 0x6E, 0x75), RGB(0xCB, 0xC5, 0xB4), RGB(0xA1, 0x2A, 0x2A), RGB(0xEE, 0xE8, 0xD5),
     RGB(0xEE, 0xE8, 0xD5), RGB(0x07, 0x36, 0x42), RGB(0xC6, 0xC0, 0xAF), RGB(0xE7, 0xE1, 0xCE), RGB(0xD8, 0xD2, 0xBF),
     RGB(0xE3, 0xDC, 0xB8), RGB(0x00, 0x00, 0x00), RGB(0x65, 0x7B, 0x83), RGB(0xEE, 0xE8, 0xD5),
     {RGB(0x22, 0x6F, 0xB8), RGB(0xB5, 0x37, 0x1A), RGB(0x2A, 0x7A, 0x3A), RGB(0x5A, 0x6E, 0x60), RGB(0x1F, 0x7A, 0x88), RGB(0x83, 0x60, 0x00), RGB(0x4A, 0x5F, 0x66), RGB(0x2A, 0x7A, 0x3A), RGB(0xB5, 0x37, 0x1A)}},
    {"arcticlight", TH(4), false,
     RGB(0xEC, 0xEF, 0xF4), RGB(0x2E, 0x34, 0x40), RGB(0x2E, 0x34, 0x40), RGB(0x20, 0x50, 0x81), RGB(0x18, 0x40, 0x68),
     RGB(0x4C, 0x56, 0x6A), RGB(0xC2, 0xC9, 0xD6), RGB(0xA1, 0x2A, 0x2A), RGB(0xE1, 0xE6, 0xEE),
     RGB(0xE1, 0xE6, 0xEE), RGB(0x2E, 0x34, 0x40), RGB(0xC2, 0xC9, 0xD6), RGB(0xDE, 0xE3, 0xEC), RGB(0xD0, 0xD6, 0xE0),
     RGB(0xD0, 0xE0, 0xF0), RGB(0x00, 0x00, 0x00), RGB(0x5A, 0x63, 0x74), RGB(0xE1, 0xE6, 0xEE),
     {RGB(0x20, 0x66, 0x9C), RGB(0xA0, 0x33, 0x33), RGB(0x2A, 0x7A, 0x3A), RGB(0x5A, 0x63, 0x74), RGB(0x1F, 0x6F, 0x88), RGB(0x7A, 0x5A, 0x26), RGB(0x2E, 0x34, 0x40), RGB(0x2A, 0x7A, 0x3A), RGB(0xB0, 0x2A, 0x2A)}},
    // --- dark ---
    {"graphite", TH(5), true,
     RGB(0x2B, 0x2B, 0x2B), RGB(0xD6, 0xD6, 0xD6), RGB(0xFF, 0xFF, 0xFF), RGB(0x7F, 0xB4, 0xF0), RGB(0xA0, 0xC8, 0xFF),
     RGB(0xB0, 0xB0, 0xB0), RGB(0x55, 0x55, 0x55), RGB(0xF0, 0xA0, 0xA0), RGB(0x3A, 0x3A, 0x3A),
     RGB(0x23, 0x23, 0x23), RGB(0xD6, 0xD6, 0xD6), RGB(0x55, 0x55, 0x55), RGB(0x33, 0x33, 0x33), RGB(0x44, 0x44, 0x44),
     RGB(0x26, 0x4F, 0x78), RGB(0xFF, 0xFF, 0xFF), RGB(0xA0, 0xA0, 0xA0), RGB(0x3A, 0x3A, 0x3A),
     {RGB(0x56, 0x9C, 0xD6), RGB(0xCE, 0x91, 0x78), RGB(0xB5, 0xCE, 0xA8), RGB(0x9A, 0xA0, 0xA6), RGB(0x4E, 0xC9, 0xB0), RGB(0xDC, 0xDC, 0xAA), RGB(0xD6, 0xD6, 0xD6), RGB(0x4E, 0xC9, 0x4E), RGB(0xF0, 0x70, 0x70)}},
    {"midnight", TH(6), true,
     RGB(0x10, 0x1A, 0x2B), RGB(0xC9, 0xD4, 0xE5), RGB(0xFF, 0xFF, 0xFF), RGB(0x7F, 0xA8, 0xFF), RGB(0xA0, 0xC0, 0xFF),
     RGB(0xA6, 0xB2, 0xC6), RGB(0x30, 0x40, 0x5A), RGB(0xF0, 0xA0, 0xA0), RGB(0x1A, 0x28, 0x40),
     RGB(0x0B, 0x14, 0x22), RGB(0xC9, 0xD4, 0xE5), RGB(0x30, 0x40, 0x5A), RGB(0x1A, 0x28, 0x40), RGB(0x28, 0x38, 0x54),
     RGB(0x27, 0x45, 0x7A), RGB(0xFF, 0xFF, 0xFF), RGB(0x90, 0x9C, 0xB0), RGB(0x1A, 0x28, 0x40),
     {RGB(0x82, 0xAA, 0xFF), RGB(0xEC, 0xC4, 0x8D), RGB(0xF7, 0x8C, 0x6C), RGB(0x8A, 0x96, 0xAB), RGB(0x7F, 0xDB, 0xCA), RGB(0xE6, 0xD6, 0x8A), RGB(0xC9, 0xD4, 0xE5), RGB(0x6B, 0xD6, 0x6B), RGB(0xF0, 0x70, 0x70)}},
    {"solardark", TH(7), true,
     RGB(0x00, 0x2B, 0x36), RGB(0x93, 0xA1, 0xA1), RGB(0xFD, 0xF6, 0xE3), RGB(0x2A, 0xA0, 0xE0), RGB(0x5A, 0xB8, 0xF0),
     RGB(0x83, 0x94, 0x96), RGB(0x0B, 0x3A, 0x46), RGB(0xE6, 0x9A, 0x8C), RGB(0x07, 0x3A, 0x46),
     RGB(0x00, 0x24, 0x2E), RGB(0x93, 0xA1, 0xA1), RGB(0x0B, 0x3A, 0x46), RGB(0x07, 0x3A, 0x46), RGB(0x0B, 0x46, 0x54),
     RGB(0x0A, 0x4A, 0x5C), RGB(0xFD, 0xF6, 0xE3), RGB(0x83, 0x94, 0x96), RGB(0x07, 0x3A, 0x46),
     {RGB(0x33, 0xA0, 0xE0), RGB(0xD0, 0x76, 0x5A), RGB(0x8A, 0xC0, 0x6A), RGB(0x8A, 0x96, 0x96), RGB(0x3A, 0xB8, 0xA0), RGB(0xC8, 0xB8, 0x4A), RGB(0x93, 0xA1, 0xA1), RGB(0x6B, 0xD6, 0x6B), RGB(0xE0, 0x60, 0x50)}},
    {"nordicdark", TH(8), true,
     RGB(0x2E, 0x34, 0x40), RGB(0xD8, 0xDE, 0xE9), RGB(0xEC, 0xEF, 0xF4), RGB(0x88, 0xC0, 0xD0), RGB(0xA0, 0xD8, 0xE8),
     RGB(0xB8, 0xC0, 0xCE), RGB(0x43, 0x4C, 0x5E), RGB(0xE6, 0x9A, 0x8C), RGB(0x3B, 0x42, 0x52),
     RGB(0x29, 0x2E, 0x39), RGB(0xD8, 0xDE, 0xE9), RGB(0x43, 0x4C, 0x5E), RGB(0x3B, 0x42, 0x52), RGB(0x4C, 0x56, 0x6A),
     RGB(0x43, 0x4C, 0x5E), RGB(0xEC, 0xEF, 0xF4), RGB(0x9A, 0xA4, 0xB6), RGB(0x3B, 0x42, 0x52),
     {RGB(0x81, 0xA1, 0xC1), RGB(0xA3, 0xBE, 0x8C), RGB(0xB4, 0x8E, 0xAD), RGB(0x90, 0x9A, 0xAE), RGB(0x8F, 0xBC, 0xBB), RGB(0xEB, 0xCB, 0x8B), RGB(0xD8, 0xDE, 0xE9), RGB(0xA3, 0xBE, 0x8C), RGB(0xBF, 0x61, 0x6A)}},
    {"hicontrast", TH(9), true,
     RGB(0x00, 0x00, 0x00), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0x00, 0xFF, 0xFF), RGB(0x80, 0xFF, 0xFF),
     RGB(0xFF, 0xFF, 0x80), RGB(0x80, 0x80, 0x80), RGB(0xFF, 0xD0, 0xD0), RGB(0x18, 0x18, 0x18),
     RGB(0x10, 0x10, 0x10), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0x30, 0x30, 0x30), RGB(0x80, 0x80, 0x80),
     RGB(0x00, 0x60, 0x80), RGB(0xFF, 0xFF, 0xFF), RGB(0xE0, 0xE0, 0xE0), RGB(0x18, 0x18, 0x18),
     {RGB(0x60, 0xC0, 0xFF), RGB(0xFF, 0xC0, 0x80), RGB(0x90, 0xFF, 0x90), RGB(0xC0, 0xE0, 0xFF), RGB(0x60, 0xFF, 0xE0), RGB(0xFF, 0xFF, 0x60), RGB(0xFF, 0xFF, 0xFF), RGB(0x60, 0xFF, 0x60), RGB(0xFF, 0x80, 0x80)}},
};
const int MdThemeCount = (int)(sizeof(MdThemes) / sizeof(MdThemes[0]));

const MdTheme* MdThemeById(const char* id)
{
    if (id != NULL)
        for (int i = 0; i < MdThemeCount; i++)
            if (lstrcmpA(MdThemes[i].id, id) == 0)
                return &MdThemes[i];
    return NULL;
}
const MdTheme* MdThemeDefault(bool dark) { return dark ? MdThemeById("graphite") : MdThemeById("paper"); }
int MdThemeIndex(const MdTheme* t)
{
    for (int i = 0; i < MdThemeCount; i++)
        if (&MdThemes[i] == t)
            return i;
    return 0;
}

// ==========================================================================
// Encoding detection (FR-050)
// ==========================================================================

static bool ValidUtf8(const BYTE* d, size_t n)
{
    size_t i = 0;
    while (i < n)
    {
        BYTE c = d[i];
        if (c < 0x80) { i++; continue; }
        int extra;
        if ((c & 0xE0) == 0xC0) extra = 1;
        else if ((c & 0xF0) == 0xE0) extra = 2;
        else if ((c & 0xF8) == 0xF0) extra = 3;
        else return false;
        if (i + extra >= n) return false;
        for (int k = 1; k <= extra; k++)
            if ((d[i + k] & 0xC0) != 0x80) return false;
        i += extra + 1;
    }
    return true;
}

static void FromCP(UINT cp, const BYTE* d, size_t n, std::wstring& out)
{
    if (n == 0) { out.clear(); return; }
    int need = MultiByteToWideChar(cp, 0, (LPCCH)d, (int)n, NULL, 0);
    out.resize(need > 0 ? need : 0);
    if (need > 0)
        MultiByteToWideChar(cp, 0, (LPCCH)d, (int)n, &out[0], need);
}

MdEncoding MdDetectDecode(const BYTE* data, size_t len, std::wstring& out)
{
    out.clear();
    if (data == NULL || len == 0) return MDENC_UTF8;

    if (len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
    {
        FromCP(CP_UTF8, data + 3, len - 3, out);
        return MDENC_UTF8BOM;
    }
    if (len >= 2 && data[0] == 0xFF && data[1] == 0xFE)
    {
        out.assign((const wchar_t*)(data + 2), (len - 2) / 2);
        return MDENC_UTF16LE;
    }
    if (len >= 2 && data[0] == 0xFE && data[1] == 0xFF)
    {
        out.resize((len - 2) / 2);
        for (size_t i = 0; i < out.size(); i++)
            out[i] = (wchar_t)((data[2 + i * 2] << 8) | data[2 + i * 2 + 1]);
        return MDENC_UTF16BE;
    }
    // binary sniff: a NUL byte in the first 4 KB and not valid UTF-8
    size_t scan = len < 4096 ? len : 4096;
    bool hasNul = false;
    for (size_t i = 0; i < scan; i++)
        if (data[i] == 0) { hasNul = true; break; }
    if (hasNul) return MDENC_BINARY;

    if (ValidUtf8(data, len))
    {
        FromCP(CP_UTF8, data, len, out);
        return MDENC_UTF8;
    }
    FromCP(CP_ACP, data, len, out); // legacy fallback (CP1250 on Czech Windows)
    return MDENC_ANSI;
}

// ==========================================================================
// GitHub-style slug
// ==========================================================================

std::wstring MdSlug(const std::wstring& s)
{
    std::wstring r;
    for (wchar_t c : s)
    {
        if (c == L' ' || c == L'\t') r += L'-';
        else if (iswalnum(c) || c == L'-' || c == L'_') r += (wchar_t)towlower(c);
        // else: drop
    }
    return r;
}

// ==========================================================================
// RTF emission
// ==========================================================================

namespace
{
// color-table indices (1-based; must match BuildHeader order)
enum
{
    CF_BODY = 1, CF_HEAD, CF_LINK, CF_LINKA, CF_QUOTE, CF_QACC, CF_INLFG, CF_INLBG,
    CF_CODEBG, CF_CODE, CF_TBORDER, CF_THEADBG, CF_RULE, CF_IMG,
    CF_KW, CF_STR, CF_NUM, CF_CMT, CF_TYPE, CF_FN, CF_OP, CF_ADD, CF_DEL
};

struct Ctx
{
    std::string* out;
    long cp;                 // approximate RichEdit char position
    const MdTheme* th;
    std::wstring docDir;
    MdRenderResult* res;
    size_t depth;
};

void raw(Ctx& c, const char* s) { c.out->append(s); }

void appColor(std::string& s, COLORREF cr)
{
    char b[48];
    sprintf_s(b, "\\red%d\\green%d\\blue%d;", GetRValue(cr), GetGValue(cr), GetBValue(cr));
    s += b;
}

void BuildHeader(Ctx& c)
{
    std::string& s = *c.out;
    s = "{\\rtf1\\ansi\\ansicpg1252\\uc1\\deff0"
        "{\\fonttbl{\\f0\\fswiss\\fcharset0 Segoe UI;}{\\f1\\fmodern\\fcharset0 Consolas;}}"
        "{\\colortbl;";
    const MdTheme* t = c.th;
    appColor(s, t->body); appColor(s, t->heading); appColor(s, t->link); appColor(s, t->linkActive);
    appColor(s, t->quoteText); appColor(s, t->quoteAccent); appColor(s, t->codeInlineFg); appColor(s, t->codeInlineBg);
    appColor(s, t->codeBg); appColor(s, t->codeText); appColor(s, t->tableBorder); appColor(s, t->tableHeadBg);
    appColor(s, t->rule); appColor(s, t->imgFg);
    appColor(s, t->syn.kw); appColor(s, t->syn.str); appColor(s, t->syn.num); appColor(s, t->syn.cmt);
    appColor(s, t->syn.type); appColor(s, t->syn.fn); appColor(s, t->syn.op); appColor(s, t->syn.add); appColor(s, t->syn.del);
    s += "}\n\\f0\\fs22\\cf1 ";
}

// escape + append a UTF-16 string as RTF text, tracking cp
void text(Ctx& c, const std::wstring& w, size_t from = 0, size_t to = (size_t)-1)
{
    std::string& s = *c.out;
    if (to == (size_t)-1) to = w.size();
    char b[16];
    for (size_t i = from; i < to; i++)
    {
        wchar_t ch = w[i];
        if (ch == L'\\') { s += "\\\\"; c.cp++; }
        else if (ch == L'{') { s += "\\{"; c.cp++; }
        else if (ch == L'}') { s += "\\}"; c.cp++; }
        else if (ch == L'\t') { s += "\\tab "; c.cp++; }
        else if (ch >= 0x20 && ch < 0x7F) { s += (char)ch; c.cp++; }
        else
        {
            sprintf_s(b, "\\u%d?", (int)(short)ch);
            s += b; c.cp++;
        }
    }
}
void par(Ctx& c) { raw(c, "\\par\n"); c.cp++; }
void lineBreak(Ctx& c) { raw(c, "\\line "); c.cp++; }

void cf(Ctx& c, int idx) { char b[16]; sprintf_s(b, "\\cf%d ", idx); raw(c, b); }
void hl(Ctx& c, int idx) { char b[16]; sprintf_s(b, "\\highlight%d ", idx); raw(c, b); }

// ---- inline parsing ----

// resolve a handful of HTML entities; returns char (or 0 if unknown)
wchar_t Entity(const std::wstring& name)
{
    if (name.empty()) return 0;
    if (name[0] == L'#')
    {
        int base = 10; size_t i = 1;
        if (name.size() > 1 && (name[1] == L'x' || name[1] == L'X')) { base = 16; i = 2; }
        long v = wcstol(name.c_str() + i, NULL, base);
        if (v > 0 && v < 0x110000) return (wchar_t)v;
        return 0;
    }
    struct E { const wchar_t* n; wchar_t c; };
    static const E tab[] = {{L"amp", L'&'}, {L"lt", L'<'}, {L"gt", L'>'}, {L"quot", L'"'}, {L"apos", L'\''}, {L"copy", 0xA9}, {L"reg", 0xAE}, {L"trade", 0x2122}, {L"nbsp", 0xA0}, {L"mdash", 0x2014}, {L"ndash", 0x2013}, {L"hellip", 0x2026}, {L"bull", 0x2022}, {L"rarr", 0x2192}, {L"larr", 0x2190}, {L"check", 0x2713}};
    for (const E& e : tab)
        if (name == e.n) return e.c;
    return 0;
}

bool IsUrlScheme(const std::wstring& u)
{
    size_t p = u.find(L':');
    if (p == std::wstring::npos) return false;
    std::wstring s = u.substr(0, p);
    for (wchar_t& ch : s) ch = (wchar_t)towlower(ch);
    return s == L"http" || s == L"https" || s == L"mailto" || s == L"ftp";
}

void emitLink(Ctx& c, const std::wstring& disp, const std::wstring& url);
void emitInline(Ctx& c, const std::wstring& s);

// emit inline with an active char format wrapper (bold/italic/strike/code)
void emitWrapped(Ctx& c, const std::wstring& inner, const char* on, const char* off)
{
    raw(c, on);
    emitInline(c, inner);
    raw(c, off);
}

void emitInline(Ctx& c, const std::wstring& s)
{
    size_t i = 0, n = s.size();
    std::wstring plain;
    auto flush = [&]() { if (!plain.empty()) { text(c, plain); plain.clear(); } };

    while (i < n)
    {
        wchar_t ch = s[i];
        // backslash escape
        if (ch == L'\\' && i + 1 < n)
        {
            wchar_t nx = s[i + 1];
            if (!iswalnum(nx)) { plain += nx; i += 2; continue; }
        }
        // code span
        if (ch == L'`')
        {
            size_t run = 0; while (i + run < n && s[i + run] == L'`') run++;
            std::wstring fence(run, L'`');
            size_t close = s.find(fence, i + run);
            if (close != std::wstring::npos)
            {
                flush();
                std::wstring code = s.substr(i + run, close - (i + run));
                raw(c, "{\\f1"); cf(c, CF_INLFG); hl(c, CF_INLBG);
                text(c, code);
                raw(c, "}");
                i = close + run; continue;
            }
        }
        // image ![alt](src)
        if (ch == L'!' && i + 1 < n && s[i + 1] == L'[')
        {
            size_t rb = s.find(L']', i + 2);
            if (rb != std::wstring::npos && rb + 1 < n && s[rb + 1] == L'(')
            {
                size_t rp = s.find(L')', rb + 2);
                if (rp != std::wstring::npos)
                {
                    flush();
                    std::wstring alt = s.substr(i + 2, rb - (i + 2));
                    std::wstring src = s.substr(rb + 2, rp - (rb + 2));
                    raw(c, "{\\f1"); cf(c, CF_IMG);
                    std::wstring ph = L"\x25A1 "; // white square: image placeholder marker
                    ph += alt.empty() ? src : alt;
                    ph += L" ";
                    text(c, ph);
                    raw(c, "}");
                    i = rp + 1; continue;
                }
            }
        }
        // link [text](url)
        if (ch == L'[')
        {
            size_t rb = s.find(L']', i + 1);
            if (rb != std::wstring::npos && rb + 1 < n && s[rb + 1] == L'(')
            {
                size_t rp = s.find(L')', rb + 2);
                if (rp != std::wstring::npos)
                {
                    flush();
                    std::wstring disp = s.substr(i + 1, rb - (i + 1));
                    std::wstring url = s.substr(rb + 2, rp - (rb + 2));
                    emitLink(c, disp, url);
                    i = rp + 1; continue;
                }
            }
        }
        // autolink <url>
        if (ch == L'<')
        {
            size_t gt = s.find(L'>', i + 1);
            if (gt != std::wstring::npos)
            {
                std::wstring inner = s.substr(i + 1, gt - (i + 1));
                if (IsUrlScheme(inner) || inner.find(L'@') != std::wstring::npos)
                {
                    flush();
                    emitLink(c, inner, inner);
                    i = gt + 1; continue;
                }
            }
        }
        // strong / emphasis / strike
        if (ch == L'*' || ch == L'_' || ch == L'~')
        {
            bool dbl = (i + 1 < n && s[i + 1] == ch);
            std::wstring marker = dbl ? std::wstring(2, ch) : std::wstring(1, ch);
            size_t close = s.find(marker, i + marker.size());
            if (close != std::wstring::npos && close > i + marker.size())
            {
                std::wstring inner = s.substr(i + marker.size(), close - (i + marker.size()));
                flush();
                if (ch == L'~' && dbl) emitWrapped(c, inner, "{\\strike ", "}");
                else if (dbl) emitWrapped(c, inner, "{\\b ", "}");
                else emitWrapped(c, inner, "{\\i ", "}");
                i = close + marker.size(); continue;
            }
        }
        // entity
        if (ch == L'&')
        {
            size_t sc = s.find(L';', i + 1);
            if (sc != std::wstring::npos && sc - i <= 12)
            {
                wchar_t e = Entity(s.substr(i + 1, sc - (i + 1)));
                if (e) { plain += e; i = sc + 1; continue; }
            }
        }
        plain += ch; i++;
    }
    flush();
}

void emitLink(Ctx& c, const std::wstring& disp, const std::wstring& url)
{
    int id = (int)c.res->links.size();
    MdLinkEntry le;
    bool internal = (!url.empty() && url[0] == L'#');
    le.internalAnchor = internal;
    le.url = internal ? url.substr(1) : url;
    c.res->links.push_back(le);

    long cpMin = c.cp;
    raw(c, "{"); cf(c, CF_LINK); raw(c, "\\ul ");
    emitInline(c, disp.empty() ? url : disp);
    raw(c, "\\ul0}");
    long cpMax = c.cp;

    MdLinkRange r; r.cpMin = cpMin; r.cpMax = cpMax; r.linkId = id;
    c.res->linkRanges.push_back(r);
}

// ---- block helpers ----

void headingParProps(Ctx& c, int level)
{
    int fs = 22;
    switch (level)
    {
    case 1: fs = 36; break;
    case 2: fs = 30; break;
    case 3: fs = 26; break;
    default: fs = 24; break;
    }
    char b[64]; sprintf_s(b, "\\pard\\sb160\\sa80\\b\\fs%d", fs);
    raw(c, b); cf(c, CF_HEAD); raw(c, " ");
}

std::wstring trimRight(const std::wstring& s)
{
    size_t e = s.size();
    while (e > 0 && (s[e - 1] == L' ' || s[e - 1] == L'\t' || s[e - 1] == L'\r')) e--;
    return s.substr(0, e);
}
std::wstring trim(const std::wstring& s)
{
    size_t b = 0; while (b < s.size() && (s[b] == L' ' || s[b] == L'\t')) b++;
    return trimRight(s.substr(b));
}

bool isBlank(const std::wstring& s)
{
    for (wchar_t c : s) if (c != L' ' && c != L'\t' && c != L'\r') return false;
    return true;
}

bool isThematicBreak(const std::wstring& s)
{
    std::wstring t = trim(s);
    if (t.size() < 3) return false;
    wchar_t c = t[0];
    if (c != L'-' && c != L'*' && c != L'_') return false;
    int cnt = 0;
    for (wchar_t x : t) { if (x == c) cnt++; else if (x != L' ') return false; }
    return cnt >= 3;
}

int atxLevel(const std::wstring& s, std::wstring& content)
{
    size_t i = 0; while (i < s.size() && s[i] == L' ') i++;
    int lvl = 0; while (i < s.size() && s[i] == L'#') { lvl++; i++; }
    if (lvl < 1 || lvl > 6) return 0;
    if (i < s.size() && s[i] != L' ') return 0;
    content = trim(s.substr(i));
    // strip trailing #'s
    size_t e = content.size(); while (e > 0 && content[e - 1] == L'#') e--;
    content = trimRight(content.substr(0, e));
    return lvl;
}

// unordered/ordered list marker; returns marker width (chars consumed) or 0
int listMarker(const std::wstring& s, bool& ordered, int& start, int& indent)
{
    size_t i = 0; indent = 0;
    while (i < s.size() && s[i] == L' ') { i++; indent++; }
    if (i >= s.size()) return 0;
    if ((s[i] == L'-' || s[i] == L'*' || s[i] == L'+') && i + 1 < s.size() && s[i + 1] == L' ')
    {
        ordered = false; start = 0; return (int)i + 2;
    }
    // ordered: digits + '.' or ')'
    size_t j = i; while (j < s.size() && iswdigit(s[j])) j++;
    if (j > i && j < s.size() && (s[j] == L'.' || s[j] == L')') && j + 1 < s.size() && s[j + 1] == L' ')
    {
        ordered = true; start = _wtoi(s.substr(i, j - i).c_str()); return (int)j + 2;
    }
    return 0;
}
} // namespace

// (HlRun / HighlightCode declared in render.h)

namespace
{
void emitCodeBlock(Ctx& c, const std::wstring& code, const std::string& lang, bool frontMatter)
{
    raw(c, "\\pard\\sb80\\sa80\\li120\\f1\\fs20 ");
    cf(c, CF_CODE); hl(c, CF_CODEBG);
    if (frontMatter) { text(c, L"\x2500\x2500 metadata \x2500\x2500"); lineBreak(c); }

    std::vector<HlRun> runs;
    if (!frontMatter) HighlightCode(code, lang, runs);

    // split into lines, apply runs per whole text with \line between rows
    if (runs.empty())
    {
        size_t start = 0;
        for (size_t i = 0; i <= code.size(); i++)
        {
            if (i == code.size() || code[i] == L'\n')
            {
                text(c, code, start, i > start && code[i > 0 ? i - 1 : 0] == L'\r' ? i - 1 : i);
                if (i < code.size()) lineBreak(c);
                start = i + 1;
            }
        }
    }
    else
    {
        // emit char by char honoring runs and newlines
        size_t ri = 0;
        for (size_t i = 0; i < code.size();)
        {
            if (code[i] == L'\n') { lineBreak(c); i++; continue; }
            if (code[i] == L'\r') { i++; continue; }
            // find run covering i
            int col = 0;
            for (const HlRun& r : runs)
                if (i >= r.start && i < r.start + r.len) { col = r.colorCf; break; }
            size_t j = i;
            while (j < code.size() && code[j] != L'\n' && code[j] != L'\r')
            {
                int col2 = 0;
                for (const HlRun& r : runs)
                    if (j >= r.start && j < r.start + r.len) { col2 = r.colorCf; break; }
                if (col2 != col) break;
                j++;
            }
            if (col) cf(c, col); else cf(c, CF_CODE);
            text(c, code, i, j);
            i = j;
        }
    }
    raw(c, "\\highlight0 "); par(c);
    raw(c, "\\pard\\f0\\fs22 "); cf(c, CF_BODY);
}

void emitHr(Ctx& c)
{
    raw(c, "\\pard\\sb80\\sa80\\brdrb\\brdrs\\brdrw10\\brdrcf13 ");
    par(c);
    raw(c, "\\pard\\fs22 "); cf(c, CF_BODY);
}

void emitTable(Ctx& c, const std::vector<std::wstring>& rows)
{
    // rows[0] header, rows[1] separator, rows[2..] body. Render monospace, aligned.
    std::vector<std::vector<std::wstring>> cells;
    for (const std::wstring& r : rows)
    {
        std::wstring t = trim(r);
        if (!t.empty() && t[0] == L'|') t = t.substr(1);
        if (!t.empty() && t.back() == L'|') t.pop_back();
        std::vector<std::wstring> cols; std::wstring cur;
        for (size_t i = 0; i < t.size(); i++)
        {
            if (t[i] == L'\\' && i + 1 < t.size()) { cur += t[i + 1]; i++; }
            else if (t[i] == L'|') { cols.push_back(trim(cur)); cur.clear(); }
            else cur += t[i];
        }
        cols.push_back(trim(cur));
        cells.push_back(cols);
    }
    if (cells.size() < 2) return;
    size_t ncol = 0; for (auto& r : cells) ncol = max(ncol, r.size());
    std::vector<size_t> w(ncol, 3);
    for (size_t ri = 0; ri < cells.size(); ri++)
    {
        if (ri == 1) continue; // separator
        for (size_t ci = 0; ci < cells[ri].size(); ci++)
            w[ci] = max(w[ci], cells[ri][ci].size());
    }
    raw(c, "\\pard\\sb80\\sa80\\f1\\fs20 "); cf(c, CF_CODE);
    for (size_t ri = 0; ri < cells.size(); ri++)
    {
        std::wstring line;
        if (ri == 1)
        {
            for (size_t ci = 0; ci < ncol; ci++) { line += std::wstring(w[ci], L'-'); line += L"  "; }
        }
        else
        {
            for (size_t ci = 0; ci < ncol; ci++)
            {
                std::wstring v = ci < cells[ri].size() ? cells[ri][ci] : L"";
                line += v; if (v.size() < w[ci]) line += std::wstring(w[ci] - v.size(), L' ');
                line += L"  ";
            }
        }
        text(c, trimRight(line));
        if (ri + 1 < cells.size()) lineBreak(c);
    }
    par(c);
    raw(c, "\\pard\\f0\\fs22 "); cf(c, CF_BODY);
}
} // namespace

// ==========================================================================
// Main render
// ==========================================================================

void MdRenderMarkdown(const std::wstring& md, const MdTheme& theme,
                      const std::wstring& docDir, MdRenderResult& out,
                      const MdRenderLimits& lim)
{
    out.rtf.clear(); out.links.clear(); out.anchors.clear(); out.linkRanges.clear();

    Ctx c; c.out = &out.rtf; c.cp = 0; c.th = &theme; c.docDir = docDir; c.res = &out; c.depth = 0;
    BuildHeader(c);

    // split into lines
    std::vector<std::wstring> lines;
    {
        std::wstring cur;
        for (wchar_t ch : md)
        {
            if (ch == L'\n') { lines.push_back(cur); cur.clear(); }
            else if (ch != L'\r') cur += ch;
        }
        lines.push_back(cur);
    }

    size_t i = 0, N = lines.size();

    // YAML front matter
    if (N > 0 && trim(lines[0]) == L"---")
    {
        size_t j = 1; std::wstring fm;
        while (j < N && trim(lines[j]) != L"---") { fm += lines[j]; fm += L'\n'; j++; }
        if (j < N) { emitCodeBlock(c, fm, "", true); i = j + 1; }
    }

    while (i < N)
    {
        const std::wstring& ln = lines[i];
        if (isBlank(ln)) { i++; continue; }

        // ATX heading
        std::wstring hc; int lvl = atxLevel(ln, hc);
        if (lvl > 0)
        {
            MdAnchorEntry a; a.slug = MdSlug(hc); a.charPos = c.cp; out.anchors.push_back(a);
            headingParProps(c, lvl);
            emitInline(c, hc);
            raw(c, "\\b0 "); par(c);
            raw(c, "\\pard\\fs22 "); cf(c, CF_BODY);
            i++; continue;
        }

        // fenced code block
        std::wstring t = trim(ln);
        if (t.size() >= 3 && (t.substr(0, 3) == L"```" || t.substr(0, 3) == L"~~~"))
        {
            std::wstring fence = t.substr(0, 3);
            std::wstring info = trim(t.substr(3));
            std::string lang;
            for (wchar_t ch : info) { if (ch == L' ') break; lang += (char)towlower(ch); }
            std::wstring code; size_t j = i + 1;
            while (j < N && trim(lines[j]).substr(0, 3) != fence) { code += lines[j]; code += L'\n'; j++; }
            if (!code.empty() && code.back() == L'\n') code.pop_back();
            // degrade math/mermaid to a labeled plain block (FR-014)
            emitCodeBlock(c, code, lang, false);
            i = (j < N) ? j + 1 : j; continue;
        }

        // thematic break
        if (isThematicBreak(ln)) { emitHr(c); i++; continue; }

        // table: current line has '|' and next line is a separator (---|---)
        if (i + 1 < N && ln.find(L'|') != std::wstring::npos)
        {
            std::wstring sep = trim(lines[i + 1]);
            bool isSep = !sep.empty();
            for (wchar_t ch : sep) if (ch != L'-' && ch != L':' && ch != L'|' && ch != L' ') { isSep = false; break; }
            if (isSep && sep.find(L'-') != std::wstring::npos)
            {
                std::vector<std::wstring> rows; rows.push_back(ln); rows.push_back(lines[i + 1]);
                size_t j = i + 2;
                while (j < N && !isBlank(lines[j]) && lines[j].find(L'|') != std::wstring::npos) { rows.push_back(lines[j]); j++; }
                emitTable(c, rows);
                i = j; continue;
            }
        }

        // blockquote
        if (t.size() > 0 && t[0] == L'>')
        {
            int qd = 0; { std::wstring x = t; while (!x.empty() && x[0] == L'>') { qd++; x = trim(x.substr(1)); } }
            std::wstring qtext;
            size_t j = i;
            while (j < N && !isBlank(lines[j]) && trim(lines[j]).size() && trim(lines[j])[0] == L'>')
            {
                std::wstring x = trim(lines[j]);
                while (!x.empty() && x[0] == L'>') { x = x.substr(1); if (!x.empty() && x[0] == L' ') x = x.substr(1); }
                if (!qtext.empty()) qtext += L' ';
                qtext += x; j++;
            }
            char b[48]; sprintf_s(b, "\\pard\\li%d\\sb60\\sa60 ", 360 * (qd > 0 ? qd : 1));
            raw(c, b); cf(c, CF_QUOTE); raw(c, "\\i ");
            emitInline(c, qtext);
            raw(c, "\\i0 "); par(c);
            raw(c, "\\pard\\fs22 "); cf(c, CF_BODY);
            i = j; continue;
        }

        // list
        bool ord; int start, indent;
        if (listMarker(ln, ord, start, indent) > 0)
        {
            int num = ord ? start : 0;
            size_t j = i;
            while (j < N && !isBlank(lines[j]))
            {
                bool o2; int st2, ind2;
                int mw = listMarker(lines[j], o2, st2, ind2);
                if (mw == 0) break;
                std::wstring item = lines[j].substr(mw);
                // task list checkbox
                std::wstring itemTrim = trim(item);
                std::wstring prefix;
                if (itemTrim.size() >= 3 && itemTrim[0] == L'[' && itemTrim[2] == L']')
                {
                    if (itemTrim[1] == L' ') { prefix = L"\x2610 "; item = itemTrim.substr(3); }
                    else if (itemTrim[1] == L'x' || itemTrim[1] == L'X') { prefix = L"\x2611 "; item = itemTrim.substr(3); }
                }
                int li = 360 + (ind2 >= 2 ? 360 : 0);
                char b[48]; sprintf_s(b, "\\pard\\li%d\\fi-200\\sb20\\sa20\\fs22 ", li); raw(c, b); cf(c, CF_BODY);
                if (o2) { wchar_t nb[16]; num = (num ? num : st2); _snwprintf_s(nb, _TRUNCATE, L"%d. ", num++); text(c, nb); }
                else text(c, L"\x2022  ");
                if (!prefix.empty()) text(c, prefix);
                emitInline(c, trim(item));
                par(c);
                j++;
            }
            raw(c, "\\pard\\fs22 "); cf(c, CF_BODY);
            i = j; continue;
        }

        // raw HTML block (inert literal) - a line starting with '<' tag
        // paragraph (join soft-wrapped lines; blank line or block start ends it)
        {
            std::wstring para = trimRight(ln);
            size_t j = i + 1;
            while (j < N && !isBlank(lines[j]))
            {
                std::wstring nx = lines[j];
                std::wstring nt = trim(nx);
                std::wstring dummy;
                if (atxLevel(nx, dummy) || isThematicBreak(nx) ||
                    (nt.size() >= 3 && (nt.substr(0, 3) == L"```" || nt.substr(0, 3) == L"~~~")) ||
                    (nt.size() && nt[0] == L'>'))
                    break;
                bool o3; int s3, i3;
                if (listMarker(nx, o3, s3, i3) > 0) break;
                // hard break: line ends with two spaces
                if (para.size() >= 2 && ln.size() >= 2) {}
                para += L' '; para += trimRight(nx); j++;
            }
            raw(c, "\\pard\\sb40\\sa40\\fs22 "); cf(c, CF_BODY);
            emitInline(c, para);
            par(c);
            i = j;
        }
    }

    raw(c, "}");
}
