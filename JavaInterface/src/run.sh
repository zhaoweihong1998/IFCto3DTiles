#!/bin/bash
export LD_LIBRARY_PATH=../../build/lib:../../build/bin/Release:../../build/bin/Debug:$LD_LIBRARY_PATH
java -Djava.library.path=../../build/lib:../../build/bin/Release:../../build/bin/Debug App ../ifc/room.ifc