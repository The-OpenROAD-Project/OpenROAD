# CUGR path: buffer removal where both routes reach the buffer gcell but the
# trees share no node there. n1 drops from metal4 to b1/A on metal1 through a
# via stack that passes b1/Z on metal3, where n2 starts. The merge must join
# the trees inside that via stack; it used to link the two tree roots with a
# wrong-way edge that failed with GRT-1252 when the merged net was rerouted.
# verify_demand reports any leaked demand.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty buf_m3.lib
read_lef Nangate45/Nangate45.lef
read_lef buf_m3.lef
read_def remove_buffers4.def

set_routing_layers -signal metal3-metal8 -clock metal3-metal8

global_route -verbose -use_cugr
set_debug_level GRT verify_demand 1

global_route -start_incremental
remove_buffers b1
global_route -end_incremental

# Move the sink so the merged net is ripped up and rerouted.
global_route -start_incremental
[[ord::get_db_block] findInst s1] setLocation 39900 32200
global_route -end_incremental

set segs_file [make_result_file "remove_buffers4_cugr.segs"]
write_global_route_segments $segs_file
diff_files remove_buffers4_cugr.segsok $segs_file
