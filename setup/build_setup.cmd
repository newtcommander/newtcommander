@echo off
setlocal enabledelayedexpansion

:: Tandem Commander installer build (feature 050)
:: Usage: build_setup.cmd [sign]
::   (no args)  Compile the Inno Setup installer from the existing Release
::              output tree - no signing, no certificate dependencies.
::   sign       First run the code-signing sweep over the Release tree (so a
::              signed installer can never package unsigned binaries), then
::              compile with /DSIGN=1 so Inno signs the installer AND the
::              uninstaller stub with the certificate + timestamp authority
::              from tools\codesign\codesign.cfg, then verify the result.
::
:: Output: setup\output\tandemcommander-<version>-x64-setup.exe

if /i "%~1"=="help" goto :show_help
if /i "%~1"=="/?" goto :show_help
if /i "%~1"=="-h" goto :show_help

set "SIGN_MODE=0"
for %%a in (%*) do (
    if /i "%%~a"=="sign" set "SIGN_MODE=1"
)

set "SETUP_DIR=%~dp0"
set "REPO_ROOT=%~dp0..\"
set "CODESIGN_DIR=%REPO_ROOT%tools\codesign\"

:: Resolve the build output tree the same way build.cmd does
if not defined OPENSAL_BUILD_DIR set "OPENSAL_BUILD_DIR=%REPO_ROOT%build\"
set "OUT_DIR=%OPENSAL_BUILD_DIR%tandemcommander\Release_x64"

if not exist "%OUT_DIR%\tandemcommander.exe" (
    echo ERROR: Release output not found: %OUT_DIR%\tandemcommander.exe
    echo   Run "build.cmd full release" first.
    exit /b 1
)

:: ============================================================
:: Locate the Inno Setup compiler
:: ============================================================
set "ISCC="
if exist "%ProgramFiles%\Inno Setup 7\ISCC.exe" set "ISCC=%ProgramFiles%\Inno Setup 7\ISCC.exe"
if not defined ISCC (
    for /f "usebackq delims=" %%p in (`where iscc 2^>nul`) do if not defined ISCC set "ISCC=%%p"
)
if not defined ISCC if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not defined ISCC (
    echo ERROR: Inno Setup compiler ^(ISCC.exe^) not found.
    echo   Install Inno Setup 7 from https://jrsoftware.org/isinfo.php
    exit /b 1
)

echo.
echo ============================================================
echo  Tandem Commander Installer Build
echo ============================================================
echo  Release tree  : %OUT_DIR%
echo  Inno Setup    : %ISCC%
if "%SIGN_MODE%"=="1" (echo  Code signing  : requested) else (echo  Code signing  : no)
echo ============================================================
echo.

if "%SIGN_MODE%"=="0" goto :compile_unsigned

:: ============================================================
:: Signed build: sweep the tree first (FR-006 - a signed installer
:: never packages unsigned shipped binaries)
:: ============================================================
echo Ensuring the release tree is fully signed...
powershell -NoProfile -ExecutionPolicy Bypass -File "%CODESIGN_DIR%sign_release.ps1" -Root "%OUT_DIR%"
if errorlevel 1 (
    echo ERROR: release tree signing failed - installer NOT built.
    exit /b 1
)

:: Read the signing profile (single committed source: codesign.cfg)
set "CS_THUMB="
set "CS_TSA="
set "PROFILE_TMP=%TEMP%\tc_signprofile.txt"
powershell -NoProfile -ExecutionPolicy Bypass -Command "$m=@{}; Get-Content -LiteralPath '%CODESIGN_DIR%codesign.cfg' | ForEach-Object { if ($_ -notmatch '^\s*(#|$)' -and $_ -match '^\s*([A-Za-z_]+)\s*=\s*(.+?)\s*$') { $m[$Matches[1].ToLower()]=$Matches[2] } }; Write-Output ('THUMB=' + $m['thumbprint']); Write-Output ('TSA=' + $m['timestamp_url'])" > "%PROFILE_TMP%"
for /f "usebackq tokens=1,* delims==" %%a in ("%PROFILE_TMP%") do set "CS_%%a=%%b"
del "%PROFILE_TMP%" >nul 2>&1
if not defined CS_THUMB (
    echo ERROR: could not read 'thumbprint' from tools\codesign\codesign.cfg
    exit /b 1
)
if not defined CS_TSA (
    echo ERROR: could not read 'timestamp_url' from tools\codesign\codesign.cfg
    exit /b 1
)

