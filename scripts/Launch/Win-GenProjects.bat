@echo off
rem %~dp0 is scripts\Launch\, so go up two levels to reach the repo root
pushd %~dp0\..\..\
call vendor\premake\bin\premake5.exe vs2022
popd
if /I not "%1"=="nopause" PAUSE