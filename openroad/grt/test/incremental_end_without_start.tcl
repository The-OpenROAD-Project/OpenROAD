# global_route -end_incremental without a prior -start_incremental is an
# error, not a crash.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def remove_buffers1.def

set_routing_layers -signal metal2-metal8 -clock metal3-metal8

global_route

catch { global_route -end_incremental } error
puts $error
