# Verify endpoint-to-startpoint phase order and progress header transitions.
set_debug_level RSZ repair_setup 1
set repair_args [list -phases "ENDPOINT_FANIN STARTPOINT_FANOUT" \
  -max_passes 1 -skip_last_gasp -skip_crit_vt_swap]
source "repair_setup1.tcl"
