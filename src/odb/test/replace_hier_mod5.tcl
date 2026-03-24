#
# replace_hier_mod5
#
# Test if uninstantiated module cannot be swapped because uninstantiated
# module is ignored.
#
source "helpers.tcl"

read_lef "data/sky130hd/sky130_fd_sc_hd.tlef"
read_lef "data/sky130hd/sky130_fd_sc_hd_merged.lef"

read_liberty "sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib"
#set_debug_level ODB dbReadVerilog 1
#set_debug_level ODB replace_design 1
read_verilog gcd_adder5.v
link_design -hier top
create_clock -name CLK -period 1 clk
set_max_delay -to carry_out 1.0

report_checks -through gcd_1/_carry_out_and_/B -fields input_pins
report_cell_usage gcd_1/_552_

set_debug_level ODB replace_design_check_sanity 1
set result [catch { replace_hier_module gcd_1/_552_ LCU_16_BRENT_KUNG }]
if { $result == 0 } {
  puts "Successfully replaced hier module"
  report_checks -through gcd_1/_carry_out_and_/B -fields input_pins
  report_cell_usage gcd_1/_552_
} else {
  puts "Failed to replaced hier module"
}
