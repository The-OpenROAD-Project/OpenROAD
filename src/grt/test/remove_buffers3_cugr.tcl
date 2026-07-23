# CUGR path: buffer removal merging routes that reach the buffer gcell on disjoint
# layer ranges: in1 -> b1 -> b2 -> out1, with b1 and b2 in the same gcell.
# b2 input pin is on metal1 and its output pin is on metal3, so the routes
# of n1 (metal1-metal2) and out1 (metal3) must be bridged with a via when
# b2 is removed (previously failed with GRT-0267 disconnected segments).
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty buf_m3.lib
read_lef Nangate45/Nangate45.lef
read_lef buf_m3.lef
read_def remove_buffers3.def

set_global_routing_layer_adjustment metal2 0.8
set_global_routing_layer_adjustment metal3 0.7
set_global_routing_layer_adjustment * 0.5

set_routing_layers -signal metal2-metal8 -clock metal3-metal8

global_route -verbose -use_cugr

global_route -start_incremental
remove_buffers b2
global_route -end_incremental

set def_file [make_result_file "remove_buffers3_cugr.def"]
set segs_file [make_result_file "remove_buffers3_cugr.segs"]

write_def $def_file
write_global_route_segments $segs_file

diff_file remove_buffers3_cugr.defok $def_file
diff_file remove_buffers3_cugr.segsok $segs_file
