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

REM PulseReflect --clang C:/msys64/mingw64/lib/clang/21 --cpp C:/msys64/mingw64/include/c++/15.2.0 --dir ..\..\src\engine\objects\components\misc --dir ..\..\src\engine\objects\components\rendering --dir ..\..\src\engine\objects\components\physics --dir ..\..\src\engine\objects\components\audio -I "..\..\src;..\..\submodules\;..\..\submodules\json\single_include;..\..\submodules\jolt;..\..\submodules\glm;..\..\submodules\freetype\include;C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows\api\core\inc"
REM PulseReflect --clang C:/msys64/mingw64/lib/clang/21 --cpp C:/msys64/mingw64/include/c++/15.2.0 -f ..\..\src\game\character.hpp -I "..\..\src;..\..\submodules\;..\..\submodules\json\single_include;..\..\submodules\jolt;..\..\submodules\glm;..\..\submodules\freetype\include;C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows\api\core\inc"
cd ..

REM Build the project
cmake --build . -j 8
IF %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed.
    exit /b %ERRORLEVEL%
)

REM Run the editor only if build succeeded
echo [INFO] Build succeeded. Starting editor...
start "" ./PulseEditor.exe --game libGameModule.dll --project ..\\test_project\\test_project.json --api opengl