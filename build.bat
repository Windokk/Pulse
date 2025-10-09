cmake -S . -B build -G "MinGW Makefiles"
cd build
cmake --build .
start /wait Epoch.exe --editor
echo %ERRORLEVEL%