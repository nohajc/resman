echo @off

IF "%APPVEYOR_REPO_TAG%"=="true" (
   7z a resman-%APPVEYOR_REPO_TAG_NAME%-win-amd64.zip %APPVEYOR_BUILD_FOLDER%\rescomp\x64\Release\rescomp.exe include\resman.h
)
