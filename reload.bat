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


REM ##### MAIN #####

call :colorEcho %title_color% "==================================="
call :colorEcho %title_color% "==         Wisp Reloader         =="
call :colorEcho %title_color% "==================================="

rem ##### install #####
call :reloadVS15Win64 
call :reloadVS15Win32 

call :colorEcho %light_green% "Reload Finished!"
if "%is_remote%" == "1" ( 
  goto :eof
) else (
  pause
)
EXIT
REM ##### MAIN #####

REM ##### GEN PROJECTS #####
:reloadVS15Win64
call :colorEcho %header_color% "#### Reloading Visual Studio 15 2017 Win64 Project. ####"
cd build_vs2017_win64
cmake .
if errorlevel 1 call :colorecho %red% "CMake finished with errors"
cd ..
EXIT /B 0

:reloadVS15Win32
call :colorEcho %header_color% "#### Reloading Visual Studio 15 2017 Win32 Project. ####"
cd build_vs2017_win32
cmake .
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
