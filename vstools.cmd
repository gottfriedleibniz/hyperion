@if defined TRACEON (@echo on) else (@echo off)

  REM  If this batch file works, then it was written by Fish.
  REM  If it doesn't then I don't know who the heck wrote it.

  goto :BEGIN

::-----------------------------------------------------------------------------
::                            VSTOOLS.CMD
::-----------------------------------------------------------------------------
:HELP

  echo.
  echo     NAME
  echo.
  echo         %~n0   --   Initialize Visual Studio build environment
  echo.
  echo     SYNOPSIS
  echo.
  echo         %~nx0    { detect ^| list ^| [target] }
  echo.
  echo     ARGUMENTS
  echo.
  echo         detect      Detects a Visual Studio installation and
  echo                     defines variables identifying which one.
  echo                     Refer to the below NOTES section.
  echo.
  echo         list        List all target architectures available for
  echo                     build environment initialization.
  echo.
  echo         [target]    Architecture used to initialize the Visual
  echo                     Studio build environment. Cross compilation
  echo                     adheres to a HOST[_TARGET] format.
  echo.
  echo     OPTIONS
  echo.
  echo         (NONE)
  echo.
  echo     NOTES
  echo.
  echo         Some flavor of Visual Studio must obviously be installed.
  echo         Supported versions are Visual Studio 2005 through 2026.
  echo.
  echo         The 'detect' call detects which version of Visual Studio
  echo         is installed and defines certain variables identifying
  echo         which version it was that was detected, but otherwise
  echo         does NOT initialize the build environment. It defines
  echo         some helpful (informative) variables ONLY.
  echo.
  echo         The 'target' call is the one that initializes the Visual
  echo         Studio build environment for a given architecture. Note
  echo         that a 'target' call will set the same variables as a
  echo         'detect' call (i.e. an internal "detect" call is always
  echo         performed).
  echo.
  echo         The variables which the 'detect' call returns are:
  echo.
  echo              vsname          Visual Studio's "short name"
  echo                              (e.g. "vs2019", "vs2008", etc)
  echo.
  echo              vsver           Visual Studio's numeric version
  echo                              (e.g. "150", "90", etc)
  echo.
  echo              vshost          The detected host architecture
  echo                              ("x86", "amd64", or "arm64")
  echo.
  echo              vstarget        The requested target architecture:
  echo                              "x86" if '32', "amd64" if '64', see
  echo                              the list and [arch] ARGUMENTS.
  echo.
  echo              vs2026..vs2005  The Visual Studio version numbers
  echo                              for each supported version of it
  echo                              (vs2019=160, vs2017=150, etc)
  echo.
  echo     EXIT STATUS
  echo.
  echo         0          Success
  echo         1          Error
  echo.
  echo     AUTHOR
  echo.
  echo         "Fish"  (David B. Trout)
  echo.
  echo     VERSION
  echo.
  echo         3.4     (Feburary 17, 2026)
:END_HELP
  endlocal
  exit /b 1


