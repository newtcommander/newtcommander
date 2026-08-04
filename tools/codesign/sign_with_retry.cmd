@echo off
rem Per-target post-build signing hook (feature 050). Called by every
rem *_release.props as:  call ...\tools\codesign\sign_with_retry.cmd "$(TargetPath)"
rem
rem Default is a no-op so ordinary Release builds stay unsigned, fast and
rem offline (FR-002). Release signing normally happens as a whole-tree sweep
rem (build.cmd ... sign -> sign_release.ps1 -Root). Set TC_CODESIGN=1 to
rem opt in to signing each target as it is built, e.g. for binaries built
rem outside the default solution (shellext, salopen, ...).
if not defined TC_CODESIGN exit /b 0
if "%~1"=="" (
    echo sign_with_retry.cmd: missing target path argument
    exit /b 1
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0sign_release.ps1" -File "%~1"
exit /b %errorlevel%
