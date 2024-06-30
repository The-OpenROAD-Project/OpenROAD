# Check that check_power_grid works with a disconnected macro
source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_lef Nangate45_data/fakeram45_64x32.lef
read_def Nangate45_data/check_power_grid_disconnected.def
catch {check_power_grid -net VDD} err
puts $err
