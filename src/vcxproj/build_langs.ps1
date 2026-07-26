# build_langs.ps1 - produces the shipped .slg language modules (feature 038)
#
# Driven by build_langs.cmd. For every (module x language) pair it:
#
#   1. copies english.slg to <language>.slg          (seed)
#   2. generates <module>.atp                        (gen_atp.ps1)
#   3. runs translator.exe -quiet-import-slt         (guarded)
#   4. verifies FILEVERSION against the owning binary (verify_slg.ps1)
#
# Three properties of translator.exe drive the design here:
#
#   * SUCCESS IS EXIT CODE 1, not 0. wndframe.cpp:365/461/532 all end with
#     ExitProcess(1) on the success path ("exit code 1 means validation
#     succeeded"). A conventional zero-is-success check rejects every good run.
#
#   * IT IS A GUI APPLICATION AND REPORTS ERRORS VIA MessageBox. An unguarded
#     failure blocks a build forever instead of failing it, so every invocation
#     runs under a timeout and is killed if it overruns.
#
#   * Save() PATCHES A COPY. CData::Save (trldata.cpp:328) begins with
#     CopyFile(FullTargetFile, ...), so the target must already exist. Seeding
#     from english.slg satisfies that AND gives the target
#     SLGCRCofImpSLT="none" (versinfo.rc2:63), which makes
#     IsSLTDataChanged() false and suppresses the one prompt on the success
#     path (trldata.cpp:2360).
#
# translator.exe is NOT built by a normal build: in salamand.sln its project has
# an ActiveCfg for Debug|x64 and Release|x64 but no .Build.0 entry -- it only
# builds under the "Utils (Release)" solution configuration. This script
# therefore builds it on demand from the .vcxproj (Release|Win32) the first time
# it is needed, rather than requiring a solution change.
#
# Exit code 0 = success; 1 = any failure. See
# specs/038-translations-build-integration/contracts/translator-cli.md

param(
    [string]$Module,
    [string]$Language,
    [switch]$ExportTemplates,
    [switch]$Force,
    [switch]$CheckLayout,
    [int]$TimeoutSeconds = 30
)

$ErrorActionPreference = 'Stop'
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# Paths arrive through the environment, not as parameters. "powershell -File"
# strips quoting from arguments containing spaces, which silently shifts every
# later positional argument -- and the default VS install path
# ("C:\Program Files\Microsoft Visual Studio\...") always contains spaces.
$RepoRoot = $env:NEWTC_REPO_ROOT
$OutDir = $env:NEWTC_OUT_DIR
$BuildDir = $env:NEWTC_BUILD_DIR
$MSBuild = $env:NEWTC_MSBUILD

foreach ($pair in @(@('NEWTC_REPO_ROOT', $RepoRoot), @('NEWTC_OUT_DIR', $OutDir), @('NEWTC_BUILD_DIR', $BuildDir))) {
    if (-not $pair[1]) {
        Write-Output ("ERROR: {0} is not set -- run this through build_langs.cmd." -f $pair[0])
        exit 1
    }
}

$languagesCfg = Join-Path $RepoRoot 'translations\languages.cfg'
$pluginsCfg = Join-Path $RepoRoot 'plugins.cfg'
$translationsRoot = Join-Path $RepoRoot 'translations'
$templatesDir = Join-Path $BuildDir 'salamander\translator\templates'
$atpRoot = Join-Path $BuildDir 'salamander\translator\projects'

# ---------------------------------------------------------------------------
# translator.exe
# ---------------------------------------------------------------------------

# NOTE on Write-Host below: every value in a PowerShell function's output stream
# becomes part of its return value, so a Write-Output progress line here would
# be returned alongside the path and the caller would receive an array. These
# functions return values, so all their messages go to the host instead.

