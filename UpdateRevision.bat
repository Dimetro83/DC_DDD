@echo off
call svn update

rem for /f "tokens=2 delims= " %%a in ('svn.exe st -q') do (
rem echo "%%a" changed
rem echo Commit changes before making distrib!
rem pause
rem del revision.h
rem exit /b 1
rem goto :end
rem )

for /F "tokens=1,2 delims=: " %%a in ('svn info') do call :InfoProc "%%a" "%%b"
goto :end

:InfoProc
if %1=="Revision" call :WriteRevision %2
goto :end

:WriteRevision
echo #ifndef FLY_REVISION_H > revision.h
echo #define REVISION %~1 >> revision.h
echo #define REVISION_STR %1 >> revision.h
echo #define L_REVISION_STR L%1 >> revision.h
echo #endif >> revision.h
goto :end

:end