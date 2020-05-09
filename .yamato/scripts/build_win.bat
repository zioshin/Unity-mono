@echo off
git submodule update --init --recursive

echo %UNITY_THISISABUILDMACHINE%

perl external/buildscripts/build_runtime_win64.pl
if NOT %errorlevel% == 0 (
 echo "mono build script failed"
 EXIT /B %errorlevel%
)
echo "mono build script ran successfully"

md incomingbuilds\win64
xcopy /s /e /h /y builds\* incomingbuilds\win64