function Resolve-Translator {
    $exe = Join-Path $BuildDir 'translator\Release\translator.exe'
    if (Test-Path -LiteralPath $exe -PathType Leaf) { return $exe }

    Write-Host '  translator.exe not found -- building it (Release|Win32, once)'
    $proj = Join-Path $scriptDir 'translator\translator.vcxproj'
    if (-not (Test-Path -LiteralPath $proj -PathType Leaf)) {
        Write-Host ("ERROR: translator project not found at '{0}'" -f $proj)
        return $null
    }
    if (-not $MSBuild -or -not (Test-Path -LiteralPath $MSBuild -PathType Leaf)) {
        Write-Host 'ERROR: translator.exe is missing and MSBuild was not located, so it'
        Write-Host '       cannot be built. Build src\vcxproj\translator\translator.vcxproj'
        Write-Host '       once with Configuration=Release Platform=Win32.'
        return $null
    }
    & $MSBuild $proj /t:Build /p:Configuration=Release /p:Platform=Win32 /v:minimal /nologo | Out-Null
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $exe -PathType Leaf)) {
        Write-Host 'ERROR: failed to build translator.exe.'
        return $null
    }
    return $exe
}

# ---------------------------------------------------------------------------
# Guarded invocation -- exit code 1 means success
# ---------------------------------------------------------------------------

function Invoke-Translator {
    param(
        [string]$Exe,
        [string[]]$Arguments,
        [string]$What
    )
    $p = Start-Process -FilePath $Exe -ArgumentList $Arguments -PassThru -WindowStyle Hidden
    if (-not $p.WaitForExit($TimeoutSeconds * 1000)) {
        try { $p.Kill() } catch { }
        Write-Host ("ERROR: {0}: translator.exe did not exit within {1}s and was killed." -f $What, $TimeoutSeconds)
        Write-Host '       It almost certainly opened an error message box. Do not run this'
        Write-Host '       step interactively; check the .slt matches the module structure.'
        return $false
    }
    if ($p.ExitCode -ne 1) {
        Write-Host ("ERROR: {0}: translator.exe exited with {1} (success is 1)." -f $What, $p.ExitCode)
        return $false
    }
    return $true
}

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

$langRecords = & (Join-Path $scriptDir 'read_languages.ps1') -Config $languagesCfg -TranslationsRoot $translationsRoot
if ($LASTEXITCODE -ne 0) {
    $langRecords | ForEach-Object { Write-Output $_ }
    exit 1
}

$languages = New-Object System.Collections.Generic.List[object]
foreach ($line in $langRecords) {
    if ($line -notmatch '\|') { continue }
    $parts = $line.Split('|')
    if ($Language -and $parts[0] -ne $Language) { continue }
    $languages.Add([pscustomobject]@{ Folder = $parts[0]; LangId = [int]$parts[1]; Origin = $parts[2] })
}
if ($Language -and $languages.Count -eq 0) {
    Write-Output ("ERROR: language '{0}' is not registered in languages.cfg" -f $Language)
    exit 1
}

# modules = the app plus every plugin set to "on"
$modules = New-Object System.Collections.Generic.List[string]
$modules.Add('salamand')
foreach ($raw in (Get-Content -LiteralPath $pluginsCfg -Encoding UTF8)) {
    $line = ($raw -split '#', 2)[0].Trim()
    if ($line.Length -eq 0 -or $line -notmatch '=') { continue }
    $name, $value = $line.Split('=', 2)
    if ($value.Trim().ToLowerInvariant() -eq 'on') { $modules.Add($name.Trim().ToLowerInvariant()) }
}
if ($Module) {
    if (-not $modules.Contains($Module.ToLowerInvariant())) {
        Write-Output ("ERROR: module '{0}' is not enabled in plugins.cfg (or does not exist)" -f $Module)
        exit 1
    }
    $modules = [System.Collections.Generic.List[string]]@($Module.ToLowerInvariant())
}

$translator = Resolve-Translator
if (-not $translator) { exit 1 }

# ---------------------------------------------------------------------------
# Mode: export English templates
# ---------------------------------------------------------------------------

