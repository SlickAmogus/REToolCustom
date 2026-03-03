@setlocal enableextensions
@pushd %~dp0
.\REtool.exe -version 4 1 -c %1
@popd
@pause