# Multi-threaded run of the global sizing coverage flow (see global_sizing.tcl).
# Exercises the parallel Phase-B worker path and asserts it produces the same
# netlist as the serial golden (global_sizing.vok).
source "helpers.tcl"
set global_sizing_threads 8
set global_sizing_result "global_sizing_threads"
source "global_sizing.tcl"
