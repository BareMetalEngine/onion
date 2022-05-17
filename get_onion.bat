@ECHO OFF

SET RUN_DIR=%~dp0
SET ONION_FILE=%RUN_DIR%onion.exe

PUSHD %RUN_DIR%
IF EXIST "%ONION_FILE%" (
	ECHO Updating onion.exe...
	curl --silent -z onion.exe -L -O https://github.com/BareMetalEngine/onion_tool/releases/latest/download/onion.exe
) else (
	ECHO Fetching onion.exe...
	curl --silent -L -O https://github.com/BareMetalEngine/onion_tool/releases/latest/download/onion.exe
)
POPD
