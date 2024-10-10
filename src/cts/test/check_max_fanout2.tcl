source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def check_buffers.def

source Nangate45/Nangate45.vars
source Nangate45/Nangate45.rc

create_clock -period 5 clk

source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock  -layer $wire_rc_layer_clk

set_max_fanout 8 CLKBUF_X3

clock_tree_synthesis -root_buf CLKBUF_X3 \
                     -buf_list CLKBUF_X3 \
                     -wire_unit 20 \
                     -sink_clustering_enable \
                     -distance_between_buffers 100 \
                     -sink_clustering_size 10 \
                     -sink_clustering_max_diameter 60 \
                     -num_static_layers 1
                     
set_propagated_clock [all_clocks]
estimate_parasitics -placement
report_check_types -max_fanout -violators

set unconnected_buffers 0
foreach buf [get_cells clkbuf_*_clk] {
  set buf_name [get_name $buf]
  set input_pin [get_pin $buf_name/A]
  set input_net [get_net -of $input_pin]
  if { $input_net == "NULL" } {
    incr unconnected_buffers
  }
}

puts "Found $unconnected_buffers unconnected buffers."
