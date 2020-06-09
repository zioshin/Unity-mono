@echo off
git submodule update --init --recursive

echo %UNITY_THISISABUILDMACHINE%

C:\Cygwin\bin\bash.exe external/buildscripts/build_ios_xwin.sh
if NOT %errorlevel% == 0 (
 echo "mono build script failed"
 EXIT /B %errorlevel%
)
echo "mono build script ran successfully"

md incomingbuilds\iphonexcompiler
xcopy /s /e /h /y builds\* incomingbuilds\iphonexcompiler
