# repair_timing -setup with WNS_PATH phase only
set repair_args [list -phases "WNS_PATH" -skip_last_gasp -skip_crit_vt_swap]
source "repair_setup1.tcl"
