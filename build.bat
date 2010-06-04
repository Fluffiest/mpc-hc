@ECHO OFF
SETLOCAL

IF /I "%1" == "help" (
TITLE build.bat %1
ECHO.
ECHO:Usage:
ECHO: "build.bat [clean|build|rebuild] [null|x86|x64] [null|Main|Resource] [Debug]"
ECHO:Executing "build.bat" will use the defaults: "build.bat build null null"
ECHO:Examples:
ECHO:build.bat build x86 Resource     -Will build the x86 resources only
ECHO:build.bat build null Resource    -Will build both x86 and x64 resources only
ECHO:build.bat build x86              -Will build x86 Main exe and the resources
ECHO:build.bat build x86 null Debug   -Will build x86 Main Debug exe and resources
ECHO.
ECHO:"null" can be replaced with anything, e.g. "all": build.bat build x86 all Debug
ECHO.
ECHO:NOTE: Debug only applies to Main project [mpc-hc.sln]
ECHO.
ENDLOCAL
EXIT /B
)

REM pre-build checks
IF "%VS90COMNTOOLS%" == "" GOTO :MissingVar
IF "%MINGW32%" == "" GOTO :MissingVar
IF "%MINGW64%" == "" GOTO :MissingVar

REM Detect if we are running on 64bit WIN and use Wow6432Node, and set the path
REM of Inno Setup accordingly
IF "%PROGRAMFILES(x86)%zzz"=="zzz" (SET "U_=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"
) ELSE (
SET "U_=HKLM\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall"
)

SET "I_=Inno Setup"
SET "A_=%I_% 5"
FOR /f "delims=" %%a IN (
	'REG QUERY "%U_%\%A_%_is1" /v "%I_%: App Path"2^>Nul^|FIND "REG_"') DO (
	SET "InnoSetupPath=%%a"&CALL :SubIS %%InnoSetupPath:*Z=%%)

GOTO :NoVarMissing

:MissingVar
COLOR 0C
Title Compiling MPC-HC [ERROR]
ECHO: "Not all build dependencies found. To build MPC-HC you need:"
ECHO: "* Visual Studio 2008 (SP1) installed"
ECHO: "* MinGW 32 bit build environment with MSYS pointed to in MINGW32 env var"
ECHO: "* MinGW 64 bit build environment with MSYS pointed to in MINGW64 env var"
ECHO. && ECHO.
ECHO:Press any key to exit...
PAUSE >NUL
ENDLOCAL
EXIT /B

:NoVarMissing
REM set up variables
Title Compiling MPC-HC...
SET start_time=%date%-%time%

IF "%1" == "" (SET BUILDTYPE=/Build) ELSE (SET BUILDTYPE=/%1)
SET ORIGPATH="%CD%"

REM FIXME: Does this work for x64 builds??
REM we do have a good alternative vcvarsall.bat x86 | x64
REM Default location: "C:\Program Files\Microsoft Visual Studio 9\VC\Vcvarsall.bat"
CALL "%VS90COMNTOOLS%vsvars32.bat"
CD %ORIGPATH%

SET BUILD_APP=devenv /nologo

REM Debug build only applies to Main (mpc-hc.sln)
IF /I "%4" == "Debug" (SET BUILDCONFIG=Debug) ELSE (SET BUILDCONFIG=Release)

REM Do we want to build x86, x64 or both?
IF /I "%2" == "x64" GOTO :skip32
SET COPY_TO_DIR=bin\mpc-hc_x86
SET Platform=Win32
CALL :Sub_build_internal %*

:skip32
IF /I "%2" == "x86" GOTO :END
SET COPY_TO_DIR=bin\mpc-hc_x64
SET Platform=x64
CALL :Sub_build_internal %*
GOTO :END

:EndWithError
Title Compiling MPC-HC [ERROR]
ECHO. && ECHO.
ECHO: **ERROR: Build failed and aborted!**
PAUSE
ENDLOCAL
EXIT /B

:END
Title Compiling MPC-HC [FINISHED]
ECHO. && ECHO.
ECHO:MPC-HC's compilation started on %start_time%
ECHO:and completed on %date%-%time%
ECHO.
ENDLOCAL
EXIT /B


:Sub_build_internal
IF /I "%3"=="Resource" GOTO :skipMain
%BUILD_APP% mpc-hc.sln %BUILDTYPE% "%BUILDCONFIG%|%Platform%"
IF %ERRORLEVEL% NEQ 0 GOTO :EndWithError

:skipMain
IF /I "%3"=="Main" GOTO :skipResource
%BUILD_APP% mpciconlib.sln %BUILDTYPE% "Release|%Platform%"
IF %ERRORLEVEL% NEQ 0 GOTO :EndWithError

DEL/f/a "%COPY_TO_DIR%\mpciconlib.exp" "%COPY_TO_DIR%\mpciconlib.lib" >NUL 2>&1

FOR %%A IN ("Belarusian" "Catalan" "Chinese simplified" "Chinese traditional" 
"Czech" "Dutch" "French" "German" "Hungarian" "Italian" "Korean" "Polish" 
"Portuguese" "Russian" "Slovak" "Spanish" "Swedish" "Turkish" "Ukrainian"
) DO (
CALL :SubMPCRES %%A
)

:skipResource
IF /I "%1" == "Clean" EXIT /B
IF /I "%3" == "Resource" EXIT /B
IF /I "%3" == "Main" EXIT /B
IF /I "%4" == "Debug" EXIT /B

IF NOT EXIST %COPY_TO_DIR% MKDIR %COPY_TO_DIR%
XCOPY "src\apps\mplayerc\AUTHORS" ".\%COPY_TO_DIR%\" /Y /V
XCOPY "src\apps\mplayerc\ChangeLog" ".\%COPY_TO_DIR%\" /Y /V
XCOPY "COPYING" ".\%COPY_TO_DIR%\" /Y /V

IF /I "%Platform%" == "x64" GOTO :skipx86installer
IF DEFINED InnoSetupPath (
"%InnoSetupPath%\iscc.exe" /Q "distrib\mpc-hc_setup.iss"
IF %ERRORLEVEL% NEQ 0 GOTO :EndWithError
) ELSE (
GOTO :END
)
GOTO :EOF

:skipx86installer
IF /I "%Platform%" == "Win32" GOTO :END
IF DEFINED InnoSetupPath (
"%InnoSetupPath%\iscc.exe" /Q "distrib\mpc-hc_setup.iss" /DBuildx64=True
IF %ERRORLEVEL% NEQ 0 GOTO :EndWithError
) ELSE (
GOTO :END
)
GOTO :EOF

:SubMPCRES
%BUILD_APP% mpcresources.sln %BUILDTYPE% "Release %~1|%Platform%"
IF %ERRORLEVEL% NEQ 0 GOTO :EndWithError
GOTO :EOF

:SubIS
SET InnoSetupPath=%*
GOTO :EOF
