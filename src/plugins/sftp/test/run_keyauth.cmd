@echo off
:: SPDX-License-Identifier: GPL-2.0-or-later
:: Builds and runs the SFTP key-authentication regression harness (feature 051)
:: against the local reference server. Compiles key_auth.c together with the
:: vendored libssh2 sources (WinCNG backend), mirroring the plugin's own
:: compile settings from sftp.vcxproj: /RTCc and /RTC1 MUST stay off (see
:: src\common\dep\libssh2\readme.txt).
::
:: Usage: run_keyauth.cmd [harness arguments...]
::        run_keyauth.cmd --scenario key-rsa
setlocal

set "HERE=%~dp0"
set "LIBSSH2=%HERE%..\..\..\common\dep\libssh2"
set "WORK=%TEMP%\sftp_keyauth_%RANDOM%"
mkdir "%WORK%" 2>nul

:: locate the VS2022 developer environment
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSINSTALL="
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
)
if not defined VSINSTALL (
    for %%e in (Enterprise Professional Community BuildTools) do (
        if exist "%ProgramFiles%\Microsoft Visual Studio\2022\%%e\VC\Auxiliary\Build\vcvars64.bat" set "VSINSTALL=%ProgramFiles%\Microsoft Visual Studio\2022\%%e"
    )
)
if not defined VSINSTALL (
    echo ERROR: Visual Studio 2022 with C++ tools not found.
    exit /b 2
)
call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 2

:: libssh2 translation units (crypto.c #includes the WinCNG backend, so
:: wincng.c/openssl.c/mbedtls.c/libgcrypt.c/os400qc3.c are NOT compiled here)
setlocal enabledelayedexpansion
set "L2SRC="
for %%f in (agent bcrypt_pbkdf channel comp chacha cipher-chachapoly crypt crypto global hostkey keepalive kex knownhost mac misc packet pem poly1305 publickey scp session sftp transport userauth userauth_kbd_packet version) do set "L2SRC=!L2SRC! "%LIBSSH2%\src\%%f.c""

pushd "%WORK%"
:: Debug CRT (/MDd + _DEBUG) so the harness reports leaked blocks with
:: file/line at exit, matching the plugin's Debug build (feature 051).
:: /RTC must stay OFF for the libssh2 sources - see libssh2\readme.txt.
cl /nologo /W3 /MDd /Od /Zi /D_DEBUG /D_CRT_SECURE_NO_WARNINGS /DWIN32 /D_WINDOWS ^
   /DLIBSSH2_WINCNG /DLIBSSH2_ECDSA_WINCNG /DLIBSSH2_API= ^
   /I"%LIBSSH2%\include" /I"%LIBSSH2%\src" ^
   "%HERE%key_auth.c" !L2SRC! ^
   /Fe:key_auth.exe /link ws2_32.lib bcrypt.lib crypt32.lib user32.lib advapi32.lib
set "CLEXIT=%errorlevel%"
if %CLEXIT% neq 0 ( popd & echo COMPILE FAILED & exit /b 2 )

:: absolute path: some environments set NoDefaultCurrentDirectoryInExePath
"%WORK%\key_auth.exe" %* > "%WORK%\out.txt" 2>&1
set "RUNEXIT=%errorlevel%"
type "%WORK%\out.txt"
:: The CRT dumps leaked blocks after main returns, so the exit code cannot
:: carry that verdict - fail the run here instead. A leaked block per session
:: is what surfaces as the Debug build's "Detected memory leaks!" popup.
findstr /c:"Detected memory leaks!" "%WORK%\out.txt" >nul
if not errorlevel 1 (
    echo FAIL leak-check the CRT reported leaked blocks - see the dump above
    set "RUNEXIT=1"
)
popd

rmdir /s /q "%WORK%" 2>nul
exit /b %RUNEXIT%
