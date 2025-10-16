# estimate_parasitics driver/load pins in same location
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_wire9.def

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

rsz::repair_net_cmd [get_net out1] 0 0 0
