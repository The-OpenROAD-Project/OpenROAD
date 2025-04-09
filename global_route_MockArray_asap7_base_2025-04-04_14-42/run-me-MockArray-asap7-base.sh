#!/usr/bin/env bash
source vars-MockArray-asap7-base.sh
if [[ ! -z ${GDB+x} ]]; then
    gdb --args openroad -no_init ${SCRIPTS_DIR}/global_route.tcl
else
    openroad -no_init ${SCRIPTS_DIR}/global_route.tcl
fi
