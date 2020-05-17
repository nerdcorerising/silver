@echo off
setlocal

if not defined BuildOS (
    set BuildOS=Windows
)

if not defined BuildArch (
    set BuildArch=x64
)

if not defined BuildType (
    set BuildType=Debug
)

set VS_COM_CMD_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat"

if not defined VS_CMD_PATH (
    if exist %VS_COM_CMD_PATH% (
        set VS_CMD_PATH=%VS_COM_CMD_PATH%
    ) else (
        echo No VS developer command prompt detected!
        goto :EOF
    )
)

echo   BuildOS      : %BuildOS%
echo   BuildArch    : %BuildArch%
echo   BuildType    : %BuildType%
echo   VS PATH      : %VS_CMD_PATH%

echo.
echo   Building

set ObjDir=%~dp0\obj
set BinDir=%~dp0\bin

if not exist %ObjDir% (
    mkdir %ObjDir%
)

pushd %ObjDir%

cmake -G "Visual Studio 16 2019" ..\ -DCMAKE_BUILD_TYPE=Debug

echo Calling VS Developer Command Prompt to build
call %VS_CMD_PATH%

msbuild -v:m silver.sln

popd

echo.
echo.
echo.
echo Done building

if not exist %BinDir% (
    mkdir %BinDir%
)

set ProgramsDir=%BinDir%\programs
if not exist %ProgramsDir% (
    mkdir %ProgramsDir%
)

echo Copying binaries
copy /y %ObjDir%\src\compiler\Debug\silver.* %BinDir%\
copy /y %ObjDir%\src\test\Debug\test.* %BinDir%\
copy /y src\test\programs\* %ProgramsDir%\
copy /y src\test\*.py %BinDir%\
