@echo off
REM Configure the build
cmake -S . -B build -G "MinGW Makefiles"
IF %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed.
    exit /b %ERRORLEVEL%
)

REM Build the project
cd build
cmake --build .
IF %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed.
    exit /b %ERRORLEVEL%
)

REM Run the editor only if build succeeded
echo [INFO] Build succeeded. Starting editor...
start "" ./PulseEditor.exe --editor libEditorModule.dll --project ..\\PROJECT\\test_project.json 