// SPDX-FileCopyrightText: 2026 Pavel Stupka
// SPDX-License-Identifier: GPL-2.0-or-later
//
// highlight.cpp - best-effort lexical syntax highlighting (FR-013).
// Not a real parser; a pragmatic tokenizer good enough for README code blocks.

#include "precomp.h"
#include "render.h"
#include <cwctype>
#include <string>

namespace
{
enum Group { G_PLAIN, G_CLIKE, G_PY, G_SQL, G_SHELL, G_JSON, G_XML, G_DIFF };

std::string Normalize(const std::string& lang)
{
    std::string l;
    for (char ch : lang) l += (char)tolower((unsigned char)ch);
    if (l == "c++" || l == "cc" || l == "hpp" || l == "cxx") return "cpp";
    if (l == "cs") return "csharp";
    if (l == "javascript" || l == "jsx" || l == "mjs") return "js";
    if (l == "typescript" || l == "tsx") return "ts";
    if (l == "py" || l == "python3") return "python";
    if (l == "yml") return "yaml";
    if (l == "sh" || l == "bash" || l == "zsh" || l == "console" || l == "shell-session") return "shell";
    if (l == "bat" || l == "cmd") return "batch";
    if (l == "ps1" || l == "ps") return "powershell";
    if (l == "patch") return "diff";
    if (l == "text" || l == "plaintext" || l == "txt") return "plain";
    if (l == "htm") return "html";
    return l;
}

Group GroupOf(const std::string& l)
{
    if (l == "c" || l == "cpp" || l == "csharp" || l == "js" || l == "ts" || l == "java" ||
        l == "go" || l == "rust" || l == "css")
        return G_CLIKE;
    if (l == "python" || l == "cmake" || l == "yaml" || l == "toml" || l == "ini" || l == "powershell")
        return G_PY; // '#' line comment family
    if (l == "sql") return G_SQL;
    if (l == "shell" || l == "batch") return G_SHELL;
    if (l == "json") return G_JSON;
    if (l == "xml" || l == "html") return G_XML;
    if (l == "diff") return G_DIFF;
    return G_PLAIN;
}

bool IsKeyword(const std::wstring& w, Group g)
{
    static const wchar_t* clike[] = {L"if", L"else", L"for", L"while", L"do", L"switch", L"case", L"default", L"break", L"continue", L"return", L"class", L"struct", L"enum", L"union", L"public", L"private", L"protected", L"virtual", L"static", L"const", L"void", L"int", L"char", L"bool", L"float", L"double", L"long", L"short", L"unsigned", L"signed", L"new", L"delete", L"this", L"nullptr", L"true", L"false", L"namespace", L"using", L"template", L"typename", L"auto", L"try", L"catch", L"throw", L"function", L"var", L"let", L"const", L"import", L"export", L"from", L"async", L"await", L"typeof", L"interface", L"type", L"func", L"package", L"fn", L"let", L"mut", L"impl", L"pub", L"match", L"in", L"of", L"extends", L"implements", L"final", L"override", L"and", L"or", L"not", NULL};
    static const wchar_t* py[] = {L"def", L"class", L"if", L"elif", L"else", L"for", L"while", L"return", L"import", L"from", L"as", L"try", L"except", L"finally", L"with", L"lambda", L"pass", L"break", L"continue", L"and", L"or", L"not", L"in", L"is", L"None", L"True", L"False", L"self", L"yield", L"global", L"nonlocal", L"raise", L"assert", L"del", L"function", L"end", L"then", L"do", NULL};
    static const wchar_t* sql[] = {L"select", L"from", L"where", L"insert", L"into", L"values", L"update", L"set", L"delete", L"create", L"table", L"drop", L"alter", L"join", L"inner", L"left", L"right", L"outer", L"on", L"group", L"by", L"order", L"having", L"and", L"or", L"not", L"null", L"as", L"distinct", L"count", L"index", L"primary", L"key", L"foreign", L"references", NULL};
    static const wchar_t* shell[] = {L"if", L"then", L"else", L"elif", L"fi", L"for", L"while", L"do", L"done", L"case", L"esac", L"function", L"return", L"echo", L"export", L"local", L"set", L"cd", L"rem", L"call", L"goto", L"exit", NULL};
    static const wchar_t* json[] = {L"true", L"false", L"null", NULL};
    const wchar_t** t = NULL;
    switch (g)
    {
    case G_CLIKE: t = clike; break;
    case G_PY: t = py; break;
    case G_SQL: t = sql; break;
    case G_SHELL: t = shell; break;
    case G_JSON: t = json; break;
    default: return false;
    }
    for (int i = 0; t[i]; i++)
    {
        // SQL/shell keywords are case-insensitive
        if (g == G_SQL || g == G_SHELL)
        {
            if (lstrcmpiW(w.c_str(), t[i]) == 0) return true;
        }
        else if (w == t[i]) return true;
    }
    return false;
}

void add(std::vector<HlRun>& runs, size_t s, size_t e, int cf)
{
    if (e > s) runs.push_back({s, e - s, cf});
}
} // namespace

