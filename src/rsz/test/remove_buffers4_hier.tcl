source "helpers.tcl"

set test_name remove_buffers4_hier

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog remove_buffers4.v
link_design top -hier

create_clock [get_ports clk] -name clk -period 0.01
set_input_delay 0 -clock [get_clocks clk] -add_delay [get_ports in*]
set_output_delay 0 -clock [get_clocks clk] -add_delay [get_ports out*]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3

# make sure sta works before/after removal
estimate_parasitics -placement
report_checks

# Remove buffers
remove_buffers
# - 3 BUFs will be removed.
# - 1 BUF (b1) will not be removed because its input & output nets are
#   connected to ports.

# 3 report_checks results should be the same.
report_checks

estimate_parasitics -placement
report_checks

estimate_parasitics -placement
report_checks

set verilog_out [make_result_file $test_name.v]
write_verilog $verilog_out
diff_file $test_name.vok $verilog_out
