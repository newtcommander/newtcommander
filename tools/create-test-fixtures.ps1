# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Creates the test fixtures for feature 004-long-paths-unicode
# (specs/004-long-paths-unicode/quickstart.md). Uses \\?\ paths so the
# fixtures can be created regardless of application/registry state.
#
# Usage: powershell -ExecutionPolicy Bypass -File tools\create-test-fixtures.ps1 [-Base <dir>] [-Perf]

param(
    [string]$Base = "$env:TEMP\salamander-test",
    [switch]$Perf # also create the 100k-file performance directory
)

$ErrorActionPreference = 'Stop'

# --- 1. Deep path fixture (total path > 300 chars) ---
$seg = "a" * 60
$deep = "\\?\$Base\$seg\$seg\$seg\$seg\$seg"
New-Item -ItemType Directory -Force $deep | Out-Null
Set-Content -LiteralPath "$deep\deep-file.txt" -Value "content at long path" -Encoding utf8
Write-Host "deep fixture: $($deep.Length - 4) chars total path"

# --- 2. Unicode name fixtures (NFC vs NFD, non-ACP, non-BMP) ---
$u = "\\?\$Base\unicode"
New-Item -ItemType Directory -Force $u | Out-Null
# NFC: precomposed c-caron (U+010D)
Set-Content -LiteralPath "$u\$([char]0x010D).txt" -Value "NFC" -Encoding utf8
# NFD: c + combining caron (U+030C) - visually identical to the NFC file
Set-Content -LiteralPath "$u\c$([char]0x030C).txt" -Value "NFD" -Encoding utf8
# Outside a Czech/Western ACP: Greek, Japanese
Set-Content -LiteralPath "$u\$([char]0x03B4)$([char]0x03BF)$([char]0x03BA)$([char]0x03B9)$([char]0x03BC)$([char]0x03AE).txt" -Value "greek" -Encoding utf8
Set-Content -LiteralPath "$u\$([char]0x30C6)$([char]0x30B9)$([char]0x30C8).txt" -Value "japanese" -Encoding utf8
# Non-BMP (surrogate pair): folder emoji U+1F4C1
Set-Content -LiteralPath "$u\$([char]::ConvertFromUtf32(0x1F4C1))report.txt" -Value "emoji" -Encoding utf8
Write-Host "unicode fixtures: $((Get-ChildItem -LiteralPath $u).Count) files in $Base\unicode"

# --- 3. Combined stress: NFD name at a deep path ---
Set-Content -LiteralPath "$deep\c$([char]0x030C)-deep.txt" -Value "NFD deep" -Encoding utf8

# --- 4. Optional: 100k files for the SC-009 performance check ---
if ($Perf) {
    $perfDir = "\\?\$Base\perf"
    New-Item -ItemType Directory -Force $perfDir | Out-Null
    Write-Host "creating 100000 files in $Base\perf (takes a while)..."
    1..100000 | ForEach-Object { New-Item -ItemType File -Force "$perfDir\f$_.txt" } | Out-Null
    Write-Host "perf fixture done"
}

Write-Host "fixtures ready under $Base"
