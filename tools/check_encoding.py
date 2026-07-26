#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Newt Commander Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Guard against the file-name display-encoding defect class (feature 042).

A file name is stored as UTF-8 everywhere in this application.  It is destroyed
only at the last step before it is drawn, and it goes wrong in exactly two ways:

  * LOSSY        - the name is converted down to the machine's legacy codepage,
                   so every character that codepage cannot hold becomes '?'.
                   One '?' per UTF-16 code unit, so one emoji costs two.
  * UNINTERPRETED- the name's UTF-8 bytes are drawn as if they were legacy text,
                   so 'c' with caron becomes two mojibake characters.

Both were reported by users, twice, on different surfaces.  This script exists so
the third occurrence fails the build instead of reaching someone's screen.

Rules
-----
cp-acp-display   A name is converted with WideCharToMultiByte(CP_ACP, ...) on a
                 path that ends in a display.  Lossy by construction: forbidden.

mixed-composition
                 A printf-family call whose FORMAT comes from LoadStr() (legacy
                 codepage) and whose arguments include a file name (UTF-8), with
                 the result handed to a message box.  The composed string is then
                 not valid UTF-8, so CMessageBox refuses its own wide drawing
                 path and falls back to the legacy one - which renders the
                 template correctly and the name as mojibake.
                 NOTE: an English build cannot reproduce this.  English resources
                 are pure ASCII, ASCII is valid UTF-8, so the composed string
                 converts cleanly and looks correct.  Only localized builds show
                 it.  That is why this rule is static rather than a runtime test.

dead-dispinfow   A dialog handles LVN_GETDISPINFOW but never sends NF_REQUERY.
                 Such a handler can never run: a list view asks its parent for a
                 notification format during its own creation, which is before
                 WM_INITDIALOG, and CDialog::CDialogProc only attaches the dialog
                 object at WM_INITDIALOG.  The query therefore goes unanswered,
                 DefDlgProc replies from IsWindowUnicode(parent) - FALSE for every
                 dialog here - and the control settles on ANSI permanently.
                 This rule would have caught the reported Find defect on the day
                 the handler was written.

Suppressing a finding
---------------------
Put a marker on the offending line or the line above it:

    // encoding-check: allow <rule-id> - <reason>

The reason is mandatory and is what a future reader will be judged on.  Blanket
suppression of a whole file is deliberately not supported.

Usage
-----
    python tools/check_encoding.py              # report, always exit 0
    python tools/check_encoding.py --strict     # exit 1 if anything is found
    python tools/check_encoding.py --rule mixed-composition
    python tools/check_encoding.py --format list   # machine-readable
