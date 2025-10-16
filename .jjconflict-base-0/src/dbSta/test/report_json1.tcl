# report_path -format json
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def reg6.def

create_clock -name clk -period 10 clk
set_input_delay -clock clk 0 in1
set_output_delay -clock clk 0 out
find_timing
report_path -format json r3/D r
