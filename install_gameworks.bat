@Echo off

REM ##### COLOR SUPPORT #####
SETLOCAL ENABLEDELAYEDEXPANSION
for /F "tokens=1,2 delims=#" %%a in ('"prompt #$H#$E# & echo on & for %%b in (1) do     rem"') do (
  set "DEL=%%a"
)
SET aqua=03
SET light_green=0a
SET red=04
SET title_color=0b
SET header_color=%aqua%
REM ##### COLOR SUPPORT #####

call :colorEcho 04 "YOU NEED TO BE LOGGED IN WITH A GIT ACCOUNT THAT IS A MEMBER OF THE `GameWorks_EULA_Access` TEAM"

call :colorEcho 03 "Downloading the Ansel and HBAO+ SDK's"

git clone https://github.com/NVIDIAGameWorks/HBAOPlus.git deps/hbao+
git clone https://github.com/NVIDIAGameWorks/AnselSDK.git deps/ansel

call :colorEcho 03 "Installation Finished"

goto :eof

REM ##### COLOR SUPPORT #####
:colorEcho
<nul set /p ".=%DEL%" > "%~2"
findstr /v /a:%1 /R "^$" "%~2" nul
del "%~2" > nul 2>&1i
echo.
EXIT /B 0
REM ##### COLOR SUPPORT #####
