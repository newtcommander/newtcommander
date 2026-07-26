# gen_atp.ps1 - Translator project (.atp) generator for build_langs.cmd
#
# Writes a minimal .atp describing one (module, language) pair:
#
#     [Files]
#     Original=<...>\english.slg      the module built from the .rc sources
#     Translated=<...>\<language>.slg the target, patched in place
#     Include=<...>\lang.rh           resource header (MANDATORY)
#
# Only these three keys are emitted. SalMenu, IgnoreList, CheckList,
# SalamanderExe, and Export are optional -- their loaders in the Translator all
# guard on an empty path (checklst.cpp:190, salmenu.cpp:261, ignorelst.cpp:262),
# so omitting them is safe. Include= is NOT optional: CFrameWindow::OpenProject
# (wndframe.cpp:344) gates on DataRH.Load(FullIncludeFile) succeeding.
#
# Paths are written absolute. CData::LoadProject resolves them through
# PathMerge (dataprj.cpp:193), which detects a drive letter or UNC prefix and
# uses the path as-is rather than appending it to the project directory.
#
# The project MUST be named <module>.atp: the Translator derives the .slt
# filename from the project basename (wndframe.cpp:489-493).
#
# Exit code 0 = success; 1 = a required input is missing ("ERROR: ..." lines).
# See specs/038-translations-build-integration/contracts/translator-cli.md

param(
    [Parameter(Mandatory = $true)][string]$Module,
    [Parameter(Mandatory = $true)][string]$Language,
    [Parameter(Mandatory = $true)][string]$RepoRoot,
    [Parameter(Mandatory = $true)][string]$OutDir,
    [Parameter(Mandatory = $true)][string]$AtpDir,
    # Emit a scratch target beside the .atp instead of the shipped <language>.slg.
    # Used by --export-templates, which only needs a readable module to export
    # English text from and must not disturb build output.
    [switch]$ScratchTarget
)

$ErrorActionPreference = 'Stop'
$errors = New-Object System.Collections.Generic.List[string]

# --- module layout (mirrors tools/translate/config.py Module) ---------------
$isApp = ($Module -eq 'salamand')
if ($isApp) {
    $langDir = Join-Path $OutDir 'lang'
    $rhPath = Join-Path $RepoRoot 'src\lang\lang.rh'
}
else {
    $langDir = Join-Path (Join-Path (Join-Path $OutDir 'plugins') $Module) 'lang'
    $rhPath = Join-Path $RepoRoot ("src\plugins\{0}\lang\lang.rh" -f $Module)
}

$englishSlg = Join-Path $langDir 'english.slg'

if (-not (Test-Path -LiteralPath $englishSlg -PathType Leaf)) {
    $errors.Add(("english.slg not found for module '{0}' at '{1}' -- build the language projects first" -f $Module, $englishSlg))
}
if (-not (Test-Path -LiteralPath $rhPath -PathType Leaf)) {
    $errors.Add(("lang.rh not found for module '{0}' at '{1}' -- Include= is mandatory" -f $Module, $rhPath))
}
if ($errors.Count -gt 0) {
    foreach ($e in $errors) { Write-Output ("ERROR: {0}" -f $e) }
    exit 1
}

if ($ScratchTarget) {
    $targetSlg = Join-Path $AtpDir ("{0}.scratch.slg" -f $Module)
}
else {
    $targetSlg = Join-Path $langDir ("{0}.slg" -f $Language)
}

if (-not (Test-Path -LiteralPath $AtpDir -PathType Container)) {
    New-Item -ItemType Directory -Path $AtpDir -Force | Out-Null
}

# --- BOM workaround --------------------------------------------------------
# 29 of the 31 lang.rh files start with a UTF-8 BOM, because UTF-8-BOM is this
# repository's source encoding standard. The Translator's resource-symbol
# parser predates that convention: it reads the .rh as ANSI and the BOM makes
# it fail with "Syntax error on line 1", which surfaces as a modal dialog and
# hangs the build.
#
# Rather than strip BOMs from source files (which would break the repo standard
# and touch 29 files for the benefit of one tool), point Include= at a
# BOM-stripped copy generated into the build tree. The .rh contains only ASCII
# identifiers, so dropping the BOM loses nothing.
$rhCopy = Join-Path $AtpDir 'lang.rh'
$rhBytes = [System.IO.File]::ReadAllBytes($rhPath)
if ($rhBytes.Length -ge 3 -and $rhBytes[0] -eq 0xEF -and $rhBytes[1] -eq 0xBB -and $rhBytes[2] -eq 0xBF) {
    [System.IO.File]::WriteAllBytes($rhCopy, $rhBytes[3..($rhBytes.Length - 1)])
}
else {
    [System.IO.File]::WriteAllBytes($rhCopy, $rhBytes)
}

$atpPath = Join-Path $AtpDir ("{0}.atp" -f $Module)

$lines = @(
    '[Files]',
    ("Original={0}" -f $englishSlg),
    ("Translated={0}" -f $targetSlg),
    ("Include={0}" -f $rhCopy),
    ''
)

# The .atp is read byte-wise as char* by CData::LoadProject, so write ANSI
# without a BOM -- a UTF-8 BOM would land in the "[Files]" section header and
# fail the strcmp.
[System.IO.File]::WriteAllText($atpPath, ($lines -join "`r`n"), [System.Text.Encoding]::Default)

Write-Output $atpPath
exit 0
