source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog hierclock_gate.v
link_design hierclock
write_db noise.db

create_clock -name sys_clk -period 1.0 -waveform {0.0 1.0} [get_port clk_i]
create_clock -name clk1 -period 4.0 -waveform {0.0 3.0} [get_pins U1/clk1_o]
create_clock -name clk2 -period 8.0 -waveform {0.0 7.0} [get_pins U1/clk2_o]

set_wire_rc -clock -layer metal3

clock_tree_synthesis -root_buf CLKBUF_X3       -buf_list CLKBUF_X3   -wire_unit 20   -obstruction_aware                      -apply_ndr    

write_verilog hierclock_out.v


