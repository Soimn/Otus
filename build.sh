#!/bin/bash
defines="-D OTUS_DEBUG -D OTUS_DLL_EXPORTS"
compile_options=""

if [ ! -d "./build" ]
	then
		mkdir build
fi
pushd ./build

clang -shared -fPIC $compile_options $defines -o libotus.so ./../src/compiler.c
clang -L. -lotus $compile_options -o otus ./../src/default_meta.c

popd