if ($ExportTemplates) {
    Write-Output ("Exporting English templates for {0} module(s) -> {1}" -f $modules.Count, $templatesDir)
    New-Item -ItemType Directory -Path $templatesDir -Force | Out-Null
    $ok = 0
    $failed = New-Object System.Collections.Generic.List[string]

    foreach ($m in $modules) {
        $atpDir = Join-Path $atpRoot ("_template\{0}" -f $m)
        New-Item -ItemType Directory -Path $atpDir -Force | Out-Null

        $atp = & (Join-Path $scriptDir 'gen_atp.ps1') -Module $m -Language '_template' `
            -RepoRoot $RepoRoot -OutDir $OutDir -AtpDir $atpDir -ScratchTarget
        if ($LASTEXITCODE -ne 0) { $atp | ForEach-Object { Write-Output $_ }; $failed.Add($m); continue }

        # The scratch target is a copy of english.slg, so every "translation"
        # equals its English original -- exactly the skeleton the merge needs.
        $langDir = if ($m -eq 'salamand') { Join-Path $OutDir 'lang' }
        else { Join-Path (Join-Path (Join-Path $OutDir 'plugins') $m) 'lang' }
        Copy-Item -LiteralPath (Join-Path $langDir 'english.slg') `
            -Destination (Join-Path $atpDir ("{0}.scratch.slg" -f $m)) -Force

        # Delete any previous template first. ExportAsTextArchive refuses to
        # overwrite silently: for an English-build module (which a copy of
        # english.slg always is) it asks "overwriting unknown file" whenever the
        # .slt already exists, and that prompt is a modal dialog that would hang
        # the export. Removing the file first is what makes this step idempotent.
        $existing = Join-Path $templatesDir ("{0}.slt" -f $m)
        Remove-Item -LiteralPath $existing -Force -ErrorAction SilentlyContinue

        if (Invoke-Translator -Exe $translator -Arguments @('-quiet-export-slt', $templatesDir, $atp) -What ("template {0}" -f $m)) {
            $ok++
        }
        else { $failed.Add($m) }
    }

    Write-Output ("  templates exported: {0}/{1}" -f $ok, $modules.Count)
    if ($failed.Count -gt 0) {
        Write-Output ("ERROR: template export failed for: {0}" -f ($failed -join ', '))
        exit 1
    }
    exit 0
}

# ---------------------------------------------------------------------------
# Mode: build language modules
# ---------------------------------------------------------------------------

Write-Output ("Building language modules: {0} module(s) x {1} language(s)" -f $modules.Count, $languages.Count)

$built = 0
$upToDate = 0
$skippedNoSource = 0
$layoutErrors = 0
$failures = New-Object System.Collections.Generic.List[string]
$layoutList = New-Object System.Collections.Generic.List[string]

