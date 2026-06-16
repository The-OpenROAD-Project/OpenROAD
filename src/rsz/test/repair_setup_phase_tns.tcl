# repair_timing -setup with TNS phase only
set repair_args [list -phases "TNS" -skip_last_gasp -skip_crit_vt_swap]
source "repair_setup1.tcl"
