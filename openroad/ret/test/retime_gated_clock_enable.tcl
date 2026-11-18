# The enable of a latch-based clock gate is read at its current depth, so
# the gate that drives it is never crossed.  Yosys does not model the clock
# gate cell, so the netlist is compared instead of proven equivalent.
source "helpers.tcl"
source "ret_helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_gated_clock_enable.v
link_design top
create_clock -period 5.0 clk

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4

set v_file [make_result_file retime_gated_clock_enable.v]
write_verilog $v_file
diff_files retime_gated_clock_enable.vok $v_file
