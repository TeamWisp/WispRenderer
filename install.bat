@Echo off

REM ##### COLOR SUPPORT #####
SETLOCAL EnableDelayedExpansion
for /F "tokens=1,2 delims=#" %%a in ('"prompt #$H#$E# & echo on & for %%b in (1) do     rem"') do (
  set "DEL=%%a"
)
SET aqua=03
SET light_green=0a
SET red=04
SET title_color=0b
SET header_color=%aqua%
REM ##### COLOR SUPPORT #####

REM ##### LOG NAMES #####
SET log_prefix="cmd_log_"
SET log_suffix=".txt"
REM ##### LOG NAMES #####

REM ##### local vars #####
set is_remote=0
set enable_unit_test=0
set windows_sdk_version=0

REM if left blank it stays in the root folder of the .bat file
set workspace_path=""


REM ##### MAIN #####

call :colorEcho %title_color% "==================================="
call :colorEcho %title_color% "==         Wisp Installer        =="
call :colorEcho %title_color% "==================================="

rem ##### argument handeling ######
if "%1" == "-remote" ( 
  set is_remote=1
) 
if "%1" == "-help" (
  echo This install.bat is use to complete the setup of the ray tracing git repository.
  echo It will donwload all dependencies and use CMake to generate a MS Visual Studio project.
  echo Options:
  echo  -remote [s]
  echo          Can be used to make this script run on build servers. It no longer needs any user input
  echo          remote accepts one argument a directory path to supply your working directory.
  echo          leave this argument blank to use the default directory.
  goto :eof
)

rem ##### pre install settings #####
if "%is_remote%" == "1" ( 
  set workspace_path="%~df2"  
  set enable_unit_test=1
) else (
  rem echo Do you want unit tests enabled? [Y/N]
  choice /c yn /m "Do you want unit tests enabled?"
  if errorlevel 2 (
    echo no unit tests will be generated
  ) else (
    set enable_unit_test=1
        echo unit tests will be generated
  )
)

FOR /F "delims=" %%i IN ('dir "C:\Program Files (x86)\Windows Kits\10\Include" /b /ad-h /t:c /o-d') DO (
    SET windows_sdk_version=%%i
    GOTO :found
)
call :colorEcho %red% No Windows SDK found in location: C:\Program Files (x86)\Windows Kits\10\Include
EXIT 1
:found
echo Latest installed Windows SDK: %windows_sdk_version%
echo Windows SDK required: 10.0.17763.0 or newer

rem ##### install #####
call :downloadDeps
call :genVS15Win64 
call :genVS15Win32 

call :colorEcho %light_green% "Installation Finished!"
if "%is_remote%" == "1" ( 
  goto :eof
) else (
  pause
)
EXIT
REM ##### MAIN #####

REM ##### DOWNLOAD DEPS #####
:downloadDeps
call :colorEcho %header_color% "#### Downloading Dependencies ####"
cd "%workspace_path%"
git submodule init
git submodule update 
EXIT /B 0
REM ##### DOWNLOAD DEPS #####

REM ##### GEN PROJECTS #####
:genVS15Win64
call :colorEcho %header_color% "#### Generating Visual Studio 15 2017 Win64 Project. ####"
cd "%workspace_path%"
echo current path: "%cd%"
mkdir build_vs2017_win64
cd build_vs2017_win64
if "%ENABLE_UNIT_TEST%" == "1" (
  echo cmake -DCMAKE_SYSTEM_VERSION=%windows_sdk_version% -G "Visual Studio 15 2017" -DENABLE_UNIT_TEST:BOOL=ON -A x64 ..
  cmake -DCMAKE_SYSTEM_VERSION=%windows_sdk_version% -G "Visual Studio 15 2017" -DENABLE_UNIT_TEST:BOOL=ON -A x64 ..
) else (
  echo cmake -DCMAKE_SYSTEM_VERSION=%windows_sdk_version% -G "Visual Studio 15 2017" -A x64 ..
  cmake -DCMAKE_SYSTEM_VERSION=%windows_sdk_version% -G "Visual Studio 15 2017" -A x64 ..
)
if errorlevel 1 call :colorecho %red% "CMake finished with errors"
cd ..
EXIT /B 0

:genVS15Win32
call :colorEcho %header_color% "#### Generating Visual Studio 15 2017 Win32 Project. ####"
cd "%workspace_path%"
echo current path: "%cd%" 
mkdir build_vs2017_win32
cd build_vs2017_win32
if "%ENABLE_UNIT_TEST%" == "1" (
  cmake -DCMAKE_SYSTEM_VERSION=%windows_sdk_version% -G "Visual Studio 15 2017" -DENABLE_UNIT_TEST:BOOL=ON -A Win32 ..
) else (
  cmake -DCMAKE_SYSTEM_VERSION=%windows_sdk_version% -G "Visual Studio 15 2017" -A Win32 ..
)
if errorlevel 1 call :colorecho %red% "CMake finished with errors"
cd ..
EXIT /B 0
REM ##### GEN CMAKE PROJECTS #####

REM ##### COLOR SUPPORT #####
:colorEcho
<nul set /p ".=%DEL%" > "%~2"
findstr /v /a:%1 /R "^$" "%~2" nul
del "%~2" > nul 2>&1i
echo.
EXIT /B 0
REM ##### COLOR SUPPORT #####
