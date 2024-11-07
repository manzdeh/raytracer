#!/bin/bash

scriptpath=$(realpath "$0")

pushd $(dirname "$scriptpath") > /dev/null

compiler_flags="-std=c++20 -Werror -Wall -Wunused-parameter \
    -Wno-gnu-zero-variadic-macro-arguments -Wno-gnu-anonymous-struct -Wno-nested-anon-types \
    -pedantic -m64 -masm=intel -mrdrnd"
defines="-D_POSIX_C_SOURCE=200809L -DAE_PLATFORM_LINUX -DVK_NO_PROTOTYPES"
libs="-lstdc++ -lc -lm -lpthread"

translation_units=$(find . -name "*.cpp" ! -name "*win32.cpp")

if [[ "$*" == *"--release"* ]]
    then
        echo Compiling RELEASE build...
        compiler_flags="$compiler_flags -O2"
        defines="$defines -DAE_RELEASE"
    else
        echo Compiling DEBUG build...
        compiler_flags="$compiler_flags -O0 -g"
        defines="$defines -DAE_DEBUG"
fi

clang++ $compiler_flags \
    $defines \
    -I src/ -I Vulkan-Headers/include/ \
    $translation_units \
    $libs \
    -o raytracer

popd > /dev/null
