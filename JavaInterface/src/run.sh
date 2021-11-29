#!/bin/bash
export LD_LIBRARY_PATH=~/3DTiles/JavaInterface/lib:$LD_LIBRARY_PATH
java -Djava.library.path=../lib App ../ifc/room.ifc
