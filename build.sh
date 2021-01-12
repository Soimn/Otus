#!/bin/bash

defines="-DOTUS_DEBUG -DOTUS_DLL_EXPORTS"
compile_options="-Wall -Wextra -Wno-logical-op-parentheses -Wno-unused-parameter -fPIC -fvisibility=hidden -fvisibility-inlines-hidden"

if [ ! -d ./build ]; then
	mkdir build
fi

pushd build > /dev/null

clang $compile_options $defines -shared -o otus.so ./../src/compiler.c

clang -o otus otus.so ./../src/meta.c

popd > /dev/null