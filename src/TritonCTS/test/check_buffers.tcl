source "helpers.tcl"
read_liberty sky130/sky130_tt.lib
read_lef sky130/sky130_tech.lef
read_lef sky130/sky130_std_cell.lef

read_def check_buffers.def

create_clock clk -period 10

clock_tree_synthesis -lut_file sky130/sky130.lut \
  -sol_list sky130/sky130.sol_list \
  -root_buf sky130_fd_sc_hs__clkbuf_1 \
  -wire_unit 20

set unconnected_buffers 0
foreach buf [get_cells clkbuf_*_clk] {
  set buf_name [get_name $buf]
  set input_pin [get_pin $buf_name/A]
  set input_net [get_net -of $input_pin]
  if { $input_net == "NULL" } {
    incr unconnected_buffers
  }
}

puts "#unconnected buffers: $unconnected_buffers"
