# repair_timing -setup with custom multi-phase order
set repair_args [list -phases "WNS_PATH TNS ENDPOINT_FANIN" -skip_last_gasp -skip_crit_vt_swap]
source "repair_setup1.tcl"
