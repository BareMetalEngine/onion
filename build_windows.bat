@ECHO OFF

rm bin\onion.exe

mkdir .build
mkdir .build\windows
pushd .build\windows

cmake ..\..
cmake --build . --config Release

popd

copy .build\bin\Release\onion.exe bin\

git config --global user.email "rexdexpl@gmail.com"
git config --global user.name "RexDex (CI)"

bin\onion.exe autoRelease