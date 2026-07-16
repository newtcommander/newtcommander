@echo off
setlocal enabledelayedexpansion

:: Open Salamander Build Script
:: Usage: build.cmd [rebuild] [release] [full]
::   (no args)       Incremental Debug x64 build
::   rebuild         Full clean + rebuild
::   release         Release x64 build
::   rebuild release Full clean + rebuild Release x64
::   full            Complete build: app + all plugins + language modules,
::                   plus runtime data files (conversion tables, toolbars,
::                   sample scripts) and plugins\plugins.ver so that all
::                   built plugins auto-register in Plugin Manager
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
set "BUILD_FULL=0"

:: Parse all arguments (order-independent)
for %%a in (%*) do (
    if /i "%%~a"=="rebuild" set "BUILD_TARGET=rebuild"
    if /i "%%~a"=="release" set "BUILD_CONFIG=Release"
    if /i "%%~a"=="full" set "BUILD_FULL=1"
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
:: Plugin build policy (plugins.cfg)
:: ============================================================
:: Validate plugins.cfg (repo root), generate the solution filter
:: src\vcxproj\salamand.gen.slnf that excludes disabled plugins from
:: the MSBuild solution build, and reconcile the output plugins
:: directory with the policy: stale outputs of disabled or removed
:: plugins are deleted and plugins.ver is filtered down to the enabled
:: set. Any validation error (missing file, syntax error, unknown or
:: duplicate entry, unlisted plugin) stops the build before MSBuild
:: runs. See specs\007-plugin-build-policy\ for the contract.

set "OUT_DIR=%OPENSAL_BUILD_DIR%salamander\%BUILD_CONFIG%_%BUILD_PLATFORM%"
set "PLUGINS_STAGE_LOG=%TEMP%\opensal_plugins_stage.txt"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0src\vcxproj\gen_plugins_filter.ps1" -Config "%~dp0plugins.cfg" -Solution "%~dp0src\vcxproj\salamand.sln" -OutSlnf "%~dp0src\vcxproj\salamand.gen.slnf" -PluginsRoot "%~dp0src\plugins" -OutputPluginsDir "%OUT_DIR%\plugins" > "%PLUGINS_STAGE_LOG%" 2>&1
set "PLUGINS_STAGE_EXIT=%errorlevel%"
type "%PLUGINS_STAGE_LOG%"
set "ENABLED_COUNT="
for /f "tokens=2" %%n in ('findstr /b /c:"Plugins:" "%PLUGINS_STAGE_LOG%"') do set "ENABLED_COUNT=%%n"
del "%PLUGINS_STAGE_LOG%" >nul 2>&1
if not "%PLUGINS_STAGE_EXIT%"=="0" (
    echo.
    echo Plugin policy check FAILED. Fix plugins.cfg and try again.
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
echo  Plugin policy : %ENABLED_COUNT% plugins enabled ^(plugins.cfg^)
if "%BUILD_FULL%"=="1" echo  Full build    : runtime data files + plugins.ver
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

"%MSBUILD_PATH%" salamand.gen.slnf /t:%BUILD_TARGET% "/p:Configuration=%BUILD_CONFIG%" /p:Platform=%BUILD_PLATFORM% /m:%NUMBER_OF_PROCESSORS%

set "BUILD_EXIT=%errorlevel%"

popd

:: Full build: copy runtime data files and generate plugins.ver
if %BUILD_EXIT% equ 0 if "%BUILD_FULL%"=="1" (
    call :populate_runtime
    if errorlevel 1 set "BUILD_EXIT=1"
)

:: Non-full builds: if an earlier full build created plugins.ver, keep it in
:: sync with the enabled set (a plugin re-enabled in plugins.cfg must be
:: registered again for auto-install; disabled entries were already removed
:: by the policy stage reconciliation)
if %BUILD_EXIT% equ 0 if not "%BUILD_FULL%"=="1" if exist "%OUT_DIR%\plugins\plugins.ver" (
    call :gen_plugins_ver
    if errorlevel 1 set "BUILD_EXIT=1"
)

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
if %BUILD_EXIT% equ 0 if "%BUILD_FULL%"=="1" (
    echo  Plugins       : %PLUG_COUNT% registered in plugins.ver ^(version %NEW_VER%^)
    echo  Languages     : %LANG_COUNT% language modules ^(english^)
)
echo ============================================================
echo.

exit /b %BUILD_EXIT%

:: ============================================================
:: Full build: populate runtime layout
:: ============================================================
:: Copies runtime data files next to the built binaries and
:: generates plugins\plugins.ver (same format as the installer:
:: first line = version number, then "<version>:<relative .spl
:: path>" lines). On next start Salamander auto-installs every
:: entry newer than its registry counter and skips plugins it
:: already knows, so all built plugins appear in Plugin Manager.

:populate_runtime
set "OUT_DIR=%OPENSAL_BUILD_DIR%salamander\%BUILD_CONFIG%_%BUILD_PLATFORM%"
echo.
echo Populating runtime layout: %OUT_DIR%

if not exist "%OUT_DIR%\plugins" if not "%ENABLED_COUNT%"=="0" (
    echo ERROR: Plugins output directory not found: !OUT_DIR!\plugins
    exit /b 1
)

:: Character conversion tables (viewer "Convert" feature)
for %%d in (centeuro cyrillic westeuro) do (
    robocopy "%~dp0convert\%%d" "%OUT_DIR%\convert\%%d" convert.cfg *.tab >nul
    if errorlevel 8 (
        echo ERROR: Failed to copy conversion tables: %%d
        exit /b 1
    )
)

:: Toolbar icons
robocopy "%~dp0src\res\toolbars" "%OUT_DIR%\toolbars" >nul
if errorlevel 8 (
    echo ERROR: Failed to copy toolbar icons.
    exit /b 1
)

:: Automation plugin: sample scripts (only when the plugin is built - see plugins.cfg)
if exist "%OUT_DIR%\plugins\automation\" (
    robocopy "%~dp0src\plugins\automation\sample-scripts" "%OUT_DIR%\plugins\automation\scripts" >nul
    if errorlevel 8 (
        echo ERROR: Failed to copy automation sample scripts.
        exit /b 1
    )
)

:: ZIP plugin: Zip2SFX configuration samples (only when the plugin is built - see plugins.cfg)
if exist "%OUT_DIR%\plugins\zip\" (
    robocopy "%~dp0src\plugins\zip\zip2sfx" "%OUT_DIR%\plugins\zip\zip2sfx" readme.txt sam_cz.set sample.set >nul
    if errorlevel 8 (
        echo ERROR: Failed to copy zip2sfx files.
        exit /b 1
    )
)

:: Generate plugins\plugins.ver (shared with the non-full sync path)
call :gen_plugins_ver
if errorlevel 1 exit /b 1

:: Count built language modules (.slg)
set "LANG_COUNT=0"
for /r "%OUT_DIR%" %%f in (*.slg) do (
    set "REL=%%f"
    if /i "!REL!"=="!REL:Intermediate=!" set /a LANG_COUNT+=1
)

echo   language modules built: %LANG_COUNT% ^(english^)
echo.
echo   NOTE: Translations in translations\ ^(czech, german, ...^) are Translator
echo         source data ^(.slt^) and cannot be compiled from this repository.
echo   NOTE: unrar additionally needs unrar.dll at runtime ^(not in repo^);
echo         pictview runs on the built-in Windows imaging ^(WIC^)
echo         engine since feature 006.
exit /b 0

:: ============================================================
:: plugins.ver generation / sync
:: ============================================================
:: Writes plugins\plugins.ver from the .spl files actually present in
:: the output (which the policy stage has already reconciled with
:: plugins.cfg). The version number MUST increase monotonically across
:: ALL builds regardless of configuration: Salamander records the last
:: processed version in the registry (Configuration.LastPluginVer)
:: under a single key shared by Debug and Release of the same platform
:: (Software\Open Salamander\5.0). A version that is <= the recorded
:: one is silently skipped (plugins2.cpp) and the build's plugins never
:: auto-install into Plugin Manager - the symptom being "a freshly
:: built plugin is missing" after switching Debug<->Release. A
:: per-directory counter restarting at 1 caused exactly that. Use a
:: clock-based token (minutes since 2000-01-01, fits a 32-bit int until
:: ~4085) so every build strictly increases and escapes any value
:: already written to the registry by earlier builds.

:gen_plugins_ver
set "PLUG_DIR=%OUT_DIR%\plugins"
set "PLUG_VER=%PLUG_DIR%\plugins.ver"
set "PLUG_COUNT=0"
set "NEW_VER=0"
if "%ENABLED_COUNT%"=="0" (
    echo   plugin policy disables all plugins - skipping plugins.ver generation
    exit /b 0
)
set "NEW_VER="
for /f "usebackq delims=" %%v in (`powershell -NoProfile -Command "[int][math]::Floor(((Get-Date).ToUniversalTime() - [datetime]'2000-01-01').TotalMinutes)"`) do set "NEW_VER=%%v"
if not defined NEW_VER set "NEW_VER=1"
> "%PLUG_VER%" echo %NEW_VER%
for %%p in ("%PLUG_DIR%") do for /r "%PLUG_DIR%" %%f in (*.spl) do (
    set "REL=%%f"
    set "REL=!REL:%%~p\=!"
    if /i "!REL!"=="!REL:Intermediate=!" (
        >> "%PLUG_VER%" echo %NEW_VER%:!REL!
        set /a PLUG_COUNT+=1
    )
)

if %PLUG_COUNT% equ 0 (
    echo ERROR: No plugins ^(*.spl^) found in !PLUG_DIR! - plugins.cfg enables %ENABLED_COUNT%.
    exit /b 1
)
echo   plugins.ver version %NEW_VER%: %PLUG_COUNT% plugins registered for auto-install
exit /b 0

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
echo   full             Complete build: app + all plugins + language modules,
echo                    plus runtime data files (conversion tables, toolbars,
echo                    sample scripts) and plugins\plugins.ver so that all
echo                    built plugins appear in Plugin Manager on next start
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
echo   build.cmd full             Complete Debug build (plugins in Plugin Manager)
echo   build.cmd full release     Complete Release build
echo.
exit /b 0
