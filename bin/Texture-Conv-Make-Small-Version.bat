@setlocal enableextensions
@pushd %~dp0
.\REtool.exe -texReduce -texReduceBy 4 %1
@popd
@pause