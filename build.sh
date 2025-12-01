cmake -S . -B build -G "Unix Makefiles"
cd build
cmake --build .
./Pulse
echo $?