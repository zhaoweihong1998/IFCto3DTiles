#!/bin/bash
export LD_LIBRARY_PATH=../../build/lib:../../build/bin/Release:../../build/bin/Debug:$LD_LIBRARY_PATH
java -classpath ./ com.tile.ifc.App ../ifc/room.ifc
