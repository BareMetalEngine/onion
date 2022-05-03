@ECHO OFF

rm bin\onion.exe

mkdir .build
mkdir .build\windows
pushd .build\windows

cmake ..\..
cmake --build . --config Release

popd

copy .build\bin\Release\onion.exe bin\
bin\onion.exe autoRelease