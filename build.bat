@echo off
setlocal

set CMAKE_BIN=D:/qtinstaal/Tools/CMake_64/bin/cmake.exe
set QT_DIR=D:/qtinstaal/6.11.0/mingw_64
set MAKE_BIN=D:/qtinstaal/Tools/mingw1310_64/bin/mingw32-make.exe

if not exist "build\CMakeCache.txt" (
    echo [INFO] Running CMake configure...
    "%CMAKE_BIN%" -B build -G "MinGW Makefiles" ^
        -DCMAKE_PREFIX_PATH=%QT_DIR% ^
        -DCMAKE_BUILD_TYPE=Debug ^
        -DCMAKE_MAKE_PROGRAM=%MAKE_BIN% ^
        -DCMAKE_C_COMPILER=D:/qtinstaal/Tools/mingw1310_64/bin/gcc.exe ^
        -DCMAKE_CXX_COMPILER=D:/qtinstaal/Tools/mingw1310_64/bin/g++.exe
    if errorlevel 1 (
        echo [ERROR] CMake configure failed.
        exit /b 1
    )
)

echo [INFO] Building...
"%CMAKE_BIN%" --build build
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo [OK] Build succeeded. Output: build\DesktopPet.exe
endlocal
