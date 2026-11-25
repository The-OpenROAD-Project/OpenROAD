# fully_rebuffer with set_opt_config -rebuffer_size_driver: drivers are sized
# together with their buffer trees, replacing some buffers.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_placed.def
create_clock [get_ports clk] -name core_clock -period 0.5
set_max_fanout 100 [current_design]
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

set_opt_config -rebuffer_size_driver true
report_worst_slack -max
report_tns
puts "area before: [format %.1f [expr { [rsz::design_area] * 1e12 }]]"

rsz::fully_rebuffer NULL

estimate_parasitics -placement
report_worst_slack -max
report_tns
puts "area after: [format %.1f [expr { [rsz::design_area] * 1e12 }]]"

reset_opt_config -rebuffer_size_driver
