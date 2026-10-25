@echo off
rem Builds and runs re\hands_build_test.cpp, the standalone test of hands_build.cpp's merged guns
rem (accept.ps1's cm-merge-unit), with the PC's MSVC: Visual Studio 2022 or its Build Tools, found
rem with vswhere as build-win.cmd does. Only hands_build.cpp, glm and the standard library go in;
rem it is no part of the app. Usage: hands_build_test.cmd [output directory]
rem (default build-win\vrtest\cm-merge-unit). Exit 0 when every case passes.
setlocal
set ROOT=%~dp0..\..
set OUT=%~1
if "%OUT%"=="" set OUT=%ROOT%\build-win\vrtest\cm-merge-unit
set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%i"
if not defined VSROOT (
	echo Visual Studio with the C++ tools was not found.
	exit /b 1
)
set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
if not exist "%OUT%" mkdir "%OUT%" || exit /b 1
pushd "%OUT%" || exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /W3 /I"%ROOT%\core\rend\vr" /I"%ROOT%\core\deps\glm" /Fehands_build_test.exe "%~dp0hands_build_test.cpp" "%ROOT%\core\rend\vr\hands_build.cpp" >build.log 2>&1
if errorlevel 1 (
	type build.log
	echo hands_build_test: the build failed
	popd
	exit /b 1
)
"%OUT%\hands_build_test.exe"
set CODE=%ERRORLEVEL%
popd
exit /b %CODE%
