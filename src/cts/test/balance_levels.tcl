source "helpers.tcl"
source "cts-helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

set block [make_array 300 200000 200000 150]

sta::db_network_defined

create_clock -period 5 clk

set_wire_rc -clock -layer metal5

set_cts_config -distance_between_buffers 100 \
  -sink_clustering_size 5 \
  -sink_clustering_max_diameter 60 \
  -wire_unit 20 \
  -num_static_layers 1 \
  -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3 

clock_tree_synthesis -sink_clustering_enable \
  -balance_levels

set def_file [make_result_file balance_levels.def]
write_def $def_file
diff_files balance_levels.defok $def_file