"""

import argparse
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
SRC = REPO / "src"

# Subtrees that are not the main application binary.  Plugins are excluded by
# policy (feature 042 must not change plugin behaviour); the rest are separate
# executables with their own text handling.
EXCLUDED = ("plugins/", "saltests/", "tserver/", "shellext/", "setup/",
            "salmon/", "salopen/", "translator/", "reglib/", "common/dep/")

SUPPRESS = re.compile(r'//\s*encoding-check:\s*allow\s+([a-z-]+)\s*-\s*(\S.*)')

PRINTF = re.compile(r'\b(?:sprintf|sprintf_s|_snprintf|_snprintf_s|wsprintf)\s*\(')
FMT_IS_LOADSTR = re.compile(r'\b(?:sprintf|sprintf_s|_snprintf|_snprintf_s|wsprintf)\s*\('
                            r'[^;]*?\bLoadStr\s*\(')
# An argument that carries a file or directory name.  Deliberately broad: the
# strict version of this pattern missed fileswnb.cpp:815 - the very defect that
# prompted this feature - because the name arrived from an accessor called
# GetEquivalentPairNoticeName().  Recorded in research.md R5.
NAME_ARG = re.compile(r'[A-Za-z_]*(?:[Nn]ame|[Pp]ath|[Ff]ile|[Dd]ir|[Mm]ask|[Aa]rchive)[A-Za-z_]*')
MSGBOX = re.compile(r'\b(?:SalMessageBox|SalMessageBoxEx|ShowMessageBox)\b')

CP_ACP_W2A = re.compile(r'\bWideCharToMultiByte\s*\(\s*CP_ACP\b')
# A CP_ACP conversion counts as a display path only when its result is handed
# straight to something that draws.  This keeps the many legitimate OLE, shell
# and clipboard boundary conversions out of the rule.
DISPLAY_SINK = re.compile(r'\bpszText\b|\bDrawText\s*\(|\bTextOut\s*\(|\bExtTextOut\s*\(|'
                          r'\bSetWindowText\s*\(|\bSetDlgItemText\s*\(|SB_SETTEXT')

DISPINFOW = re.compile(r'\bLVN_GETDISPINFOW\b')
REQUERY = re.compile(r'\bNF_REQUERY\b')

RULES = ("cp-acp-display", "mixed-composition", "dead-dispinfow")


class Finding:
    def __init__(self, rule, path, line, text):
        self.rule, self.path, self.line, self.text = rule, path, line, text

    def __str__(self):
        return f"{self.path}:{self.line}: [{self.rule}] {self.text.strip()[:110]}"


def sources():
    for p in sorted(SRC.rglob("*.cpp")):
        rel = p.relative_to(SRC).as_posix()
        if rel.startswith(EXCLUDED):
            continue
        yield p, rel


def call_text(lines, i):
    """Return exactly the printf-family call starting on line i.

    A fixed line window is not good enough: it swallows the next statement and
    reports calls whose format is a plain literal as if it came from LoadStr().
    Match parentheses instead, so the text is the call and nothing else.
    """
    m = PRINTF.search(lines[i])
    if not m:
        return None
    # text from the call NAME onwards (the name is needed for FMT_IS_LOADSTR to
    # match), across at most 12 lines, cut at the call's closing paren
    blob = " ".join([lines[i][m.start():]] + lines[i + 1:i + 12])
    open_at = blob.index("(")
    depth = 0
    for pos in range(open_at, len(blob)):
        if blob[pos] == '(':
            depth += 1
        elif blob[pos] == ')':
            depth -= 1
            if depth == 0:
                return blob[:pos + 1]
    return blob


def suppressed(lines, i, rule):
    """A marker on the offending line, or anywhere in the comment block above it.

    The block is walked rather than just the previous line so a reason long
    enough to be worth reading can wrap onto continuation lines without the
    marker silently ceasing to apply.
    """
    if 0 <= i < len(lines):
        m = SUPPRESS.search(lines[i])
        if m and m.group(1) == rule:
            return True
    j = i - 1
    while j >= 0 and lines[j].lstrip().startswith("//"):
        m = SUPPRESS.search(lines[j])
        if m and m.group(1) == rule:
            return True
        j -= 1
    return False


def scan(only=None):
    findings = []
    for path, rel in sources():
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
        joined = "\n".join(lines)

        # --- dead-dispinfow: a whole-file property -------------------------
        if only in (None, "dead-dispinfow"):
            if DISPINFOW.search(joined) and not REQUERY.search(joined):
                i = next(i for i, l in enumerate(lines) if DISPINFOW.search(l))
                if not suppressed(lines, i, "dead-dispinfow"):
                    findings.append(Finding(
                        "dead-dispinfow", rel, i + 1,
                        "LVN_GETDISPINFOW handler in a file that never sends NF_REQUERY "
                        "- the handler cannot run"))

        for i, ln in enumerate(lines):
            # --- mixed-composition ----------------------------------------
            if only in (None, "mixed-composition") and PRINTF.search(ln):
                stmt = call_text(lines, i) or ""
                # the FORMAT argument must be the LoadStr() one: it is the first
                # argument after the destination buffer
                if FMT_IS_LOADSTR.search(stmt):
                    after = stmt[stmt.find("LoadStr("):]
                    if NAME_ARG.search(after) and MSGBOX.search(" ".join(lines[i:i + 14])):
                        if not suppressed(lines, i, "mixed-composition"):
                            findings.append(Finding("mixed-composition", rel, i + 1, ln))

            # --- cp-acp-display -------------------------------------------
            if only in (None, "cp-acp-display") and CP_ACP_W2A.search(ln):
                window = " ".join(lines[max(0, i - 6):i + 8])
                if DISPLAY_SINK.search(window):
                    if not suppressed(lines, i, "cp-acp-display"):
                        findings.append(Finding("cp-acp-display", rel, i + 1, ln))

    return findings


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--strict", action="store_true",
                    help="exit 1 when anything is found (used by build.cmd)")
    ap.add_argument("--rule", choices=RULES, help="scan a single rule")
    ap.add_argument("--format", choices=("report", "list"), default="report")
    args = ap.parse_args()

    findings = scan(args.rule)

    if args.format == "list":
        for f in findings:
            print(f)
    else:
        by_rule = {}
        for f in findings:
            by_rule.setdefault(f.rule, []).append(f)
        print("=" * 72)
        print(" check_encoding.py - file-name display-encoding guard (feature 042)")
        print("=" * 72)
        for rule in RULES:
            if args.rule and rule != args.rule:
                continue
            hits = by_rule.get(rule, [])
            print(f"\n[{rule}] {len(hits)} finding(s)")
            for f in hits:
                print(f"  {f}")
        print(f"\nTOTAL: {len(findings)} finding(s)")
        if findings and args.strict:
            print("\nA file name would be destroyed on its way to the screen.")
            print("Fix the site, or suppress it with a reason:")
            print("    // encoding-check: allow <rule-id> - <reason>")

    return 1 if (findings and args.strict) else 0


if __name__ == "__main__":
    sys.exit(main())
