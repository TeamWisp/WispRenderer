@Echo Off

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

REM ##### MAIN #####

call :colorEcho %title_color% "==================================="
call :colorEcho %title_color% "           Wisp Installer          "
call :colorEcho %title_color% "==================================="

if "%1" == "-remote" ( 
  cd %~dp2  
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

call :downloadDeps
call :genVS15Win64 
REM >> test.txt 2>&1
call :genVS15Win32 
REM >> test.txt 2>&1

call :colorEcho %light_green% "Installation Finished!"
if "%1" == "-remote" ( 
  goto :eof
) else (
  pause
)
EXIT
REM ##### MAIN #####

REM ##### DOWNLOAD DEPS #####
:downloadDeps
call :colorEcho %header_color% "#### Downloading Dependencies ####"
git submodule init
git submodule update 
EXIT /B 0
REM ##### DOWNLOAD DEPS #####

REM ##### DOWNLOAD DEPS #####
:downloadDepsServer
call :colorEcho %header_color% "## Downloading Dependencies ##"
cd %~dp2
git submodule init
git submodule update 
EXIT /B 0
REM ##### DOWNLOAD DEPS #####

REM ##### GEN PROJECTS #####
:genVS15Win64
call :colorEcho %header_color% "#### Generating Visual Studio 15 2017 Win64 Project. ####"
mkdir build_vs2017_win64
cd build_vs2017_win64
cmake -G "Visual Studio 15 2017" -D ENABLE_UNIT_TEST:BOOL=TRUE -A x64 ..
if errorlevel 1 call :colorecho %red% "CMake finished with errors"
cd ..
EXIT /B 0

:genVS15Win64Server
call :colorEcho %header_color% "## Generating Visual Studio 15 2017 Win64 Project. ##"
cd %~dp2
mkdir build_vs2017_win64
cd build_vs2017_win64
cmake -G "Visual Studio 15 2017" -A x64 ..
if errorlevel 1 (
  call :colorecho %red% "CMake finished with errors"
)
cd ..
EXIT /B 0

:genVS15Win32
call :colorEcho %header_color% "#### Generating Visual Studio 15 2017 Win32 Project. ####"
mkdir build_vs2017_win32
cd build_vs2017_win32
cmake -G "Visual Studio 15 2017" -A Win32 ..
if errorlevel 1 call :colorecho %red% "CMake finished with errors"
cd ..
EXIT /B 0

:genVS15Win32Server
call :colorEcho %header_color% "## Generating Visual Studio 15 2017 Win32 Project. ##"
cd %~dp2
mkdir build_vs2017_win32
cd build_vs2017_win32
cmake -G "Visual Studio 15 2017" -A Win32 ..
if errorlevel 1 (
  call :colorecho %red% "CMake finished with errors"
)
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
