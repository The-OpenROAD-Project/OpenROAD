# set_wire_rc selectors matching no chip warn and are ignored
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def reg3.def

set_wire_rc -tech no_such_tech -resistance 1e-3 -capacitance 1e-1
set_wire_rc -redistribution_layer -resistance 1e-3 -capacitance 1e-1
