@echo off
:: SPDX-License-Identifier: GPL-2.0-or-later
:: Builds and runs the SFTP plugin's pure-logic unit test harness.
:: Compiles sftputils.cpp standalone in a flat temp dir using a shim precomp.h.
setlocal

set "HERE=%~dp0"
set "WORK=%TEMP%\sftp_test_%RANDOM%"
mkdir "%WORK%" 2>nul

copy /y "%HERE%harness.cpp" "%WORK%\harness.cpp" >nul
copy /y "%HERE%..\sftputils.cpp" "%WORK%\sftputils.cpp" >nul
copy /y "%HERE%..\sftputils.h" "%WORK%\sftputils.h" >nul
copy /y "%HERE%precomp_shim.h" "%WORK%\precomp.h" >nul

:: locate the VS2022 developer environment
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
if not defined VSINSTALL (
    echo ERROR: Visual Studio 2022 with C++ tools not found.
    exit /b 1
)

call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1

pushd "%WORK%"
cl /nologo /EHsc /W3 /std:c++latest harness.cpp sftputils.cpp /Fe:harness.exe
set "CLEXIT=%errorlevel%"
if %CLEXIT% neq 0 ( popd & echo COMPILE FAILED & exit /b 1 )

harness.exe
set "RUNEXIT=%errorlevel%"
popd

rmdir /s /q "%WORK%" 2>nul
exit /b %RUNEXIT%
