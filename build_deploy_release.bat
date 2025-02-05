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

:: Configure with CMake for a Release build
echo Configuring with CMake for Release...
cmake .. -DCMAKE_BUILD_TYPE=Release

:: Build the project in Release mode
echo Building the project in Release mode...
cmake --build . --config Release

:: Check if the build was successful
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)

:: Optional: Manually run windeployqt in Release mode if needed.
:: Only do this if you want to deploy again outside the built-in CMake steps.
::
:: echo Running windeployqt for Release...
:: "E:\QT\6.8.2\msvc2022_64\bin\windeployqt.exe" --release ^
::     --no-translations ^
::     --no-system-d3d-compiler ^
::     "Release\codebase_processor.exe"

echo Build completed successfully!

:: If you're using the default Visual Studio generator, the final exe is likely in build\Release.
echo You can find the executable in build\Release\ (if using Visual Studio).
echo If using Ninja or MinGW, the exe will be directly in build\.

pause
