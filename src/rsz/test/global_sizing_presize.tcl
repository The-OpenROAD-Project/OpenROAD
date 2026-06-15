set ::env(RSZ_GLOBAL_SIZING_PRESIZE_MODE) 2

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup2.def
read_sdc repair_setup2.sdc

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

repair_timing -setup -phases GLOBAL_SIZING

unset ::env(RSZ_GLOBAL_SIZING_PRESIZE_MODE)
