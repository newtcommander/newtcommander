# read_languages.ps1 - shipped-language registry reader for build_langs.cmd
#
# Parses translations\languages.cfg (UTF-8 INI), validates it against the
# directories under translations\, and emits one pipe-delimited record per
# language so a batch file can consume it with a simple for /f loop:
#
#     <folder>|<langid>|<origin>|<enabled>
#
# EVERY registered language is emitted, enabled or not (feature 039). The
# consumers need both sets: build_langs.ps1 builds the enabled ones, and
# lang_policy.ps1 needs to know what to delete from the build output. One
# reader serving both means one place to be wrong.
#
# This script is a PURE READER -- it never deletes or writes anything. Output
# reconciliation lives in lang_policy.ps1.
#
# Batch cannot read the registry directly: author and comment fields carry
# Cyrillic, CJK, and accented Latin text, which the console code page mangles.
# This script is the boundary -- it emits ASCII-safe fields only, and the rich
# metadata stays on the Python side (tools/translate/config.py).
#
# Exit code 0 = success; 1 = validation or I/O error ("ERROR: ..." lines).
# See specs/038-translations-build-integration/data-model.md

param(
    [Parameter(Mandatory = $true)][string]$Config,
    [Parameter(Mandatory = $true)][string]$TranslationsRoot
)

$ErrorActionPreference = 'Stop'
$errors = New-Object System.Collections.Generic.List[string]

# Every section name encountered, whether or not its record validated. V3 below
# uses this rather than the validated list: a section with a bad field is still
# a section, and reporting its directory as "not registered" on top of the real
# error sends the reader looking for a second, non-existent problem.
$seenFolders = New-Object System.Collections.Generic.List[string]

if (-not (Test-Path -LiteralPath $Config -PathType Leaf)) {
    Write-Output ("ERROR: languages.cfg not found at '{0}'. This file is the" -f $Config)
    Write-Output '       shipped-language registry: one [folder] section per language'
    Write-Output '       directory under translations\.'
    exit 1
}

# --- parse the INI ---------------------------------------------------------
$languages = New-Object System.Collections.Generic.List[object]
$current = $null
$fields = @{}
$required = @('langid', 'display_name', 'author', 'web', 'comment', 'helpdir', 'origin', 'enabled')

function Flush-Section {
    if ($null -eq $script:current) { return }
    $script:seenFolders.Add($script:current)
    $missing = @($script:required | Where-Object { -not $script:fields.ContainsKey($_) })
    if ($missing.Count -gt 0) {
        $script:errors.Add(("language [{0}] is missing: {1}" -f $script:current, ($missing -join ', ')))
    }
    else {
        $langid = 0
        if (-not [int]::TryParse($script:fields['langid'], [ref]$langid)) {
            $script:errors.Add(("language [{0}] has non-numeric langid '{1}'" -f $script:current, $script:fields['langid']))
        }
        elseif ($script:fields['origin'] -notin @('human', 'mixed', 'machine')) {
            $script:errors.Add(("language [{0}] has unknown origin '{1}' (expected human, mixed, or machine)" -f $script:current, $script:fields['origin']))
        }
        elseif ($script:fields['enabled'] -notin @('on', 'off')) {
            $script:errors.Add(("language [{0}] has enabled '{1}' (expected on or off)" -f $script:current, $script:fields['enabled']))
        }
        else {
            $script:languages.Add([pscustomobject]@{
                Folder  = $script:current
                LangId  = $langid
                Origin  = $script:fields['origin']
                Enabled = ($script:fields['enabled'] -eq 'on')
            })
        }
    }
    $script:current = $null
    $script:fields = @{}
}

$lineNo = 0
foreach ($raw in (Get-Content -LiteralPath $Config -Encoding UTF8)) {
    $lineNo++
    $line = ($raw -split '#', 2)[0].Trim()
    if ($line.Length -eq 0) { continue }

    if ($line.StartsWith('[') -and $line.EndsWith(']')) {
        Flush-Section
        $current = $line.Substring(1, $line.Length - 2).Trim()
        continue
    }
    if ($null -eq $current) {
        $errors.Add(("line {0}: value outside a [language] section" -f $lineNo))
        continue
    }
    $eq = $line.IndexOf('=')
    if ($eq -lt 0) {
        $errors.Add(("line {0}: expected 'key = value' -- got '{1}'" -f $lineNo, $raw.Trim()))
        continue
    }
    $fields[$line.Substring(0, $eq).Trim()] = $line.Substring($eq + 1).Trim()
}
Flush-Section

if ($languages.Count -eq 0 -and $errors.Count -eq 0) {
    $errors.Add('no languages defined')
}

# --- V1: LANGIDs must be unique (the chooser matches on them) --------------
$seen = @{}
foreach ($lang in $languages) {
    if ($seen.ContainsKey($lang.LangId)) {
        $errors.Add(("LANGID {0} used by both [{1}] and [{2}] -- must be unique" -f $lang.LangId, $seen[$lang.LangId], $lang.Folder))
    }
    else { $seen[$lang.LangId] = $lang.Folder }
}

# --- V2: registry and translations\ must agree -----------------------------
if (Test-Path -LiteralPath $TranslationsRoot -PathType Container) {
    $onDisk = @{}
    foreach ($d in (Get-ChildItem -LiteralPath $TranslationsRoot -Directory)) {
        $onDisk[$d.Name] = $true
    }
    foreach ($lang in $languages) {
        if (-not $onDisk.ContainsKey($lang.Folder)) {
            $errors.Add(("no translations\{0}\ directory -- create it or remove the [{0}] record" -f $lang.Folder))
        }
    }
    $registered = @{}
    foreach ($name in $seenFolders) { $registered[$name] = $true }
    foreach ($name in $onDisk.Keys) {
        if (-not $registered.ContainsKey($name)) {
            $errors.Add(("translations\{0}\ exists but is not registered in languages.cfg" -f $name))
        }
    }
}
else {
    $errors.Add(("translations root not found: {0}" -f $TranslationsRoot))
}

if ($errors.Count -gt 0) {
    foreach ($e in $errors) { Write-Output ("ERROR: {0}" -f $e) }
    exit 1
}

foreach ($lang in $languages) {
    $enabled = if ($lang.Enabled) { 1 } else { 0 }
    Write-Output ("{0}|{1}|{2}|{3}" -f $lang.Folder, $lang.LangId, $lang.Origin, $enabled)
}
exit 0
