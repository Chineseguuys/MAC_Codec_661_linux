
#! /bin/bash

MY_FILE="./build/test"
TARGET_FILE=test.out
THREADS=10

if [ $# -eq 0 ]; then
    cmake -S . -B ./build/ -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
    make -C ./build/
elif [ $# -eq 1 ]; then
    if [ "$1" == "-c" ]; then
        cmake -S . -B ./build/ -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
    fi

    if [ "$1" == "-b" ]; then
        make -C ./build/ -j ${THREADS}

        if [ -f "${MY_FILE}" ]; then
            echo "copy ${MY_FILE} to ${TARGET_FILE}"
            cp -u -v "${MY_FILE}" "./${TARGET_FILE}"
        fi
    fi
else
    echo "paramters error"
fi