foreach ($m in $modules) {
    $langDir = if ($m -eq 'salamand') { Join-Path $OutDir 'lang' }
    else { Join-Path (Join-Path (Join-Path $OutDir 'plugins') $m) 'lang' }
    $englishSlg = Join-Path $langDir 'english.slg'

    if (-not (Test-Path -LiteralPath $englishSlg -PathType Leaf)) {
        $failures.Add(("{0}: english.slg not found at '{1}'" -f $m, $englishSlg))
        continue
    }

    foreach ($lang in $languages) {
        $slt = Join-Path $translationsRoot ("{0}\{1}.slt" -f $lang.Folder, $m)
        if (-not (Test-Path -LiteralPath $slt -PathType Leaf)) {
            # No translation source for this pair yet. Not an error -- the
            # merge stage has simply not produced it. It IS reported, because a
            # missing pair is what makes the product prompt per plugin.
            $skippedNoSource++
            continue
        }

        $what = ("{0}/{1}" -f $lang.Folder, $m)
        $targetSlg = Join-Path $langDir ("{0}.slg" -f $lang.Folder)
        $atpDir = Join-Path $atpRoot ("{0}\{1}" -f $lang.Folder, $m)
        $sltDir = Split-Path -Parent $slt

        # Incremental: nothing to do when the existing module is newer than both
        # of its inputs. Skips ~220 process launches on an unchanged rebuild.
        if (-not $Force -and (Test-Path -LiteralPath $targetSlg -PathType Leaf)) {
            $out = (Get-Item -LiteralPath $targetSlg).LastWriteTimeUtc
            if ($out -gt (Get-Item -LiteralPath $slt).LastWriteTimeUtc -and
                $out -gt (Get-Item -LiteralPath $englishSlg).LastWriteTimeUtc) {
                $upToDate++
                continue
            }
        }

        # 1. seed -- Save() patches a copy, and "none" suppresses the prompt
        Copy-Item -LiteralPath $englishSlg -Destination $targetSlg -Force

        # 2. project
        $atp = & (Join-Path $scriptDir 'gen_atp.ps1') -Module $m -Language $lang.Folder `
            -RepoRoot $RepoRoot -OutDir $OutDir -AtpDir $atpDir
        if ($LASTEXITCODE -ne 0) {
            $atp | ForEach-Object { Write-Output $_ }
            $failures.Add(("{0}: could not generate .atp" -f $what))
            continue
        }

        # 3. import
        if (-not (Invoke-Translator -Exe $translator -Arguments @('-quiet-import-slt', $sltDir, $atp) -What $what)) {
            $failures.Add(("{0}: import failed" -f $what))
            continue
        }
        $built++

        # 4. layout gate (spec FR-013 / SC-005). A translation longer than its
        # English original can push a control out of its dialog, and this is the
        # Translator's own layout validation.
        #
        # OPT-IN, because it is expensive in exactly the case it exists for:
        # -quiet-validate-layout exits cleanly only when there are NO findings
        # (wndframe.cpp:361). With findings it falls through to interactive mode
        # and sits there until the guard kills it, so every reported module
        # costs a full timeout. That is fine for a deliberate QA pass and far
        # too slow for every build.
        if ($CheckLayout) {
            if (-not (Invoke-Translator -Exe $translator -Arguments @('-quiet-validate-layout', $atp) -What ("layout {0}" -f $what))) {
                $layoutErrors++
                $layoutList.Add($what)
            }
        }
    }
}

# --- cleanup: the Translator leaves a .bak beside every target it patches ---
foreach ($m in $modules) {
    $langDir = if ($m -eq 'salamand') { Join-Path $OutDir 'lang' }
    else { Join-Path (Join-Path (Join-Path $OutDir 'plugins') $m) 'lang' }
    if (Test-Path -LiteralPath $langDir -PathType Container) {
        Get-ChildItem -LiteralPath $langDir -Filter '*.bak' -File -ErrorAction SilentlyContinue |
        Remove-Item -Force -ErrorAction SilentlyContinue
    }
}

Write-Output ("  language modules built: {0}" -f $built)
if ($upToDate -gt 0) {
    Write-Output ("  up to date (skipped)   : {0}" -f $upToDate)
}
if ($layoutErrors -gt 0) {
    Write-Output ("  LAYOUT WARNINGS        : {0} module(s) have clipped or overlapping controls:" -f $layoutErrors)
    foreach ($l in $layoutList) { Write-Output ("      {0}" -f $l) }
    Write-Output "      Fix by widening the control (cx) in the offending translations\<lang>\<module>.slt"
}
if ($skippedNoSource -gt 0) {
    Write-Output ("  skipped (no .slt yet)  : {0}  -- run: python -m translate.merge --all" -f $skippedNoSource)
}

# --- version check (FR-026) ------------------------------------------------
if ($built -gt 0) {
    $verifyArgs = @{ OutDir = $OutDir }
    if ($Module) { $verifyArgs['Module'] = $Module }
    & (Join-Path $scriptDir 'verify_slg.ps1') @verifyArgs
    if ($LASTEXITCODE -ne 0) { exit 1 }
}

if ($failures.Count -gt 0) {
    Write-Output ''
    Write-Output ("ERROR: {0} pair(s) failed:" -f $failures.Count)
    foreach ($f in $failures) { Write-Output ("       {0}" -f $f) }
    exit 1
}
exit 0
