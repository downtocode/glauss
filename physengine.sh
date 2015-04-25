#!/usr/bin/env bash
RUN="./build/physengine"
DEBUG="gdb -ex=r --args"
if [ ! -f $PROGRAM ]; then
    PROGRAM="physengine"
fi

if [ "${@: -1}" = "--debug" ] || [ "${@: -1}" = "-d" ]; then
    RUN=$DEBUG" "$RUN
fi

$RUN $@
