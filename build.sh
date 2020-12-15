#!/bin/bash

defines=""
compile_options="-Wall -Wextra -Wno-logical-op-parentheses -fPIC"
link_options=""

if [ ! -d ./build ]; then
	mkdir build
fi

pushd build > /dev/null

clang $compile_options $defines -shared -o otus.so ./../src/compiler.c

popd > /dev/null