@echo off
set DIR_ATTRIB=

SETLOCAL ENABLEDELAYEDEXPANSION
set START_DIR=%~d0%~p0%~nx0
set START_DIR=!START_DIR:%~0=!
echo %START_DIR%

cd "%~d0%~p0\.."
del src_list.txt
for /f "tokens=4 delims= " %%a in ('svn.exe st -v') do (
echo %%a
set DIR_ATTRIB=%%~aa
set DIR_ATTRIB=!DIR_ATTRIB:~0,1!
if "!DIR_ATTRIB!"=="-" echo %%a>>src_list.txt
)
cd %START_DIR%