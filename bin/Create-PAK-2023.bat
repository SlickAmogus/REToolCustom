@setlocal enableextensions
@pushd %~dp0
.\REtool.exe -c %1
@popd
@pause