# repair_timing -hold -max_utilization
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_placed.def
create_clock [get_ports clk] -name core_clock -period 2

set_wire_rc -layer metal3
estimate_parasitics -placement

report_design_area
report_worst_slack -min
report_worst_slack -max

catch {repair_timing -hold -slack_margin .4 -max_utilization 13} error
puts $error

report_design_area
report_worst_slack -min
report_worst_slack -max
