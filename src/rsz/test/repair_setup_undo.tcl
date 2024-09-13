# repair_timing -setup 2 corners
source "helpers.tcl"
define_corners fast slow
read_liberty -corner slow Nangate45/Nangate45_slow.lib
read_liberty -corner fast Nangate45/Nangate45_fast.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup1.def
create_clock -period 0.3 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

puts "initial QoR"
report_worst_slack -max
report_tns -digits 3
write_verilog_for_eqy repair_setup_undo before "None"

set db [ord::get_db]
set chip [$db getChip]
set block [$chip getBlock]
odb::dbDatabase_beginEco $block

repair_timing -setup
puts "post repair_timing QoR"
report_worst_slack -max
report_tns -digits 3

odb::dbDatabase_endEco $block
odb::dbDatabase_undoEco $block
estimate_parasitics -placement

puts "post undo repair_timing QoR"
report_worst_slack -max
report_tns -digits 3

run_equivalence_test repair_setup_undo ./Nangate45/work_around_yosys/ "None"
