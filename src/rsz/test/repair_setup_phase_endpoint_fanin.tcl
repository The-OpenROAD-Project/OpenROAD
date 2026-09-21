# repair_timing -setup with ENDPOINT_FANIN phase only
set repair_args [list -phases "ENDPOINT_FANIN" -skip_last_gasp -skip_crit_vt_swap]
source "repair_setup1.tcl"
