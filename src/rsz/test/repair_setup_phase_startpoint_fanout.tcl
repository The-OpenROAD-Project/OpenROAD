# repair_timing -setup with STARTPOINT_FANOUT phase only
set repair_args [list -phases "STARTPOINT_FANOUT" -skip_last_gasp -skip_crit_vt_swap]
source "repair_setup1.tcl"
