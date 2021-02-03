## THIS TEST REQUIRES TEST "gcd_resize" TO RUN FIRST!
# gcd full meal deal
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def
read_sdc gcd.sdc

set_wire_rc -layer metal3
estimate_parasitics -placement

report_worst_slack

set_dont_use {AOI211_X1 OAI211_X1}


#Common
set db [ord::get_db]
set chip [$db getChip]
set block [$chip getBlock]
#End Common


#Redo Journal
odb::readEco $block results/gcd_resize.journal
odb::dbDatabase_commitEco $block
estimate_parasitics -placement
#End Redo Journal

report_checks -path_delay min_max 
report_check_types -max_slew -max_fanout -max_capacitance
report_worst_slack
report_long_wires 10
report_floating_nets -verbose
report_design_area


