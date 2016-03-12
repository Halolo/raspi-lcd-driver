#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

BUILD=false

usage() {
    echo "
-----
Usage
-----

This script generate the Makefiles used to compile the project.

Use these options to cross compile, or none of them for a native build:
-t: Toolchain path
-p: Toolchain prefix
-s: Sysroot path

Other options:
-b: Compile project
-h: Help"
}

while getopts ":t:p:s:bh" opt; do
    case $opt in
        t)
            TOOLCHAIN_PATH="$OPTARG"
        ;;
        p)
            TOOLCHAIN_PREFIX="$OPTARG"
        ;;
        s)
            SYSROOT="$OPTARG"
        ;;
        b)
            BUILD=true
        ;;
        h)
            usage
            exit 0
        ;;
        \?)
            echo "Invalid option: -$OPTARG"
            usage
            exit 1
        ;;
        :)
            echo "Option -$OPTARG requires an argument."
            usage
            exit 1
        ;;
    esac
done

if [ -d "$BUILD_DIR" ]; then
    rm -r "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"

pushd "$BUILD_DIR" > /dev/null

RET=0

if [ ! "$TOOLCHAIN_PATH" ] && [ ! "$TOOLCHAIN_PREFIX" ] && [ ! "$SYSROOT" ]; then
    echo "Native build. This must be run on target. To cross compile, provide '-t', '-p' and '-s' options"
    cmake -G"Eclipse CDT4 - Unix Makefiles" "../src"
    RET=$?
elif [ ! "$TOOLCHAIN_PATH" ] || [ ! "$TOOLCHAIN_PREFIX" ] || [ ! "$SYSROOT" ]; then
    echo "To cross compile, you must provide:
- the toolchain path using '-t' option
- the toolchain prefix using the '-p' option
- the sysroot path using the '-s' option"
    RET=1
else
    if [ ! -d "$TOOLCHAIN_PATH" ]; then
        echo "Can't find toolchain path, '$TOOLCHAIN_PATH' does not exists"
        RET=1
    fi

    if [ ! -d "$SYSROOT" ]; then
        echo "Can't find sysroot, '$SYSROOT' does not exists"
        RET=1
    fi

    GCC="$TOOLCHAIN_PATH/$TOOLCHAIN_PREFIX-gcc"
    GPP="$TOOLCHAIN_PATH/$TOOLCHAIN_PREFIX-g++"
    if [ ! -e "$GCC" ]; then
        echo "Can't find gcc, '$GCC' does not exists"
        RET=1
    fi

    if  [ ! -e "$GPP" ]; then
        echo "Can't find g++, '$GPP' does not exists"
        RET=1
    fi

    if [ $RET == 0 ]; then
        cmake -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="../src/cross-compile.cmake" -DCMAKE_FIND_ROOT_PATH="$SYSROOT" -DCMAKE_C_COMPILER="$GCC" -DCMAKE_CXX_COMPILER="$GPP" "../src"
        RET=$?
    fi
fi

if [ $BUILD == true ] && [ $RET == 0 ]; then
    echo "Build sources..."
    make
fi

popd > /dev/null

exit $RET
