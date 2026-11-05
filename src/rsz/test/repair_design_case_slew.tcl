# repair_design load slew estimate with a `when`-disabled driver arc.
# set_case_analysis 1 on B disables the slower `when : "!B"` A->ZN arc of
# XNOR2_X1 for timing search, but the delay calculator still merges its slew
# into the ZN slew that the max slew check reports.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_design_case_slew.def

create_clock -period 1 clk1
set_input_delay 0 -clock clk1 {a b}
set_input_transition 0.02 {a b}
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

set_case_analysis 1 [get_ports b]
set_max_transition 0.08 [current_design]
# Keep the driver so that repair_design must buffer the net.
set_dont_touch [get_cells drvr]

puts "before: max_slew_violations=[sta::max_slew_violation_count]"
repair_design
estimate_parasitics -placement
puts "after: max_slew_violations=[sta::max_slew_violation_count]\
  instances=[llength [get_cells *]]"
report_check_types -max_slew -violators -digits 3
