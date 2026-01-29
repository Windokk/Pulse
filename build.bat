@echo off
REM Configure the build
cmake -S . -B build -G "MinGW Makefiles"
IF %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed.
    exit /b %ERRORLEVEL%
)

cd build

REM Build reflection
cd tools

PulseReflect --clang C:/msys64/mingw64/lib/clang/21 --cpp C:/msys64/mingw64/include/c++/15.2.0 --dir ..\..\src\engine\ecs\components\misc --dir ..\..\src\engine\ecs\components\rendering --dir ..\..\src\engine\ecs\components\physics --dir ..\..\src\engine\ecs\components\audio -I "..\..\src;..\..\submodules\;..\..\submodules\json\single_include;..\..\submodules\jolt;..\..\submodules\glm;..\..\submodules\freetype\include;C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows\api\core\inc"
cd ..

REM Build the project
cmake --build .
IF %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed.
    exit /b %ERRORLEVEL%
)

REM Run the editor only if build succeeded
echo [INFO] Build succeeded. Starting editor...
start "" ./PulseEditor.exe --editor libEditorModule.dll --project ..\\test_project\\test_project.json