@echo off

set "defines= /DGREMLIN_DEBUG"
set "compile_options= /diagnostics:column /Gm- /FC /W4 /wd4100 /wd4201 /Od /Zo /Zf /Z7 /Oi /std:c++17 /nologo /GR- /MP"
set "link_options= /INCREMENTAL:NO /opt:ref /out:gremlin.exe"

pushd .\build

cl %compile_options% %defines% ./../src/main.cpp /link %link_options%

popd