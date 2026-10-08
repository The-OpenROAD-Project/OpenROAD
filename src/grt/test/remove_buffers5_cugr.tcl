# CUGR path: buffer removal where both routes reach the buffer gcell on
# disjoint layer ranges. n1 arrives on metal2 to reach b1/A on metal1, and n2
# starts at b1/Z on metal3. The merge must bridge the trees with a via, as
# the guides do; it used to link the two tree roots with a via edge between
# different gcells.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty buf_m3.lib
read_lef Nangate45/Nangate45.lef
read_lef buf_m3.lef
read_def remove_buffers4.def

set_routing_layers -signal metal2-metal8 -clock metal2-metal8

global_route -verbose -use_cugr

global_route -start_incremental
remove_buffers b1
global_route -end_incremental

# Move the sink so the merged net is ripped up and rerouted.
global_route -start_incremental
[[ord::get_db_block] findInst s1] setLocation 39900 32200
global_route -end_incremental

set segs_file [make_result_file "remove_buffers5_cugr.segs"]
write_global_route_segments $segs_file
diff_files remove_buffers5_cugr.segsok $segs_file
