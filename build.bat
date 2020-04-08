@echo off

set "defines= /DGREMLIN_PLATFORM_WINDOWS /DGREMLIN_DEBUG"
set "compile_options= /diagnostics:column /Gm- /FC /W4 /wd4100 /wd4201 /wd4101 /wd4204 /Od /Zo /Zf /Z7 /Oi /std:c++17 /nologo /GR- /MP"
set "link_options= /INCREMENTAL:NO /opt:ref /out:gremlin.exe"

if NOT EXIST .\build mkdir build
pushd .\build

cl %compile_options% %defines% .\..\src\main.c /link %link_options%

popd
