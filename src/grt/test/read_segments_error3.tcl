# estimate parasitics based on gr results
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def est_rc2.def

catch {read_global_route_segments read_segments_error3.segs} error
puts $error
