# verilog constants 1'b0/1'b1
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog hierclock_gate.v
link_design hierclock

create_clock -name sys_clk -period 1.0 -waveform {0.0 1.0} [get_port clk_i]
create_clock -name clk1 -period 4.0 -waveform {0.0 3.0} [get_pins U1/clk1_o]
create_clock -name clk2 -period 8.0 -waveform {0.0 7.0} [get_pins U1/clk2_o]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

report_checks -path_delay max -fields {slew cap input nets fanout} -format full_clock_expanded
repair_design

write_verilog hierclock_out.v
diff_files hierclock_out.v hierclock_out.vok
