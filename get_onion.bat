@ECHO OFF

SET RUN_DIR=%~dp0
SET ONION_FILE=%RUN_DIR%onion.exe

PUSHD %RUN_DIR%
curl curl --silent -z onion.exe -L -O https://github.com/BareMetalEngine/onion_tool/releases/latest/download/onion.exe
POPD