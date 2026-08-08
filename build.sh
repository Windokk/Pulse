#!/bin/bash
# Configure the build
cmake -S . -B build -G "Unix Makefiles"
if [ $? -ne 0 ]; then
    echo "[ERROR] CMake configuration failed."
    exit $?
fi

cd build

# Build reflection
cd tools

# PulseReflect --clang C:/msys64/mingw64/lib/clang/21 --cpp C:/msys64/mingw64/include/c++/15.2.0 --dir ../../src/engine/objects/components/misc --dir ../../src/engine/objects/components/rendering --dir ../../src/engine/objects/components/physics --dir ../../src/engine/objects/components/audio -I "../../src;../../submodules/;../../submodules/json/single_include;../../submodules/jolt;../../submodules/glm;../../submodules/freetype/include;C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows\api\core\inc"
# PulseReflect --clang C:/msys64/mingw64/lib/clang/21 --cpp C:/msys64/mingw64/include/c++/15.2.0 -f ../../src/game/character.hpp -I "../../src;../../submodules/;../../submodules/json/single_include;../../submodules/jolt;../../submodules/glm;../../submodules/freetype/include;C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows\api\core\inc"
cd ..

# Build the project
cmake --build . -j 8
if [ $? -ne 0 ]; then
    echo "[ERROR] Build failed."
    exit $?
fi

# Run the editor only if build succeeded
echo "[INFO] Build succeeded. Starting editor..."
./PulseEditor --game libGameModule.so --project ../test_project/test_project.json --api opengl
