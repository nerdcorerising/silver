@echo off
setlocal

if not defined BuildType (
    set BuildType=Debug
)

echo   BuildType    : %BuildType%

echo.
echo   Building

set RootDir=%~dp0
set BinDir=%RootDir%bin
set ObjDir=%BinDir%\obj

REM Find and set up Visual Studio environment to ensure MSVC is used
where cl >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Setting up Visual Studio environment...
    for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VSInstallDir=%%i
    if defined VSInstallDir (
        call "%VSInstallDir%\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else (
        echo ERROR: Could not find Visual Studio with C++ tools installed.
        echo Please run this script from a Visual Studio Developer Command Prompt.
        exit /b 1
    )
)

if not exist %BinDir% (
    mkdir %BinDir%
)
if not exist %ObjDir% (
    mkdir %ObjDir%
)

REM LLVM_DIR should point to your LLVM installation's lib/cmake/llvm directory
REM Download from: https://github.com/llvm/llvm-project/releases
if not defined LLVM_DIR set LLVM_DIR=C:\Program Files\LLVM\lib\cmake\llvm
if not exist "%LLVM_DIR%" (
    echo ERROR: LLVM not found at "%LLVM_DIR%"
    echo Please download LLVM from https://github.com/llvm/llvm-project/releases
    echo and either install to the default location or set LLVM_DIR.
    exit /b 1
)
echo   LLVM_DIR     : %LLVM_DIR%

pushd %ObjDir%

cmake -G Ninja "%RootDir%." -DCMAKE_BUILD_TYPE=%BuildType% -DLLVM_DIR="%LLVM_DIR%"
if %ERRORLEVEL% neq 0 (
    popd
    echo ERROR: cmake configuration failed
    exit /b %ERRORLEVEL%
)

ninja
if %ERRORLEVEL% neq 0 (
    popd
    echo ERROR: ninja build failed
    exit /b %ERRORLEVEL%
)

popd

echo.
echo.
echo.
echo Done building

set ProgramsDir=%BinDir%\programs
if not exist %ProgramsDir% (
    mkdir %ProgramsDir%
)
set FrameworkDir=%BinDir%\framework
if not exist %FrameworkDir% (
    mkdir %FrameworkDir%
)

echo Copying binaries
copy /y %ObjDir%\src\compiler\silver.* %BinDir%\
copy /y %ObjDir%\src\runtime\silver_runtime.lib %BinDir%\
copy /y %ObjDir%\src\test\test_runner.exe %BinDir%\
copy /y src\compiler\framework\* %FrameworkDir%\
copy /y src\test\programs\* %ProgramsDir%\