void HighlightCode(const std::wstring& code, const std::string& langRaw, std::vector<HlRun>& runs)
{
    std::string lang = Normalize(langRaw);
    Group g = GroupOf(lang);
    if (g == G_PLAIN) return;

    // diff: color whole lines by first char
    if (g == G_DIFF)
    {
        size_t i = 0, n = code.size();
        while (i < n)
        {
            size_t j = i; while (j < n && code[j] != L'\n') j++;
            wchar_t first = (i < n) ? code[i] : 0;
            if (first == L'+') add(runs, i, j, MDCF_KEYWORD); // reuse add color; render maps colorCf directly
            else if (first == L'-') add(runs, i, j, MDCF_STRING);
            else if (first == L'@') add(runs, i, j, MDCF_COMMENT);
            i = (j < n) ? j + 1 : j;
        }
        // remap: use dedicated diff colors (22/23) via direct override
        for (HlRun& r : runs)
        {
            if (r.colorCf == MDCF_KEYWORD) r.colorCf = 22; // add
            else if (r.colorCf == MDCF_STRING) r.colorCf = 23; // del
        }
        return;
    }

    bool hashComment = (g == G_PY || g == G_SHELL);
    bool dashComment = (g == G_SQL);
    bool slashComment = (g == G_CLIKE);
    bool blockComment = (g == G_CLIKE || g == G_SQL);
    bool xmlComment = (g == G_XML);

    size_t i = 0, n = code.size();
    while (i < n)
    {
        wchar_t ch = code[i];

        // XML comment / tag coloring
        if (xmlComment && ch == L'<')
        {
            if (i + 3 < n && code.compare(i, 4, L"<!--") == 0)
            {
                size_t e = code.find(L"-->", i + 4);
                e = (e == std::wstring::npos) ? n : e + 3;
                add(runs, i, e, MDCF_COMMENT); i = e; continue;
            }
            size_t e = code.find(L'>', i);
            e = (e == std::wstring::npos) ? n : e + 1;
            add(runs, i, e, MDCF_KEYWORD); i = e; continue;
        }

        // line comments
        if (hashComment && ch == L'#')
        { size_t e = i; while (e < n && code[e] != L'\n') e++; add(runs, i, e, MDCF_COMMENT); i = e; continue; }
        if (g == G_SHELL && ch == L':' && i + 1 < n && code[i + 1] == L':')
        { size_t e = i; while (e < n && code[e] != L'\n') e++; add(runs, i, e, MDCF_COMMENT); i = e; continue; }
        if (dashComment && ch == L'-' && i + 1 < n && code[i + 1] == L'-')
        { size_t e = i; while (e < n && code[e] != L'\n') e++; add(runs, i, e, MDCF_COMMENT); i = e; continue; }
        if (slashComment && ch == L'/' && i + 1 < n && code[i + 1] == L'/')
        { size_t e = i; while (e < n && code[e] != L'\n') e++; add(runs, i, e, MDCF_COMMENT); i = e; continue; }
        if (blockComment && ch == L'/' && i + 1 < n && code[i + 1] == L'*')
        { size_t e = code.find(L"*/", i + 2); e = (e == std::wstring::npos) ? n : e + 2; add(runs, i, e, MDCF_COMMENT); i = e; continue; }

        // strings
        if (ch == L'"' || ch == L'\'' || (ch == L'`' && g == G_CLIKE))
        {
            wchar_t q = ch; size_t e = i + 1;
            while (e < n && code[e] != q) { if (code[e] == L'\\' && e + 1 < n) e++; if (code[e] == L'\n') break; e++; }
            if (e < n && code[e] == q) e++;
            add(runs, i, e, MDCF_STRING); i = e; continue;
        }

        // numbers
        if (iswdigit(ch))
        {
            size_t e = i; while (e < n && (iswalnum(code[e]) || code[e] == L'.' || code[e] == L'x' || code[e] == L'X')) e++;
            add(runs, i, e, MDCF_NUMBER); i = e; continue;
        }

        // identifiers / keywords
        if (iswalpha(ch) || ch == L'_')
        {
            size_t e = i; while (e < n && (iswalnum(code[e]) || code[e] == L'_')) e++;
            std::wstring w = code.substr(i, e - i);
            if (IsKeyword(w, g)) add(runs, i, e, MDCF_KEYWORD);
            else if (e < n && code[e] == L'(') add(runs, i, e, MDCF_FUNC);
            i = e; continue;
        }

        i++;
    }
}
