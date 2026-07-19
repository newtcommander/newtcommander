// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Standalone unit test for the mdview Markdown->HTML generator (htmlgen.cpp +
// md4c). Engine-independent: exercises the pure transformation without
// WebView2. Build via specs/021-mdview-html-renderer/quickstart.md.
//
// Asserts structural properties of the generated HTML across the feature's
// user stories (tables/alignment, code+lang, slugs, raw-HTML pass-through,
// text escaping / XSS boundary, local image rewrite, remote-image block).

#include <windows.h>
#include <string>
#include <cstdio>
#include "render.h"
#include "htmlgen.h"

static int g_pass = 0, g_fail = 0;

static void Check(const char* name, bool cond)
{
    printf(cond ? "  [PASS] %s\n" : "  [FAIL] %s\n", name);
    if (cond) g_pass++; else g_fail++;
}

static bool Has(const std::string& hay, const char* needle)
{
    return hay.find(needle) != std::string::npos;
}

static std::string Gen(const std::string& md, const std::wstring& docDir = L"C:\\docs",
                       const std::wstring& find = L"", bool allowRemote = false,
                       MdHtmlResult* keep = nullptr)
{
    const MdTheme* t = MdThemeById("paper");
    MdHtmlResult local;
    MdHtmlResult& r = keep ? *keep : local;
    MdRenderHtml(md, *t, docDir, r, find, allowRemote);
    return r.html;
}

int main()
{
    // US1: GFM table with column alignment
    {
        std::string h = Gen("| A | B | C |\n|:--|:-:|--:|\n| 1 | 2 | 3 |\n");
        Check("table renders as grid", Has(h, "<table>") && Has(h, "<th"));
        Check("table left align", Has(h, "text-align:left"));
        Check("table center align", Has(h, "text-align:center"));
        Check("table right align", Has(h, "text-align:right"));
    }
    // US1: heading slug
    {
        std::string h = Gen("# Hello World\n");
        Check("heading has slug id", Has(h, "id=\"hello-world\""));
        Check("heading tag", Has(h, "<h1"));
    }
    // US1: fenced code block with language + highlight class hooks
    {
        std::string h = Gen("```c\nint x = 42;\n```\n");
        Check("code block emitted", Has(h, "<pre><code"));
        Check("code language class", Has(h, "language-c"));
    }
    // US1: lists + task list
    {
        std::string h = Gen("- a\n- b\n\n1. one\n2. two\n\n- [x] done\n- [ ] todo\n");
        Check("unordered list", Has(h, "<ul>"));
        Check("ordered list", Has(h, "<ol>"));
        Check("task checkbox", Has(h, "type=\"checkbox\"") && Has(h, "checked"));
    }
    // US2 / escaping: literal '<' in text must be escaped (XSS boundary)
    {
        std::string h = Gen("a < b and c > d & e\n");
        Check("text '<' escaped", Has(h, "&lt;"));
        Check("text '&' escaped", Has(h, "&amp;"));
    }
    // US4: embedded raw HTML rendered verbatim (safety is the engine lockdown)
    {
        std::string h = Gen("Press <kbd>Esc</kbd> to close.\n");
        Check("inline raw HTML passed through", Has(h, "<kbd>Esc</kbd>"));
    }
    // US4 + US2: a raw HTML <script> block is passed through verbatim by the
    // generator (NOT sanitized); inertness is enforced by the WebView2 lockdown.
    {
        std::string h = Gen("<script>window.x=1</script>\n");
        Check("raw <script> block passed through (inert at engine)", Has(h, "<script>window.x=1</script>"));
        // and it is NOT executed here (pure string) - documented design
    }
    // US3: local relative image is rewritten to the interceptor origin
    {
        MdHtmlResult r;
        std::string h = Gen("![alt](pic.png)\n", L"C:\\docs", L"", false, &r);
        Check("local image rewritten", Has(h, "https://mdview.invalid/img/0"));
        Check("local image recorded", r.images.size() == 1 && r.images[0].kind == MdImageRef::Local);
    }
    // US3: remote image blocked by default (placeholder, not fetched)
    {
        MdHtmlResult r;
        std::string h = Gen("![x](http://example.com/x.png)\n", L"C:\\docs", L"", false, &r);
        Check("remote image blocked -> placeholder", Has(h, "md-imgph"));
        Check("remote image not listed (no fetch)", r.images.empty());
        Check("remote image no <img src=http", !Has(h, "src=\"http://example.com"));
    }
    // US3: with consent, remote image becomes servable
    {
        MdHtmlResult r;
        std::string h = Gen("![x](http://example.com/x.png)\n", L"C:\\docs", L"", true, &r);
        Check("remote image with consent -> servable", Has(h, "https://mdview.invalid/img/0") &&
                                                            r.images.size() == 1 &&
                                                            r.images[0].kind == MdImageRef::Remote);
    }
    // US5: script-free find marks matches
    {
        MdHtmlResult r;
        std::string h = Gen("alpha beta alpha gamma alpha\n", L"C:\\docs", L"alpha", false, &r);
        Check("find marks emitted", Has(h, "<mark id=\"mdfind-0\">") && r.matchCount == 3);
    }
    // document wrapper + theme CSS present
    {
        std::string h = Gen("hi\n");
        Check("doctype + article wrapper", Has(h, "<!doctype html>") && Has(h, "markdown-body"));
        Check("theme CSS variables", Has(h, "--bg:") && Has(h, ".hl-kw"));
        Check("reading measure", Has(h, "max-width:46rem"));
    }

    printf("\n=== htmlgen test: %d passed, %d failed ===\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
