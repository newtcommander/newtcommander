@echo off
setlocal enabledelayedexpansion

:: Open Salamander Build Script
:: Usage: build.cmd [rebuild] [release]
::   (no args)       Incremental Debug x64 build
::   rebuild         Full clean + rebuild
::   release         Release x64 build
::   rebuild release Full clean + rebuild Release x64
::   help            Show this help message

if /i "%~1"=="help" goto :show_help
if /i "%~1"=="/?" goto :show_help
if /i "%~1"=="-h" goto :show_help

:: ============================================================
:: Parse arguments
:: ============================================================

set "BUILD_TARGET=build"
set "BUILD_CONFIG=Debug"
set "BUILD_PLATFORM=x64"

:: Parse all arguments (order-independent)
for %%a in (%*) do (
    if /i "%%~a"=="rebuild" set "BUILD_TARGET=rebuild"
    if /i "%%~a"=="release" set "BUILD_CONFIG=Release"
)

:: ============================================================
:: Prerequisite checks
:: ============================================================

set "PREREQ_FAIL=0"

:: Check OPENSAL_BUILD_DIR — default to .\build\ relative to this script
if not defined OPENSAL_BUILD_DIR (
    set "OPENSAL_BUILD_DIR=%~dp0build\"
    echo NOTE: OPENSAL_BUILD_DIR not set, defaulting to !OPENSAL_BUILD_DIR!
)

:: Locate Visual Studio 2022 via vswhere
set "MSBUILD_PATH="
set "VSWHERE="
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not defined VSWHERE goto :no_vswhere

:: Use vswhere to find VS2022 with C++ workload
set "VS_INSTALL="
set "VS_TMP=%TEMP%\opensal_vs_install.txt"
"!VSWHERE!" -latest -version "[17.0,18.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "!VS_TMP!" 2>nul
if exist "!VS_TMP!" (
    set /p VS_INSTALL=<"!VS_TMP!"
    del "!VS_TMP!" >nul 2>&1
)

if not defined VS_INSTALL (
    echo ERROR: Visual Studio 2022 with C++ Desktop workload not found.
    echo   Install Visual Studio 2022 from https://visualstudio.microsoft.com/downloads/
    echo   Ensure the "Desktop development with C++" workload is selected.
    set "PREREQ_FAIL=1"
    goto :prereq_done
)

set "MSBUILD_PATH=!VS_INSTALL!\Msbuild\Current\Bin\MSBuild.exe"
if not exist "!MSBUILD_PATH!" (
    echo ERROR: MSBuild.exe not found at expected path:
    echo   !MSBUILD_PATH!
    set "PREREQ_FAIL=1"
    set "MSBUILD_PATH="
)
goto :prereq_done

:no_vswhere
:: vswhere not found - try PATH fallback
where msbuild >nul 2>&1
if !errorlevel! equ 0 (
    set "MSBUILD_PATH=msbuild"
    echo NOTE: vswhere.exe not found. Using MSBuild from PATH.
    goto :prereq_done
)

:: Last resort: hardcoded Community path
set "MSBUILD_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\MSBuild.exe"
if exist "!MSBUILD_PATH!" (
    echo NOTE: Using hardcoded MSBuild path for Community edition.
    goto :prereq_done
)

echo ERROR: Cannot locate MSBuild. Neither vswhere.exe nor MSBuild in PATH found.
echo   Install Visual Studio 2022 from https://visualstudio.microsoft.com/downloads/
set "PREREQ_FAIL=1"
set "MSBUILD_PATH="

:prereq_done

if "%PREREQ_FAIL%"=="1" (
    echo.
    echo Prerequisites check FAILED. Fix the issues above and try again.
    exit /b 1
)

:: ============================================================
:: Display build configuration
:: ============================================================

echo.
echo ============================================================
echo  Open Salamander Build
echo ============================================================
echo  Configuration : %BUILD_CONFIG% %BUILD_PLATFORM%
echo  Mode          : %BUILD_TARGET%
echo  Output        : %OPENSAL_BUILD_DIR%salamander\%BUILD_CONFIG%_%BUILD_PLATFORM%\
echo  MSBuild       : %MSBUILD_PATH%
echo ============================================================
echo.

:: ============================================================
:: Run MSBuild
:: ============================================================

:: Record start time
set "START_TIME=%time%"

pushd "%~dp0src\vcxproj"

"%MSBUILD_PATH%" salamand.sln /t:%BUILD_TARGET% "/p:Configuration=%BUILD_CONFIG%" /p:Platform=%BUILD_PLATFORM% /m:%NUMBER_OF_PROCESSORS%

set "BUILD_EXIT=%errorlevel%"

popd

:: Record end time
set "END_TIME=%time%"

:: ============================================================
:: Calculate duration
:: ============================================================

:: Parse start time (HH:MM:SS.CC)
for /f "tokens=1-4 delims=:,." %%a in ("%START_TIME: =0%") do (
    set /a "START_S=%%a*3600 + %%b*60 + %%c"
)

:: Parse end time
for /f "tokens=1-4 delims=:,." %%a in ("%END_TIME: =0%") do (
    set /a "END_S=%%a*3600 + %%b*60 + %%c"
)

:: Handle midnight crossing
set /a "DURATION_S=END_S - START_S"
if %DURATION_S% lss 0 set /a "DURATION_S+=86400"

:: Format duration
set /a "DUR_M=DURATION_S / 60"
set /a "DUR_S=DURATION_S %% 60"

:: ============================================================
:: Build summary
:: ============================================================

echo.
echo ============================================================
if %BUILD_EXIT% equ 0 (
    echo  BUILD SUCCEEDED
) else (
    echo  BUILD FAILED  ^(exit code: %BUILD_EXIT%^)
)
echo  Configuration : %BUILD_CONFIG% %BUILD_PLATFORM%
echo  Duration      : %DUR_M% min %DUR_S% sec
echo  Output        : %OPENSAL_BUILD_DIR%salamander\%BUILD_CONFIG%_%BUILD_PLATFORM%\
echo ============================================================
echo.

exit /b %BUILD_EXIT%

:: ============================================================
:: Help
:: ============================================================

:show_help
echo.
echo Open Salamander Build Script
echo.
echo Usage: build.cmd [command]
echo.
echo Commands:
echo   (none)           Incremental Debug x64 build (default)
echo   rebuild          Full clean + rebuild
echo   release          Release x64 build
echo   help             Show this help message
echo.
echo   Arguments can be combined in any order.
echo.
echo Prerequisites:
echo   - Visual Studio 2022 with "Desktop development with C++" workload
echo   - Windows 10/11 SDK
echo   - OPENSAL_BUILD_DIR env var (optional, defaults to .\build\)
echo.
echo Examples:
echo   build.cmd                  Debug incremental build
echo   build.cmd release          Release incremental build
echo   build.cmd rebuild          Debug clean rebuild
echo   build.cmd rebuild release  Release clean rebuild
echo.
exit /b 0
