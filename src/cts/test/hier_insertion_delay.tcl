# Test CTS insertion delay on hierarchical design with custom tile library
source "helpers.tcl"

source Nangate45/Nangate45.vars
read_liberty Nangate45/Nangate45_typ.lib
read_liberty array_tile_ins_delay.lib
read_lef Nangate45/Nangate45.lef
read_lef array_tile_ins_delay.lef
read_verilog hier_insertion_delay.v
link_design -hier multi_sink
read_def -floorplan_initialize hier_insertion_delay.def


source Nangate45/Nangate45.rc
source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk

create_clock -name core -period 5 clk

#set_debug_level CTS clustering 3
#set_debug_level CTS legalizer 3
#set_debug_level CTS Stree 4

set_cts_config -wire_unit 20 -distance_between_buffers 100 \
  -sink_clustering_size 10 -sink_clustering_max_diameter 60 \
  -num_static_layers 1 -root_buf CLKBUF_X3 -buf_list CLKBUF_X3

clock_tree_synthesis -sink_clustering_enable

set unconnected_buffers 0
foreach buf [get_cells clkbuf_*] {
  set buf_name [get_name $buf]
  set input_pin [get_pin $buf_name/A]
  set input_net [get_net -of $input_pin]
  if { $input_net == "NULL" } {
    incr unconnected_buffers
  }
}

puts "Found $unconnected_buffers unconnected buffers."
set vout_file [make_result_file hier_insertion_delay.v]
write_verilog $vout_file
diff_files hier_insertion_delay.vok $vout_file
