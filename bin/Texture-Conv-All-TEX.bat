@setlocal enableextensions
@pushd %~dp0
for %%s in (*.tex.*) do .\REtool.exe "%%s"
@popd
@pause