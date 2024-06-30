source "helpers.tcl"
source "cts-helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

set block [make_array 300 200000 200000 150]

sta::db_network_defined

create_clock -period 5 clk

set_wire_rc -clock -layer metal5

clock_tree_synthesis -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3 \
  -wire_unit 20 \
  -sink_clustering_enable \
  -distance_between_buffers 100 \
  -sink_clustering_size 5 \
  -sink_clustering_max_diameter 60 \
  -balance_levels \
  -num_static_layers 1 \
  -obstruction_aware    

set def_file [make_result_file balance_levels.def]
write_def $def_file
diff_files balance_levels.defok $def_file
