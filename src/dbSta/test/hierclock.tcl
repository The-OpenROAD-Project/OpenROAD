# verilog constants 1'b0/1'b1
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog hierclock_gate.v
link_design hierclock -hier

#make some hierarchical clocks.
create_clock -name clk1 -period 2.0 -waveform {0.0 1.0} [get_pins U2/clk_i]
create_clock -name clk2 -period 4.0 -waveform {0.0 2.0} [get_pins U3/clk_i]


report_checks -path_delay min -fields {slew cap input nets fanout} -format full_clock_expanded

set v_file [make_result_file hierclock_out.v]
write_verilog $v_file
diff_files $v_file hierclock_out.vok

