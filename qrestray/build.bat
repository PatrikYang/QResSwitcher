@echo off
:: Build script for QResTray
:: Requires: Visual Studio 2019 or 2022 (or Build Tools)

setlocal

:: Try to find vswhere
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist %VSWHERE% set VSWHERE="%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist %VSWHERE% (
    echo ERROR: vswhere.exe not found. Please install Visual Studio 2019/2022 or Build Tools.
    pause
    exit /b 1
)

:: Get VS install path
for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set VS_PATH=%%i
)

if "%VS_PATH%"=="" (
    echo ERROR: No Visual Studio with C++ tools found.
    pause
    exit /b 1
)

:: Initialize MSVC environment (Win32/x86 target)
call "%VS_PATH%\VC\Auxiliary\Build\vcvars32.bat"

:: Build
echo.
echo Building QResTray...
echo.

msbuild "%~dp0qrestray.vcxproj" /p:Configuration=Release /p:Platform=Win32 /m /nologo

if %ERRORLEVEL% == 0 (
    echo.
    echo Build succeeded!
    echo Output: %~dp0..\bin\Release\qrestray.exe
) else (
    echo.
    echo Build FAILED. See errors above.
)

pause
endlocal
