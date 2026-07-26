# lang_policy.ps1 - language build policy stage (feature 039)
#
# The language counterpart of gen_plugins_filter.ps1. Called from build.cmd on
# EVERY build, before MSBuild, it does three things:
#
#   1. validates translations\languages.cfg (delegating the parse to
#      read_languages.ps1) and stops the build on any error;
#   2. reconciles the build output with the policy -- every .slg that does not
#      belong to an enabled language is deleted;
#   3. reports the counts for the build banner.
#
# WHY IT RUNS ON EVERY BUILD, NOT JUST FULL BUILDS
#
# Language modules are PRODUCED by build_langs.cmd, which build.cmd calls only
# from :populate_runtime, i.e. only on "build.cmd full". If removal lived there
# too, a plain build.cmd after switching a language off would leave its modules
# in the output tree; the product enumerates lang\*.slg from disk
# (src\dialogs2.cpp:928), would still find them, and switching the language off
# would appear to do nothing. So removal happens here and production stays
# there. Removal has no dependency on build output existing; production has
# every dependency on it.
#
# THE RECONCILIATION RULE IS POSITIVE
#
#   keep english.slg and one .slg per ENABLED language, delete every other .slg
#
# stated that way rather than "delete the disabled ones" so a language renamed
# or dropped from the registry entirely is cleaned up too -- the same thing the
# plugin stage does for unknown output directories. english.slg is never
# produced by build_langs (MSBuild compiles it from the .rc sources) and is
# always kept.
#
# Exit code 0 = success; 1 = validation error (nothing is deleted in that case).
# See specs/039-language-build-policy/contracts/build-scripts.md

param(
    [Parameter(Mandatory = $true)][string]$Config,
    [Parameter(Mandatory = $true)][string]$TranslationsRoot,
    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# ---------------------------------------------------------------------------
# 1. Validate -- read_languages.ps1 owns the parse and every error message
# ---------------------------------------------------------------------------

$records = & (Join-Path $scriptDir 'read_languages.ps1') -Config $Config -TranslationsRoot $TranslationsRoot
if ($LASTEXITCODE -ne 0) {
    $records | ForEach-Object { Write-Output $_ }
    Write-Output ''
    Write-Output 'ERROR: the language registry is the build policy for languages --'
    Write-Output '       every section needs "enabled = on" or "enabled = off".'
    exit 1
}

$enabled = New-Object System.Collections.Generic.List[string]
$disabled = New-Object System.Collections.Generic.List[string]
foreach ($line in $records) {
    if ($line -notmatch '\|') { continue }
    $parts = $line.Split('|')
    if ($parts[3] -eq '1') { $enabled.Add($parts[0]) } else { $disabled.Add($parts[0]) }
}

# ---------------------------------------------------------------------------
# 2. Reconcile the build output
# ---------------------------------------------------------------------------
#
# A tree that does not exist yet (first build) has nothing stale in it, so a
# missing -OutputRoot is not an error.

$removed = 0
if ($OutputRoot -and (Test-Path -LiteralPath $OutputRoot -PathType Container)) {

    # keep-set: english.slg plus one .slg per enabled language
    $keep = @{ 'english.slg' = $true }
    foreach ($folder in $enabled) { $keep[("{0}.slg" -f $folder)] = $true }

    $langDirs = New-Object System.Collections.Generic.List[string]
    $appLang = Join-Path $OutputRoot 'lang'
    if (Test-Path -LiteralPath $appLang -PathType Container) { $langDirs.Add($appLang) }
    $pluginsDir = Join-Path $OutputRoot 'plugins'
    if (Test-Path -LiteralPath $pluginsDir -PathType Container) {
        foreach ($d in (Get-ChildItem -LiteralPath $pluginsDir -Directory)) {
            $pluginLang = Join-Path $d.FullName 'lang'
            if (Test-Path -LiteralPath $pluginLang -PathType Container) { $langDirs.Add($pluginLang) }
        }
    }

    foreach ($dir in $langDirs) {
        foreach ($f in (Get-ChildItem -LiteralPath $dir -Filter '*.slg' -File)) {
            if (-not $keep.ContainsKey($f.Name.ToLowerInvariant())) {
                Remove-Item -LiteralPath $f.FullName -Force
                Write-Output ("Reconcile: removed stale language module {0}" -f $f.FullName)
                $removed++
            }
        }
    }
}

# ---------------------------------------------------------------------------
# 3. Report -- build.cmd parses this line for the configuration banner
# ---------------------------------------------------------------------------

if ($removed -gt 0) {
    Write-Output ("Reconcile: {0} stale language module(s) removed" -f $removed)
}
Write-Output ("Languages: {0} enabled, {1} disabled" -f $enabled.Count, $disabled.Count)
if ($disabled.Count -gt 0) {
    Write-Output ("           off: {0}" -f (($disabled | Sort-Object) -join ', '))
}
exit 0