::-----------------------------------------------------------------------------
::                               BEGIN
::-----------------------------------------------------------------------------
:BEGIN

  :: Initialize the variables we'll be passing back to the caller.
  :: We do this BEFORE our "setlocal" to update THEIR variable pool.

  set "vs2005=80"
  set "vs2008=90"
  set "vs2010=100"
  set "vs2012=110"
  set "vs2013=120"
  set "vs2015=140"
  set "vs2017=150"
  set "vs2019=160"
  set "vs2022=170"
  set "vs2026=180"

  set "VSNAMES=vs2026 vs2022 vs2019 vs2017 vs2015 vs2013 vs2012 vs2010 vs2008 vs2005"

  set "vsname="
  set "vsver="
  set "vshost="
  set "vstarget="

  setlocal enabledelayedexpansion

  set "TRACE=if defined DEBUG echo"
  set "return=goto :EOF"

  :: Validate passed arguments...

  if /i "%~1" == ""        goto :HELP
  if /i "%~1" == "?"       goto :HELP
  if /i "%~1" == "/?"      goto :HELP
  if /i "%~1" == "-?"      goto :HELP
  if /i "%~1" == "--help"  goto :HELP
  if /i not "%~2" == ""    goto :HELP

  :: Define target architecture...

  call :detect_vstudio "%~1"

  if %rc% NEQ 0 (
    endlocal
    exit /b 1
  )

  :: Go set the build environment if that's what they want

  if /i "%~1" == "list" (
    call :print_vcvars_files
    goto :END_HELP
  ) else if /i not "%~1" == "detect" (
    goto :SET_VSTUDIO_ENV
  )

  :: Otherwise all they're interested in are the variables

  endlocal                          ^
    && set "vsname=%vsname%"        ^
    && set "vsver=%vsver%"          ^
    && set "vshost=%vshost%"        ^
    && set "vstarget=%vstarget%"

  exit /b 0


::-----------------------------------------------------------------------------
::                          SET_VSTUDIO_ENV
::-----------------------------------------------------------------------------
:SET_VSTUDIO_ENV

  ::--------------------------------------------------------
  ::   Set the target Visual Studio build environment
  ::--------------------------------------------------------
  ::  Note that we must set the build environment OUTSIDE
  ::  the scope of our setlocal/endlocal to ensure that
  ::  whatever build environment gets set ends up being
  ::  passed back to the caller.
  ::--------------------------------------------------------

  endlocal                          ^
    && set "vsname=%vsname%"        ^
    && set "vsver=%vsver%"          ^
    && set "vshost=%vshost%"        ^
    && set "vstarget=%vstarget%"    ^
    && set "VCVARSDIR=%VCVARSDIR%"

  pushd .
  echo.

  set rc=1
  if %vsver% LSS %vs2017% (
    @REM   Visual Studio 2015 or EARLIER

    call "%VCVARSDIR%\vcvarsall.bat"  %vstarget%
    set "rc=%errorlevel%"
  ) else (
    @REM   Visual Studio 2017 or LATER

    @REM Mapped targets for backwards compatibility
    if /i "%vstarget%" == "x86" (
      call "%VCVARSDIR%\vcvars32.bat"
      set "rc=%errorlevel%"
    ) else if /i "%vstarget%" == "amd64" (
      call "%VCVARSDIR%\vcvars64.bat"
      set "rc=%errorlevel%"
    ) else if exist "%VCVARSDIR%\vcvars%vstarget%.bat" (
      call "%VCVARSDIR%\vcvars%vstarget%.bat"
      set "rc=%errorlevel%"
    ) else (
      echo %~nx0: ERROR: "%VCVARSDIR%\vcvars%vstarget%.bat" not found
      echo %~nx0: INFO: Provided target: "%vstarget%"
      call :print_vcvars_files
    )
  )

  @if defined TRACEON (@echo on) else (@echo off)

  echo.
  popd
  exit /b %rc%


::-----------------------------------------------------------------------------
::                          detect_vstudio
::-----------------------------------------------------------------------------
:detect_vstudio

  set "vstarget=%~1"
  set "vsver="
  set "vsname="
  set "VCVARSDIR="
  set "rc=0"

  set "@VSNAMES=%VSNAMES%"

:detect_vstudio_loop

  for /f "tokens=1*" %%a in ("%@VSNAMES%") do (
    call :detect_vstudio_sub "%%a"
    if defined VCVARSDIR (
      goto :detect_vstudio_host
    )
    if "%%b" == "" (
      goto :detect_vstudio_error
    )
    set "@VSNAMES=%%b"
    goto :detect_vstudio_loop
  )

:detect_vstudio_error

  echo %~nx0: ERROR: No supported version of Visual Studio is installed
  set "vstarget="
  set "vsver="
  set "vsname="
  set "VCVARSDIR="
  set "rc=1"
  %return%

