# verify_slg.ps1 - post-build language-module version check (spec FR-026)
#
# The engine refuses any .slg whose VS_FIXEDFILEINFO file version differs from
# the module that owns it -- IsSLGFileValid (src/salamdr2.cpp:3005) compares
# dwFileVersionMS/LS and returns FALSE on any mismatch. A mismatched module is
# silently dropped at runtime and the user gets a language-selection prompt or a
# missing language, so this must fail the build instead.
#
# In the normal pipeline a mismatch is impossible by construction: each
# <language>.slg starts as a byte copy of english.slg from the same build, and
# translator.exe rewrites only VERSIONINFO *strings* plus
# VarFileInfo\Translation -- never FILEVERSION. This check exists to catch the
# abnormal cases: a stale .slg left in the output tree from an earlier build, a
# partially-written file, or a copy taken from the wrong configuration.
#
# Exit code 0 = all modules match; 1 = at least one mismatch or missing owner.
# See specs/038-translations-build-integration/data-model.md

param(
    [Parameter(Mandatory = $true)][string]$OutDir,
    # Restrict the check to one module (used when iterating a single pair).
    [string]$Module,
    [switch]$Quiet
)

$ErrorActionPreference = 'Stop'

function Get-FileVersionPair {
    param([string]$Path)
    $info = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($Path)
    # Compose the same two DWORDs IsSLGFileValid compares.
    $ms = ([uint32]$info.FileMajorPart -shl 16) -bor ([uint32]$info.FileMinorPart -band 0xFFFF)
    $ls = ([uint32]$info.FileBuildPart -shl 16) -bor ([uint32]$info.FilePrivatePart -band 0xFFFF)
    return [pscustomobject]@{
        MS   = $ms
        LS   = $ls
        Text = ("{0}.{1}.{2}.{3}" -f $info.FileMajorPart, $info.FileMinorPart, $info.FileBuildPart, $info.FilePrivatePart)
    }
}

$checked = 0
$failures = New-Object System.Collections.Generic.List[string]

# --- the application module ------------------------------------------------
$targets = New-Object System.Collections.Generic.List[object]

if (-not $Module -or $Module -eq 'salamand') {
    $appLangDir = Join-Path $OutDir 'lang'
    if (Test-Path -LiteralPath $appLangDir -PathType Container) {
        $targets.Add([pscustomobject]@{
            Name  = 'salamand'
            Owner = Join-Path $OutDir 'tandemcommander.exe'
            Dir   = $appLangDir
        })
    }
}

# --- plugin modules --------------------------------------------------------
$pluginsRoot = Join-Path $OutDir 'plugins'
if (Test-Path -LiteralPath $pluginsRoot -PathType Container) {
    foreach ($d in (Get-ChildItem -LiteralPath $pluginsRoot -Directory)) {
        if ($Module -and $Module -ne $d.Name) { continue }
        $langDir = Join-Path $d.FullName 'lang'
        if (-not (Test-Path -LiteralPath $langDir -PathType Container)) { continue }
        # The plugin binary sits inside its own directory:
        # plugins\<name>\<name>.spl (plugin_base.props OutDir), not plugins\<name>.spl.
        $targets.Add([pscustomobject]@{
            Name  = $d.Name
            Owner = Join-Path $d.FullName ("{0}.spl" -f $d.Name)
            Dir   = $langDir
        })
    }
}

foreach ($t in $targets) {
    if (-not (Test-Path -LiteralPath $t.Owner -PathType Leaf)) {
        $failures.Add(("{0}: owning binary not found at '{1}'" -f $t.Name, $t.Owner))
        continue
    }
    $owner = Get-FileVersionPair -Path $t.Owner

    foreach ($slg in (Get-ChildItem -LiteralPath $t.Dir -Filter '*.slg' -File)) {
        $checked++
        $v = Get-FileVersionPair -Path $slg.FullName
        if ($v.MS -ne $owner.MS -or $v.LS -ne $owner.LS) {
            $failures.Add(("{0}\{1}: version {2} does not match {3} ({4}) -- the engine will reject this module" -f `
                        $t.Name, $slg.Name, $v.Text, (Split-Path -Leaf $t.Owner), $owner.Text))
        }
    }
}

if ($failures.Count -gt 0) {
    Write-Output ("ERROR: {0} language module(s) failed the version check:" -f $failures.Count)
    foreach ($f in $failures) { Write-Output ("       {0}" -f $f) }
    Write-Output '       A .slg must carry the exact FILEVERSION of the binary it belongs to.'
    Write-Output '       Delete the stale output and rebuild; do not ship these.'
    exit 1
}

if (-not $Quiet) {
    Write-Output ("  version check: {0} language module(s) OK across {1} module(s)" -f $checked, $targets.Count)
}
exit 0
