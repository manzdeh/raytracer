#!/bin/bash

scriptpath=$(realpath "$0")

pushd $(dirname "$scriptpath") > /dev/null

echo Compiling DEBUG build...

compiler_flags="-std=c++20 -Werror -Wall -Wunused-parameter \
    -Wno-gnu-zero-variadic-macro-arguments -Wno-gnu-anonymous-struct -Wno-nested-anon-types \
    -O0 -g -pedantic \
    -m64 -masm=intel -mrdrnd"
defines="-D_POSIX_C_SOURCE=200809L -DAE_PLATFORM_LINUX"
libs="-lstdc++ -lc -lm"

translation_units=$(find . -name "*.cpp" ! -name "*win32.cpp")

clang++ $compiler_flags \
    $defines \
    -I src/ \
    $translation_units \
    $libs \
    -o raytracer

popd > /dev/null