:detect_vstudio_sub

  set "vsname=%~1"
  set "vsver=!%vsname%!"

  if "!VS%vsver%COMNTOOLS!" == "" (
    %return%
  )

  :: Remove trailing backslash if present

  set "VSCOMNTOOLS=!VS%vsver%COMNTOOLS!"

  if "%VSCOMNTOOLS:~-1%" == "\" (
    set "VSCOMNTOOLS=%VSCOMNTOOLS:~0,-1%"
  )

  :: Detect accidental typos in VSCOMNTOOLS path

  if not exist "%VSCOMNTOOLS%" (
    echo %~nx0: ERROR: Defined VS%vsver%COMNTOOLS directory does not exist!
    echo %~nx0: INFO: VS%vsver%COMNTOOLS = "%VSCOMNTOOLS%"??
    set "vstarget="
    set "vsver="
    set "vsname="
    set "VCVARSDIR="
    set "rc=1"
    %return%
  )

  :: Define directory where "vcvarsxx.bat" files live

  set "VCVARSDIR=%VSCOMNTOOLS%\..\..\VC"

  if %vsver% GEQ %vs2017% (
    set "VCVARSDIR=%VCVARSDIR%\Auxiliary\Build"
  )

  %return%

:detect_vstudio_host

  if   /i "%PROCESSOR_ARCHITEW6432%" == "x86"   set "vshost=x86"
  if   /i "%PROCESSOR_ARCHITEW6432%" == "AMD64" set "vshost=amd64"
  if   /i "%PROCESSOR_ARCHITEW6432%" == "ARM64" set "vshost=arm64"
  if not defined vshost (
    if /i "%PROCESSOR_ARCHITECTURE%" == "x86"   set "vshost=x86"
    if /i "%PROCESSOR_ARCHITECTURE%" == "AMD64" set "vshost=amd64"
    if /i "%PROCESSOR_ARCHITECTURE%" == "ARM64" set "vshost=arm64"
  )
  %return%

::-----------------------------------------------------------------------------
::                          print_vcvars_archs
::-----------------------------------------------------------------------------
:print_vcvars_files
  ::----------------------------------------------------------
  :: Compile a list of all available Visual Studio host/target
  :: architectures by iterating over all available vcvars*.bat
  :: files in %VCVARSDIR%.
  ::--------------------------------------------------------
  setlocal enabledelayedexpansion

  @REM Mapped targets for backwards compatibility
  set "VSARCHS=x64 | x86"

  for %%F in ("%VCVARSDIR%\vcvars*.bat") do (
    set "filename=%%~nF"
    set "filename=!filename:~6!"
    if /I not "!filename!" == "all" (
      set "VSARCHS=!VSARCHS! ^| !filename!"
    )
  )

  echo %~nx0: INFO: Available targets: !VSARCHS!
  endlocal
  exit /b 0

:print_vcvars_help
  ::--------------------------------------------------------
  :: Compile a list of all available Visual Studio target
  :: architectures by parsing the vcvarsall.bat help text
  :: that lists available host/targets.
  ::--------------------------------------------------------
  :: Instead of checking for 'vcvars*.bat', the logic in
  :: SET_VSTUDIO_ENV could be modified to determine if
  :: vstarget exists in the list generated by this routine.
  ::--------------------------------------------------------
  setlocal enabledelayedexpansion

  @REM Mapped files for backwards compatibility
  set "VSARCHS=32 | 64"

  for /F "tokens=*" %%a in ('
    call "%VCVARSDIR%\vcvarsall.bat" /help ^| findstr /C:"\[arch\]: "
  ') do (
    set "line=%%a"
    set "VSARCHS=!VSARCHS! ^| !line:~8!"
    goto :end_print_vcvars
  )

:end_print_vcvars
  echo %~nx0: INFO: Available targets: !VSARCHS!
  endlocal
  exit /b 0

::-----------------------------------------------------------------------------
