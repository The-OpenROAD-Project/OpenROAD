# CUGR path: buffer removal where the buffer pins sit in different gcells, so
# an L-shaped connection is added. Its metal3 leg reaches n1 inside the via
# stack that drops from metal4 to b1/A on metal1, where n1's tree has no node.
# The merge must split that via stack to attach the connection; it used to
# leave n2's tree detached with its demand still committed, and it never
# committed the connection vias. verify_demand reports any leaked demand.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty buf_m3.lib
read_lef Nangate45/Nangate45.lef
read_lef buf_m3.lef
read_def remove_buffers6.def

set_routing_layers -signal metal3-metal8 -clock metal3-metal8

global_route -verbose -use_cugr
set_debug_level GRT verify_demand 1

global_route -start_incremental
remove_buffers b1
global_route -end_incremental

set segs_file [make_result_file "remove_buffers6_cugr.segs"]
write_global_route_segments $segs_file
diff_files remove_buffers6_cugr.segsok $segs_file
