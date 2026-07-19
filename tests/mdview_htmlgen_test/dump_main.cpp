// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// dump_main.cpp - reads a Markdown file (arg1) and writes the generated HTML
// (arg2) using the mdview htmlgen pipeline. Dev tool to visually inspect the
// rendered output outside of Salamander/WebView2.

#include <windows.h>
#include <cstdio>
#include <string>
#include <vector>
#include "render.h"
#include "htmlgen.h"

int main(int argc, char** argv)
{
    if (argc < 3) { printf("usage: dump <in.md> <out.html> [theme]\n"); return 2; }
    FILE* f = fopen(argv[1], "rb");
    if (!f) { printf("cannot open %s\n", argv[1]); return 1; }
    std::string md;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) md.append(buf, n);
    fclose(f);
    // strip a UTF-8 BOM if present
    if (md.size() >= 3 && (unsigned char)md[0] == 0xEF && (unsigned char)md[1] == 0xBB && (unsigned char)md[2] == 0xBF)
        md.erase(0, 3);

    const MdTheme* t = MdThemeById(argc >= 4 ? argv[3] : "paper");
    if (!t) t = MdThemeDefault(false);
    MdHtmlResult r;
    MdRenderHtml(md, *t, L"C:\\docs", r, L"", false);

    FILE* o = fopen(argv[2], "wb");
    if (!o) { printf("cannot write %s\n", argv[2]); return 1; }
    fwrite(r.html.data(), 1, r.html.size(), o);
    fclose(o);
    printf("wrote %zu bytes, %zu images, %d anchors\n", r.html.size(), r.images.size(), (int)r.anchors.size());
    return 0;
}
