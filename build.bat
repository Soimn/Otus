@echo off

set "defines= "
set "compile_options= /diagnostics:column /Gm- /FC /W4 /wd4116 /wd4100 /wd4201 /wd4101 /wd4204 /Od /Zo /Zf /Z7 /Oi /std:c++17 /nologo /GR- /MP"
set "link_options= /INCREMENTAL:NO /opt:ref"

if NOT EXIST .\build mkdir build
pushd .\build

cl %compile_options% %defines% .\..\src\compiler.c /link %link_options% /dll /out:otus.dll
cl %compile_options% %defines% .\..\src\main.c /link %link_options% otus.lib /out:otus.exe

popd
