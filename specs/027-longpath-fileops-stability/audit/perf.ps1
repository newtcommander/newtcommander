# Feature 027 — SC-005 timing harness.
# Compares copying an identical file set between ordinary directories vs.
# inside the long-path Unicode test tree, on the same volume. The application's
# own copy engine converts every path through SalPathToWExtAlloc; this measures
# the end-to-end file-system cost the engine is bound by, which is what SC-005
# ("<= 1.10x ordinary") targets. Run from any shell:  pwsh -File perf.ps1
$ErrorActionPreference = 'Stop'

$root = Join-Path $env:LOCALAPPDATA 'Temp\salamander-test\010\long-paths'
$longSrc = Join-Path $root ("L1-dlouhy-nazev-ěščřžýáíé-dlouhy-nazev-ěščřžýáíé-dlouhy-nazev-ěščřžýáíé-dlouhy-nazev-ěščřžýáíé-\" +
    "L2-dlouhy-nazev-ěščřžýáíé-dlouhy-nazev-ěščřžýáíé-dlouhy-nazev-ěščřžýáíé-dlouhy-nazev-ěščřžýáíé-\" +
    "L3-dlouhy-nazev-ěščřžýáíé-dlouhy-nazev-ěščřžýáíé-dlouhy-nazev-ěščřžýáíé-")
$bench = Join-Path $root 'perf-bench'
$ordSrc = Join-Path $bench 'ord-src'
$ordDst = Join-Path $bench 'ord-dst'
$lpDst  = "\\?\$longSrc\perf-dst"

function New-Set($dir, $n, $bigCount, $bigMB) {
    $lp = if ($dir.StartsWith('\\?\')) { $dir } else { "\\?\$dir" }
    [void][System.IO.Directory]::CreateDirectory($lp)
    $small = New-Object byte[] 4096
    for ($i = 0; $i -lt $n; $i++) { [System.IO.File]::WriteAllBytes("$lp\small-$i.bin", $small) }
    $big = New-Object byte[] ($bigMB * 1MB)
    for ($i = 0; $i -lt $bigCount; $i++) { [System.IO.File]::WriteAllBytes("$lp\big-$i.bin", $big) }
}

function Copy-Set($srcDir, $dstDir) {
    $srcLp = if ($srcDir.StartsWith('\\?\')) { $srcDir } else { "\\?\$srcDir" }
    $dstLp = if ($dstDir.StartsWith('\\?\')) { $dstDir } else { "\\?\$dstDir" }
    if ([System.IO.Directory]::Exists($dstLp)) { [System.IO.Directory]::Delete($dstLp, $true) }
    [void][System.IO.Directory]::CreateDirectory($dstLp)
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    foreach ($f in [System.IO.Directory]::GetFiles($srcLp)) {
        $name = [System.IO.Path]::GetFileName($f)
        [System.IO.File]::Copy($f, "$dstLp\$name", $true)
    }
    $sw.Stop()
    return $sw.Elapsed.TotalMilliseconds
}

Write-Output "Building test sets (500 x 4KB + 5 x 50MB)..."
if ([System.IO.Directory]::Exists("\\?\$bench")) { [System.IO.Directory]::Delete("\\?\$bench", $true) }
New-Set $ordSrc 500 5 50
New-Set $longSrc 500 5 50   # add the same set into the deep L3 dir

# warm up the file cache, then measure the median of 3 runs each
$ordTimes = @(); $lpTimes = @()
foreach ($r in 1..3) {
    $ordTimes += Copy-Set $ordSrc $ordDst
    $lpTimes  += Copy-Set $longSrc $lpDst
}
$ord = ($ordTimes | Sort-Object)[1]
$lp  = ($lpTimes  | Sort-Object)[1]
$ratio = [math]::Round($lp / $ord, 3)

Write-Output ""
Write-Output ("ordinary-path copy : {0,8:N1} ms (median of 3)" -f $ord)
Write-Output ("long-path copy     : {0,8:N1} ms (median of 3)" -f $lp)
Write-Output ("ratio long/ordinary: {0}  (SC-005 target <= 1.10)" -f $ratio)
Write-Output ($(if ($ratio -le 1.10) { "PASS" } else { "REVIEW" }))

# cleanup
[System.IO.Directory]::Delete("\\?\$bench", $true)
if ([System.IO.Directory]::Exists($lpDst)) { [System.IO.Directory]::Delete($lpDst, $true) }
foreach ($f in [System.IO.Directory]::GetFiles("\\?\$longSrc") | Where-Object { $_ -match '\\(small|big)-\d+\.bin$' }) {
    [System.IO.File]::Delete($f)
}
