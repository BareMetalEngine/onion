@ECHO OFF

rm bin\onion.exe

mkdir .build
mkdir .build\windows
pushd .build\windows

cmake ..\..
cmake --build . --config Release

popd

bin\onion.exe autoRelease