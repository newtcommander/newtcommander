# Feature 027 — static exhaustion check.
# Repeatable guard proving the long-path CRASH sites resolved by features
# 011-015 + 027 stay resolved, and flagging NEW dangerous patterns in the
# core path-handling files for triage against audit/INVENTORY.md.
# Run:  pwsh -File check.ps1   (exit 0 = clean, 1 = regression)
$ErrorActionPreference = 'Stop'
$src = Join-Path $PSScriptRoot '..\..\..\src' | Resolve-Path
$fail = 0
function Grep($file, $pattern) {
    $p = Join-Path $src $file
    if (-not (Test-Path $p)) { Write-Output "MISSING $file"; return @() }
    Select-String -LiteralPath $p -Pattern $pattern -AllMatches
}
function MustNot($file, $pattern, $why) {
    $hits = Grep $file $pattern
    if ($hits) {
        $script:fail = 1
        Write-Output "REGRESSION [$file] $why"
        $hits | ForEach-Object { Write-Output ("   line {0}: {1}" -f $_.LineNumber, $_.Line.Trim()) }
    }
}
function Must($file, $pattern, $why) {
    if (-not (Grep $file $pattern)) {
        $script:fail = 1
        Write-Output "REGRESSION [$file] expected but missing: $why"
    }
}

# --- resolved crash sites must stay resolved ---
# D1: CDrivesList::CurrentPath widened + bounded copy
MustNot 'drivelst.h'  'char CurrentPath\[MAX_PATH\]'      'CurrentPath must stay SAL_MAX_PATH_UTF8 (Alt+F1 crash D1)'
Must    'drivelst.h'  'char CurrentPath\[SAL_MAX_PATH_UTF8\]' 'CurrentPath widened'
MustNot 'drivelst.cpp' 'lstrcpy\(CurrentPath,'            'CurrentPath copy must be bounded (lstrcpyn)'

# D2: CExecuteExpData::Buffer widened
MustNot 'execute.cpp' 'char Buffer\[MAX_PATH\]'           'CExecuteExpData::Buffer must stay SAL (F4 crash D2)'
Must    'execute.cpp' 'char Buffer\[SAL_MAX_PATH_UTF8\]'  'CExecuteExpData::Buffer widened'

# F5/F6 engine gates raised off MAX_PATH/PATH_MAX_PATH
MustNot 'fileswn6.cpp' '>= MAX_PATH - 2'                  'BuildScriptDir source gate must be SAL'
MustNot 'salamdr5.cpp' '!CreateDirectory\(newDirs'       'intermediate dir create must be W-backed (SalCreateDirectory)'

# security/encrypt on the copy route must be W-backed
MustNot 'worker.cpp' 'GetNamedSecurityInfo\(\(char\*\)'  'security info must use SalGetNamedSecurityInfo (W)'
MustNot 'worker.cpp' 'SetNamedSecurityInfo\(\(char\*\)'  'security info must use SalSetNamedSecurityInfo (W)'

# clipboard long-path module present
Must 'common\salclip.cpp' 'SalBuildWideDropFiles'        'long-path clipboard builder present'

# --- heuristic: NEW unbounded full-path copies into MAX_PATH buffers ---
# (advisory: any hit not already an INVENTORY row needs classification)
Write-Output ''
Write-Output '--- advisory heuristic scan (triage against INVENTORY.md) ---'
$watch = 'drivelst.cpp','execute.cpp','fileswn5.cpp','fileswn6.cpp','fileswn8.cpp','shellib.cpp','shellsup.cpp','viewer2.cpp'
$advisory = 0
foreach ($f in $watch) {
    # a char[MAX_PATH] local that is the destination of a bare strcpy/lstrcpy (not lstrcpyn)
    $decls = Grep $f 'char\s+(\w+)\[MAX_PATH\]'
    foreach ($d in $decls) {
        if ($d.Matches[0].Groups[1].Value) {
            $name = $d.Matches[0].Groups[1].Value
            $bad = Grep $f ("(?<![n])(?:strcpy|lstrcpy)\(" + [regex]::Escape($name) + ",")
            if ($bad) {
                $advisory++
                Write-Output ("  [$f] '$name' char[MAX_PATH] with bare strcpy/lstrcpy:")
                $bad | ForEach-Object { Write-Output ("     line {0}: {1}" -f $_.LineNumber, $_.Line.Trim()) }
            }
        }
    }
}
if ($advisory -eq 0) { Write-Output '  none' }

Write-Output ''
if ($fail -eq 0) { Write-Output 'CHECK PASS — no CRASH-site regressions' }
else { Write-Output 'CHECK FAIL — see REGRESSION lines above' }
exit $fail
