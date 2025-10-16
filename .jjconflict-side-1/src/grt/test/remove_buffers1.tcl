# manual buffer removal test
# in1 -> b1 -> b2 -> b3 -> out1
# remove buffers b1 and b3 only
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def remove_buffers1.def

set_global_routing_layer_adjustment metal2 0.8
set_global_routing_layer_adjustment metal3 0.7
set_global_routing_layer_adjustment * 0.5

set_routing_layers -signal metal2-metal8 -clock metal3-metal8

global_route -verbose

# make sure sta works before/after removal

global_route -start_incremental
remove_buffers b1 b3
global_route -end_incremental

set def_file [make_result_file "remove_buffers1.def"]
set segs_file [make_result_file "remove_buffers1.segs"]

write_def $def_file
write_global_route_segments $segs_file

diff_file remove_buffers1.defok $def_file
diff_file remove_buffers1.segsok $segs_file
