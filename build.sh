export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)
cmake -S . -B build -G "Unix Makefiles"
cd build
cmake --build .
#./PulseEditor --editor libEditorModule.so --project ../test_project/test_project.json
#echo $?