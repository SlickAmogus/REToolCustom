@setlocal enableextensions
@pushd %~dp0
for %%s in (*.dds) do .\REtool.exe -keepBC7type "%%s"
@popd
@pause