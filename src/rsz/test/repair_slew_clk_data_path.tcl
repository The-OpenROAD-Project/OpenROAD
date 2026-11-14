# repair_design honors set_max_transition -data_path [get_clocks ...].
# drvr (DFF_X1) Q drives three INV_X8 inputs: its slew (~0.084) is below the
# Nangate45 library default (0.199) but above the clock data-path limit
# (0.065).  report_check_types flags it; repair_design must too, and must
# size drvr against the clock limit: DFF_X2 (~0.044) meets it, so the repair
# is a resize with no buffers.  Before the fix repair_design saw no violation,
# and its driver sizing tie-broke to the smallest (still violating) DFF_X1.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_slew_clk_data_path.def

create_clock -period 1 [get_ports clk]
set_max_transition 0.065 -data_path [get_clocks clk]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

proc master { inst_name } {
  return [[[[ord::get_db_block] findInst $inst_name] getMaster] getName]
}
proc q_slew_meets_limit { } {
  set slew [get_property [get_pins drvr/Q] slew_max]
  return [expr { $slew <= 0.065 }]
}

report_check_types -max_slew -violators -digits 3
check "the clock data-path limit is violated before repair" \
  { expr { [sta::max_slew_violation_count] > 0 } } 1
check "the library default limit alone is met before repair" \
  { expr { [get_property [get_pins drvr/Q] slew_max] < 0.198 } } 1

set insts_before [llength [get_cells *]]
repair_design
report_check_types -max_slew -violators -digits 3

check "drvr is resized against the clock data-path limit" { master drvr } \
  DFF_X2
check "the resize alone repairs it: no buffers" \
  { llength [get_cells *] } $insts_before
check "drvr/Q meets the clock data-path limit" { q_slew_meets_limit } 1
check "no max slew violations remain" { sta::max_slew_violation_count } 0

exit_summary
