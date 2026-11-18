source "helpers.tcl"

read_liberty ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib
read_lef ihp-sg13g2/sg13g2_tech.lef
read_lef ihp-sg13g2/sg13g2_stdcell.lef
read_def check_max_fanout3.def

source ihp-sg13g2/setRC.tcl

create_clock -period 5 clk
set_wire_rc -clock -layer Metal5

set_cts_config -wire_unit 20 \
  -distance_between_buffers 100 \
  -num_static_layers 1 \
  -clock_buffer_footprint DLY

report_cts_config

clock_tree_synthesis -sink_clustering_enable

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