:: Resolve signtool.exe (newest Windows 10/11 SDK, x64)
set "SIGNTOOL="
for /f "usebackq delims=" %%p in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "$b = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'; if (Test-Path -LiteralPath $b) { $d = Get-ChildItem -LiteralPath $b -Directory | Where-Object { $_.Name -match '^10\.[0-9.]+$' } | Sort-Object { [version]$_.Name } -Descending; foreach ($x in $d) { $t = Join-Path $x.FullName 'x64\signtool.exe'; if (Test-Path -LiteralPath $t) { Write-Output $t; break } } }"`) do if not defined SIGNTOOL set "SIGNTOOL=%%p"
if not defined SIGNTOOL (
    for /f "usebackq delims=" %%p in (`where signtool 2^>nul`) do if not defined SIGNTOOL set "SIGNTOOL=%%p"
)
if not defined SIGNTOOL (
    echo ERROR: signtool.exe not found ^(Windows 10/11 SDK is a build prerequisite^).
    exit /b 1
)

echo.
echo Compiling signed installer...
"%ISCC%" /DSIGN=1 "/Stcsign=$q%SIGNTOOL%$q sign /sha1 %CS_THUMB% /tr %CS_TSA% /td sha256 /fd sha256 /v $f" "%SETUP_DIR%tandemcommander.iss"
if errorlevel 1 (
    echo ERROR: Inno Setup compilation failed.
    exit /b 1
)

:: Verify the produced installer signature (newest setup exe in output\)
powershell -NoProfile -ExecutionPolicy Bypass -Command "$f = Get-ChildItem -LiteralPath '%SETUP_DIR%output' -Filter 'tandemcommander-*-setup.exe' | Sort-Object LastWriteTime -Descending | Select-Object -First 1; if (-not $f) { Write-Host 'ERROR: no installer found in setup\output'; exit 1 }; $s = Get-AuthenticodeSignature -LiteralPath $f.FullName; if ($s.Status -eq 'Valid' -and $null -ne $s.SignerCertificate -and $s.SignerCertificate.Thumbprint -eq '%CS_THUMB%'.ToUpper()) { Write-Host ('Installer signed and verified: ' + $f.Name); exit 0 } else { Write-Host ('ERROR: installer signature invalid: ' + $f.Name + ' (status: ' + $s.Status + ')'); exit 1 }"
if errorlevel 1 exit /b 1

echo.
echo Signed installer ready in %SETUP_DIR%output\
exit /b 0

:: ============================================================
:: Unsigned build (development test) - no signing dependencies
:: ============================================================
:compile_unsigned
echo Compiling unsigned installer...
"%ISCC%" "%SETUP_DIR%tandemcommander.iss"
if errorlevel 1 (
    echo ERROR: Inno Setup compilation failed.
    exit /b 1
)
echo.
echo Unsigned installer ready in %SETUP_DIR%output\
exit /b 0

:show_help
echo.
echo Tandem Commander Installer Build
echo.
echo Usage: build_setup.cmd [sign]
echo.
echo   (no args)  Compile the installer unsigned (no cert/signtool needed)
echo   sign       Sign the whole Release tree first, then compile the
echo              installer with a signed setup.exe and uninstaller
echo              (certificate: tools\codesign\codesign.cfg)
echo.
echo The Release tree must exist first: run "build.cmd full release".
echo Output: setup\output\tandemcommander-^<version^>-x64-setup.exe
echo.
exit /b 0
