# Check that check_power_grid fails with missing terminals
source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/gcd.def
catch {check_power_grid -net VDD} err
puts $err
