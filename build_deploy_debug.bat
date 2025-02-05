@echo off
setlocal enabledelayedexpansion

:: Check if VS Developer Command Prompt variables are set
if not defined VSINSTALLDIR (
    echo Setting up Visual Studio environment...
    call "E:\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
)

:: Create and enter build directory
if not exist build mkdir build
cd build

:: Configure with CMake
echo Configuring with CMake...
cmake .. -DCMAKE_BUILD_TYPE=Debug

:: Build the project
echo Building the project...
cmake --build . --config Debug

:: Check if build was successful
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build completed successfully!
echo You can find the executable in build\Debug\
echo Don't forget to run the program from that directory!

pause