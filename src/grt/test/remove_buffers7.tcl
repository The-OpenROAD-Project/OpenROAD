# Remove a buffer whose input and output routes overlap: b2 is moved to the
# row of b3, so both routes reach it through the same gcells. The merged net
# keeps only the route between its remaining pins.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def remove_buffers1.def

[[ord::get_db_block] findInst b2] setLocation 40470 35000

set_global_routing_layer_adjustment metal2 0.8
set_global_routing_layer_adjustment metal3 0.7
set_global_routing_layer_adjustment * 0.5

set_routing_layers -signal metal2-metal8 -clock metal3-metal8

global_route

global_route -start_incremental
remove_buffers b2
global_route -end_incremental

report_wire_length -net n1 -global_route

set segs_file [make_result_file "remove_buffers7.segs"]
write_global_route_segments $segs_file
diff_files remove_buffers7.segsok $segs_file
