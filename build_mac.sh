#! /bin/bash

cmake -S . -B ./build/ -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++
make -C ./build/