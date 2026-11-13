# Timing-driven repair_design leaves SCAN nets unbuffered because scan_opt
# reorders the scan chains after placement. The high-fanout net marked SCAN
# keeps all of its loads.
source helpers.tcl
read_liberty ./library/nangate45/NangateOpenCellLibrary_typical.lib

read_lef ./nangate45.lef
read_def ./simple01-td-repair.def

create_clock -name core_clock -period 0.5 clk

set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5

set scan_net [[ord::get_db_block] findNet {ctrl.state.out_reg\[1\].qi}]
$scan_net setSigType SCAN
set_max_fanout 4 [current_design]

global_placement -timing_driven

puts "scan net loads: [llength [$scan_net getITerms]]"
