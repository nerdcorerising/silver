@echo off
setlocal

if not defined BuildType (
    set BuildType=Debug
)

echo   BuildType    : %BuildType%

echo.
echo   Building

set BinDir=%~dp0\bin
set ObjDir=%BinDir%\obj

if not exist %BinDir% (
    mkdir %BinDir%
)
if not exist %ObjDir% (
    mkdir %ObjDir%
)

echo Restoring vcpkg packages
vcpkg install --triplet x64-windows

pushd %ObjDir%

cmake -G Ninja ..\..\ -DCMAKE_BUILD_TYPE=%BuildType% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake

ninja

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
copy /y src\compiler\framework\* %FrameworkDir%\
copy /y src\test\programs\* %ProgramsDir%\
copy /y src\test\*.py %BinDir%\
