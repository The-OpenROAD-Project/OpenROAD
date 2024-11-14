source "helpers.tcl"

read_liberty ihp-sg13g2_data/sg13g2_stdcell_typ_1p20V_25C.lib
read_lef ihp-sg13g2_data/sg13g2_tech.lef
read_lef ihp-sg13g2_data/sg13g2_stdcell.lef
read_def check_max_fanout1.def

source ihp-sg13g2_data/setRC.tcl

create_clock -period 5 clk
set_wire_rc -clock -layer Metal5

clock_tree_synthesis -root_buf sg13g2_buf_4 \
                     -buf_list sg13g2_buf_4 \
                     -wire_unit 20 \
                     -sink_clustering_enable \
                     -distance_between_buffers 100 \
                     -sink_clustering_size 10 \
                     -sink_clustering_max_diameter 60 \
                     -num_static_layers 1 \
                     -obstruction_aware

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
