#!/bin/bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j 8
cd ../JavaInterface/src
sh run.sh