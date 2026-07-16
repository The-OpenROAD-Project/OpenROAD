# Check that check_power_grid works when the net is empty
source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/gcd.def

set net [odb::dbNet_create [ord::get_db_block] VDDTest]
$net setSigType POWER

odb::dbBTerm_create $net "VDDTest"

catch { check_power_grid -net VDDTest } err
puts $err
